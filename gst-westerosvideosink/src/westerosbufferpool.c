
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Object header */
#include "gstwesterosvideosink.h"
#include "westerosbufferpool.h"

/* Debugging category */
#include <gst/gstinfo.h>

/* Helper functions */
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

/* wl metadata */
GType
gst_wl_meta_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] =
      { "memory", "size", "colorspace", "orientation", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstWlMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static void
gst_wl_meta_free (GstWlMeta * meta, GstBuffer * buffer)
{
  gst_object_unref (meta->sink);
  munmap (meta->data, meta->size);
  wl_buffer_destroy (meta->wbuffer);
}

const GstMetaInfo *
gst_wl_meta_get_info (void)
{
  static const GstMetaInfo *wl_meta_info = NULL;

  if (g_once_init_enter (&wl_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_WL_META_API_TYPE, "GstWlMeta",
        sizeof (GstWlMeta), (GstMetaInitFunction) NULL,
        (GstMetaFreeFunction) gst_wl_meta_free,
        (GstMetaTransformFunction) NULL);
    g_once_init_leave (&wl_meta_info, meta);
  }
  return wl_meta_info;
}

/* bufferpool */
static void gst_westeros_buffer_pool_finalize (GObject * object);

#define gst_westeros_buffer_pool_parent_class parent_class
G_DEFINE_TYPE (GstWesterosBufferPool, gst_westeros_buffer_pool,
    GST_TYPE_BUFFER_POOL);

static gboolean
westeros_buffer_pool_set_config (GstBufferPool * pool, GstStructure * config)
{
  GstWesterosBufferPool *wpool = GST_WESTEROS_BUFFER_POOL_CAST (pool);
  GstVideoInfo info;
  GstCaps *caps;

  if (!gst_buffer_pool_config_get_params (config, &caps, NULL, NULL, NULL))
    goto wrong_config;

  if (caps == NULL)
    goto no_caps;

  /* now parse the caps from the config */
  if (!gst_video_info_from_caps (&info, caps))
    goto wrong_caps;

  GST_LOG_OBJECT (pool, "%dx%d, caps %" GST_PTR_FORMAT, info.width, info.height,
      caps);

  /*Fixme: Enable metadata checking handling based on the config of pool */

  wpool->caps = gst_caps_ref (caps);
  wpool->info = info;
  wpool->width = info.width;
  wpool->height = info.height;

  return GST_BUFFER_POOL_CLASS (parent_class)->set_config (pool, config);
  /* ERRORS */
wrong_config:
  {
    GST_WARNING_OBJECT (pool, "invalid config");
    return FALSE;
  }
no_caps:
  {
    GST_WARNING_OBJECT (pool, "no caps in config");
    return FALSE;
  }
wrong_caps:
  {
    GST_WARNING_OBJECT (pool,
        "failed getting geometry from caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
}

static struct wl_shm_pool *
make_shm_pool (wDisplay *display, int size, void **data)
{
  struct wl_shm_pool *pool;
  int fd;
  char filename[1024];
  static int init = 0;

  snprintf (filename, 256, "%s-%d-%s", "/tmp/wayland-shm", init++, "XXXXXX");

  fd = mkstemp (filename);
  if (fd < 0) {
    GST_ERROR ("open %s failed:", filename);
    return NULL;
  }
  if (ftruncate (fd, size) < 0) {
    GST_ERROR ("ftruncate failed:..!");
    close (fd);
    return NULL;
  }

  *data = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (*data == MAP_FAILED) {
    GST_ERROR ("mmap failed: ");
    close (fd);
    return NULL;
  }

  pool = wl_shm_create_pool (display->wos_shm, fd, size);

  close (fd);

  return pool;
}

static struct shm_pool *
shm_pool_create (wDisplay *display, size_t size)
{
  struct wos_shared_pool *pool = malloc (sizeof *pool);

  if (!pool)
    return NULL;

  pool->pool = make_shm_pool (display, size, &pool->buff);
  if (!pool->pool) {
    free (pool);
    return NULL;
  }

  pool->size = size;
  pool->count = 0;

  return pool;
}

static void *
shm_pool_allocate (struct wos_shared_pool *pool, size_t size, int *offset)
{
  if (pool->count + size > pool->size)
    return NULL;

  *offset = pool->count;
  pool->count += size;

  return (char *) pool->buff + *offset;
}

/* Start allocating from the beginning of the pool again */
static void
shm_pool_reset (struct wos_shared_pool *pool)
{
  pool->count = 0;
}

static GstWlMeta *
gst_buffer_add_westeros_meta (GstBuffer * buffer, GstWesterosBufferPool * wpool)
{
  GstWlMeta *wmeta;
  GstwesterosVideoSink *sink;
  void *data;
  gint offset;
  guint stride = 0;
  guint size = 0;

  sink = wpool->sink;
  stride = wpool->width * 4;
  size = stride * wpool->height;

  wmeta = (GstWlMeta *) gst_buffer_add_meta (buffer, GST_WL_META_INFO, NULL);
  wmeta->sink = gst_object_ref (sink);

  /*Fixme: size calculation should be more grcefull, have to consider the padding */
  if (!sink->shared_pool) {
    sink->shared_pool = shm_pool_create (sink->window, size * 15);
    shm_pool_reset (sink->shared_pool);
  }

  if (!sink->shared_pool) {
    GST_ERROR ("Failed to create shm_pool");
    return NULL;
  }

  data = shm_pool_allocate (sink->shared_pool, size, &offset);
  if (!data)
    return NULL;

  wmeta->wbuffer = wl_shm_pool_create_buffer (sink->shared_pool->pool, offset,
      sink->VWidth, sink->VHeight, stride, sink->format);

  wmeta->data = data;
  wmeta->size = size;

  gst_buffer_append_memory (buffer,
      gst_memory_new_wrapped (GST_MEMORY_FLAG_NO_SHARE, data,
          size, 0, size, NULL, NULL));


  return wmeta;
}

static GstFlowReturn
westeros_buffer_pool_alloc (GstBufferPool * pool, GstBuffer ** buffer,
    GstBufferPoolAcquireParams * params)
{
  GstWesterosBufferPool *w_pool = GST_WESTEROS_BUFFER_POOL_CAST (pool);
  GstBuffer *w_buffer;
  GstWlMeta *meta;

  w_buffer = gst_buffer_new ();
  meta = gst_buffer_add_westeros_meta (w_buffer, w_pool);
  if (meta == NULL) {
    gst_buffer_unref (w_buffer);
    goto no_buffer;
  }
  *buffer = w_buffer;

  return GST_FLOW_OK;

  /* ERROR */
no_buffer:
  {
    GST_WARNING_OBJECT (pool, "can't create buffer");
    return GST_FLOW_ERROR;
  }
}

GstBufferPool *
gst_westeros_buffer_pool_new (GstwesterosVideoSink * sink)
{
  GstWesterosBufferPool *pool;

  g_return_val_if_fail (GST_IS_WESTEROSVIDEOSINK (sink), NULL);
  pool = g_object_new (GST_TYPE_WESTEROS_BUFFER_POOL, NULL);
  pool->sink = gst_object_ref (sink);

  return GST_BUFFER_POOL_CAST (pool);
}

static void
gst_westeros_buffer_pool_class_init (GstWesterosBufferPoolClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBufferPoolClass *gstbufferpool_class = (GstBufferPoolClass *) klass;

  gobject_class->finalize = gst_westeros_buffer_pool_finalize;

  gstbufferpool_class->set_config = westeros_buffer_pool_set_config;
  gstbufferpool_class->alloc_buffer = westeros_buffer_pool_alloc;
}

static void
gst_westeros_buffer_pool_init (GstWesterosBufferPool * pool)
{
}

static void
gst_westeros_buffer_pool_finalize (GObject * object)
{
  GstWesterosBufferPool *pool = GST_WESTEROS_BUFFER_POOL_CAST (object);

  gst_object_unref (pool->sink);

  G_OBJECT_CLASS (gst_westeros_buffer_pool_parent_class)->finalize (object);
}
