
#ifndef __GST_WESTEROS_BUFFER_POOL_H__
#define __GST_WESTEROS_BUFFER_POOL_H__

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


/* buffer pool functions */
#define GST_TYPE_WESTEROS_BUFFER_POOL      (gst_westeros_buffer_pool_get_type())
#define GST_IS_WESTEROS_BUFFER_POOL(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_WESTEROS_BUFFER_POOL))
#define GST_WESTEROS_BUFFER_POOL(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_WESTEROS_BUFFER_POOL, GstWesterosBufferPool))
#define GST_WESTEROS_BUFFER_POOL_CAST(obj) ((GstWesterosBufferPool*)(obj))

struct _GstWesterosBufferPool
{
  GstBufferPool gst_bufferpool;
  GstwesterosVideoSink *sink;
  GstCaps *gst_caps;
  GstVideoInfo gst_VideoInfo;
  guint VWidth;
  guint VHeight;
};

struct _GstWosMetaData {
  GstMeta gst_metadata;
  GstwesterosVideoSink *sink;
  struct wl_buffer *wos_buffer;
  size_t buffer_size;
  void *data;
};

struct _GstWesterosBufferPoolClass
{
  GstBufferPoolClass parent_class;
};

typedef struct _GstWosMetaData GstWosMetaData;
typedef struct _GstWesterosBufferPool GstWesterosBufferPool;
typedef struct _GstWesterosBufferPoolClass GstWesterosBufferPoolClass;


GType gst_wos_metadata_get_type (void);
GType gst_westeros_buffer_pool_get_type (void);
GstBufferPool *gst_wayland_buffer_pool_new (GstwesterosVideoSink * sink);
const GstMetaInfo * gst_wos_metadata_get_info (void);

G_END_DECLS

#endif /*__GST_WESTEROS_BUFFER_POOL_H__*/
