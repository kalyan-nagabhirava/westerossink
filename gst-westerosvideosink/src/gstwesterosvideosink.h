/*
 * GStreamer
 * Copyright (C) 2016 kalyan-nagabhirava  <kalyankumar.nagabhirava@lnttechservice.com>
 * 
 */

#ifndef __GST_WESTEROSVIDEOSINK_H__
#define __GST_WESTEROSVIDEOSINK_H__

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

#include <wayland-client.h>



G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_WESTEROSVIDEOSINK \
  (gst_westeros_video_sink_get_type())
#define GST_WESTEROSVIDEOSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_WESTEROSVIDEOSINK,GstwesterosVideoSink))
#define GST_WESTEROSVIDEOSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_WESTEROSVIDEOSINK,GstwesterosVideoSinkClass))
#define GST_IS_WESTEROSVIDEOSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_WESTEROSVIDEOSINK))
#define GST_IS_WESTEROSVIDEOSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_WESTEROSVIDEOSINK))

typedef struct _GstwesterosVideoSink      GstwesterosVideoSink;
typedef struct _GstwesterosVideoSinkClass GstwesterosVideoSinkClass;

enum
{
  PROP_0,
  PROP_WAYLAND_DISPLAY
};


struct westeros_window_display
{
  struct wl_display *wos_display;
  struct wl_registry *wos_registry;
  struct wl_compositor *wos_compositor;
  struct wl_shell *wos_shell;
  struct wl_shm *wos_shm;
  uint32_t wos_formats;

  int width, height;
  struct wl_surface *wos_surface;
  struct wl_shell_surface *wos_shell_surface;
  struct wl_buffer *wos_buffer;
  struct wl_callback *wos_callback;
  //guint redraw_pending :1;
  guint redraw_check :1;

};


struct wos_shared_pool {
  struct wl_shm_pool *pool;
  size_t size;
  size_t count;
  void *buff;
};

typedef westeros_window_display wDisplay;



struct _GstwesterosVideoSink
{
  GstVideoSink parent;

  wDisplay *window;
  struct wos_shared_pool *shared_pool;

  GstBufferPool *pool;

  GMutex wos_lock;

  gint VWidth;
  gint VHeight;
  uint32_t format;

};

struct _GstwesterosVideoSinkClass 
{
  GstElementClass parent_class;
};

GType gst_westeros_video_sink_get_type (void);

G_END_DECLS

#endif /* __GST_WESTEROSVIDEOSINK_H__ */
