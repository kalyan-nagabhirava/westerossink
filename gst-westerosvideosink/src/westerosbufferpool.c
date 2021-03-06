/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 L&T Technology Services
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */




#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gst/gstinfo.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include "gstwesterosvideosink.h"
#include "westerosbufferpool.h"

#define SIZE 1024
#define SHM_FILE_PATH "/tmp/westeros-shm"

#define GST_WOS_METADATA_GET_TYPE  (gst_wos_metadata_get_type())
#define GST_WOS_METADATA_INFO  (gst_wos_metadata_get_info())

GType 
gst_wos_metadata_get_type ()
{
  static const gchar *api_tags[] =
      { "memory", 
        "size", 
        "colorspace", 
        "orientation", 
         NULL 
      };
  static volatile GType g_type;

  if (g_once_init_enter (&g_type)) {
    GType _type = gst_meta_api_type_register ("GstWosMetaDataAPI", api_tags);
    g_once_init_leave (&g_type, _type);
  }
  return g_type;
}



static void 
gst_wos_metadata_free (GstWosMetaData * wos_metadata, GstBuffer *gst_buffer)
{
  gst_object_unref (wos_metadata->sink);
  munmap (wos_metadata->data, wos_metadata->buffer_size);
  wl_buffer_destroy (wos_metadata->wos_buffer);
}



const GstMetaInfo *
gst_wos_metadata_get_info ()
{
  static const GstMetaInfo *wos_metadata_info = NULL;
  if (g_once_init_enter (&wos_metadata_info)) 
  {
    const GstMetaInfo *gst_meta =
        gst_meta_register (GST_WOS_METADATA_GET_TYPE, "GstWosMetaData",sizeof (GstWosMetaData), (GstMetaInitFunction) NULL,
        (GstMetaFreeFunction) gst_wos_metadata_free,
        (GstMetaTransformFunction) NULL);

    g_once_init_leave (&wos_metadata_info, gst_meta);
  }
  return wos_metadata_info;
}



GstWosMetaData* 
gst_buffer_get_wos_metadata(GstBuffer *gst_buffer)
{
  GstWosMetaData *metaData;
  GType type = gst_wos_metadata_get_type ();
  metaData = (GstWosMetaData*)gst_buffer_get_meta(gst_buffer,type);
  return metaData;
}



static void gst_westeros_buffer_pool_finalize (GObject * object);

#define gst_westeros_buffer_pool_parent_class parent_class
G_DEFINE_TYPE (GstWesterosBufferPool, gst_westeros_buffer_pool, GST_TYPE_BUFFER_POOL);

static gboolean 
westeros_buffer_pool_set_config (GstBufferPool * gst_bufferpool, 
                                 GstStructure * gst_config)
{
  GstVideoInfo gst_videoInfo ;
  GstCaps *gst_caps;
  GstWesterosBufferPool *wos_bufferpool = GST_WESTEROS_BUFFER_POOL_CAST (gst_bufferpool);

  GstwesterosVideoSink *sink  = wos_bufferpool->sink;

  if (!gst_buffer_pool_config_get_params (gst_config, &gst_caps, NULL, NULL, NULL))
    goto config_failed;

  if (gst_caps == NULL)
    goto caps_unavailable;

  if (!gst_video_info_from_caps (&gst_videoInfo, gst_caps))
    goto invalid_caps;

  GST_LOG_OBJECT (gst_bufferpool, "%dx%d, caps %" GST_PTR_FORMAT, gst_videoInfo.width, gst_videoInfo.height,gst_caps);


  wos_bufferpool->gst_caps = gst_caps_ref (gst_caps);
  wos_bufferpool->gst_VideoInfo = gst_videoInfo;

  wos_bufferpool->VWidth = gst_videoInfo.width;
  wos_bufferpool->VHeight = gst_videoInfo.height;

  return GST_BUFFER_POOL_CLASS (parent_class)->set_config (gst_bufferpool, gst_config);

  /* ERRORS */
config_failed:
  {
    GST_WARNING_OBJECT (gst_bufferpool, "invalid config");
    return FALSE;
  }
caps_unavailable:
  {
    GST_WARNING_OBJECT (gst_bufferpool, "no caps in config");
    return FALSE;
  }
invalid_caps:
  {
    GST_WARNING_OBJECT (gst_bufferpool,
        "failed getting geometry from caps %" GST_PTR_FORMAT, gst_caps);
    return FALSE;
  }
}

static struct wl_shm_pool*
make_wos_shm_pool (wDisplay *display, 
		   int size,
                   void **data)
{
  struct wl_shm_pool *wos_pool;
  char shm_filename[SIZE];
  static int count = 0;
  int fd;

// Create temp uniq file
  snprintf (shm_filename, SIZE, "%s-%d-%s", SHM_FILE_PATH, count++, "XXXXXX");

  if( (fd = mkstemp (shm_filename)) < 0) 
  {
    GST_ERROR ("File  %s creation failed:", shm_filename);
    return NULL;
  }
  if (ftruncate (fd, size) < 0) 
  {
    GST_ERROR ("file %s truncate failed",shm_filename);
    close (fd);
    return NULL;
  }

  *data = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); //map device to memory
  if (*data == MAP_FAILED) 
   {
    GST_ERROR ("mmap failed: ");
    close (fd);
    return NULL;
   }

  wos_pool = wl_shm_create_pool (display->wos_shm, fd, size);
  close (fd);
  return wos_pool;
}



static struct shm_pool*
wosCreateSharedPool (wDisplay *display, 
		     size_t buffer_size)
{
  wSharedPool *sharedPool = NULL;
  sharedPool = (wSharedPool*) malloc (sizeof *sharedPool);

  if (sharedPool == NULL)
    return NULL;

  sharedPool->pool = make_wos_shm_pool (display, buffer_size, &sharedPool->buff);
  if (sharedPool->pool == NULL) 
  {
    free (sharedPool);
    return NULL;
  }
  sharedPool->size = buffer_size;
  sharedPool->count = 0;

  return sharedPool;
}

static void *
wosAllocateSharedPool (wSharedPool *sharedPool, 
		       size_t size, 
                       int *offset)
{
  void *data ;
  if ( (sharedPool->count + size) > sharedPool->size)
    return NULL;

  *offset = sharedPool->count;
  sharedPool->count += size;

  data = (char *) sharedPool->buff + *offset;
  return data;
}

static void 
shm_pool_reset (wSharedPool *sharedPool)
{
  sharedPool->count = 0;
}

static GstWosMetaData *
gst_buffer_add_westeros_metadata (GstBuffer * gst_buffer, 
                                 GstWesterosBufferPool * wos_buffer_pool)
{
  GstwesterosVideoSink *sink  = wos_buffer_pool->sink;
  wDisplay *display = sink->wos_display;
  wSharedPool *sharedPool = sink->wos_sharedPool;

  GstWosMetaData *wos_metadata = NULL;
  void *data;
  gint offset;
  guint stride,size;

  stride = wos_buffer_pool->VWidth * 4;
  size = wos_buffer_pool->VHeight * stride;


  wos_metadata = (GstWosMetaData *) gst_buffer_add_meta (gst_buffer, GST_WOS_METADATA_INFO, NULL);
  wos_metadata->sink = gst_object_ref (sink);

  if (!sharedPool ) 
  {
    sharedPool = wosCreateSharedPool (display, size * 15); 
    shm_pool_reset (sharedPool);
  }

  if (!sharedPool) 
    return NULL;

  data = wosAllocateSharedPool (sharedPool, size, &offset);
  if (!data )
    return NULL;

  wos_metadata->wos_buffer = wl_shm_pool_create_buffer (sharedPool->pool, 
                                      offset,sink->defaultWidth, sink->defaultHeight, stride, sink->format);
  wos_metadata->data = data;
  wos_metadata->buffer_size = size;

  gst_buffer_append_memory (gst_buffer,
      gst_memory_new_wrapped (GST_MEMORY_FLAG_NO_SHARE, data,
          size, 0, size, NULL, NULL));

  return wos_metadata;
}

static GstFlowReturn 
westeros_buffer_pool_alloc (GstBufferPool * gst_pool, 
                            GstBuffer ** gst_buffer, 
                           GstBufferPoolAcquireParams * gst_bufferpoolParams)
{
  GstWesterosBufferPool *wos_pool = GST_WESTEROS_BUFFER_POOL_CAST (gst_pool);
  GstWosMetaData *wos_metadata ;

  GstBuffer *wos_gstBuffer = gst_buffer_new ();
  wos_metadata = gst_buffer_add_westeros_metadata (wos_gstBuffer, wos_pool);

  if (wos_metadata == NULL) 
   {
    gst_buffer_unref (wos_gstBuffer);
    goto buffer_unavailable;
  }
  *gst_buffer = wos_gstBuffer;

  return GST_FLOW_OK;

buffer_unavailable:
  {
    GST_WARNING_OBJECT (gst_pool, "can't create buffer");
    return GST_FLOW_ERROR;
  }
}
static void 
gst_westeros_buffer_pool_finalize (GObject * object)
{
  GstWesterosBufferPool *wos_pool = GST_WESTEROS_BUFFER_POOL_CAST (object);

  gst_object_unref (wos_pool->sink);
  G_OBJECT_CLASS (gst_westeros_buffer_pool_parent_class)->finalize (object);
}



GstBufferPool *
gst_westeros_buffer_pool_new (GstwesterosVideoSink * sink)
{
  GstWesterosBufferPool *wos_pool = NULL;

  g_return_val_if_fail (GST_IS_WESTEROSVIDEOSINK (sink), NULL);
  wos_pool = g_object_new (GST_TYPE_WESTEROS_BUFFER_POOL, NULL);
  wos_pool->sink = gst_object_ref (sink);
  return GST_BUFFER_POOL_CAST (wos_pool);
}

static void 
gst_westeros_buffer_pool_class_init (GstWesterosBufferPoolClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBufferPoolClass *gstbufferpool_class = (GstBufferPoolClass *) klass;
  gstbufferpool_class->alloc_buffer = westeros_buffer_pool_alloc;
  gstbufferpool_class->set_config = westeros_buffer_pool_set_config;
  gobject_class->finalize = gst_westeros_buffer_pool_finalize;
}

static void 
gst_westeros_buffer_pool_init (GstWesterosBufferPool * pool)
{
}


