
#ifndef __GST_WAYLAND_BUFFER_POOL_H__
#define __GST_WAYLAND_BUFFER_POOL_H__

#include "gstwesterosvideosink.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/gstvideometa.h>


G_BEGIN_DECLS


typedef struct _GstWlMeta GstWlMeta;

typedef struct _GstWesterosBufferPool GstWesterosBufferPool;
typedef struct _GstWesterosBufferPoolClass GstWesterosBufferPoolClass;

GType gst_wl_meta_api_get_type (void);
#define GST_WL_META_API_TYPE  (gst_wl_meta_api_get_type())
const GstMetaInfo * gst_wl_meta_get_info (void);
#define GST_WL_META_INFO  (gst_wl_meta_get_info())

#define gst_buffer_get_wl_meta(b) ((GstWlMeta*)gst_buffer_get_meta((b),GST_WL_META_API_TYPE))

struct _GstWlMeta {
  GstMeta meta;

  GstwesterosVideoSink *sink;

  struct wl_buffer *wbuffer;
  void *data;
  size_t size;
};

/* buffer pool functions */
#define GST_TYPE_WESTEROS_BUFFER_POOL      (gst_westeros_buffer_pool_get_type())
#define GST_IS_WESTEROS_BUFFER_POOL(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WESTEROS_BUFFER_POOL))
#define GST_WESTEROS_BUFFER_POOL(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_WESTEROS_BUFFER_POOL, GstWesterosBufferPool))
#define GST_WESTEROS_BUFFER_POOL_CAST(obj) ((GstWesterosBufferPool*)(obj))

struct _GstWesterosBufferPool
{
  GstBufferPool bufferpool;

  GstwesterosVideoSink *sink;

  /*Fixme: keep all these in GstWesterosBufferPoolPrivate*/
  GstCaps *caps;
  GstVideoInfo info;
  guint width;
  guint height;
};

struct _GstWesterosBufferPoolClass
{
  GstBufferPoolClass parent_class;
};

GType gst_westeros_buffer_pool_get_type (void);

GstBufferPool *gst_wayland_buffer_pool_new (GstwesterosVideoSink * sink);

G_END_DECLS

#endif /*__GST_WAYLAND_BUFFER_POOL_H__*/
