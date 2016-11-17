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
#include <stdbool.h>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/gstvideometa.h>

#include <wayland-client.h>
#include <simpleshell-client-protocol.h>



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



struct wosDisplay_t
{
  struct wl_display *wos_display;
  struct wl_registry *wos_registry;
  struct wl_compositor *wos_compositor;
  struct wl_simple_shell *wos_shell;
  struct wl_shm *wos_shm;
  uint32_t wos_formats;

  int width, height;
  struct wl_surface *wos_surface;
  struct wl_shell_surface *wos_shell_surface;
  struct wl_buffer *wos_buffer;
  struct wl_callback *wos_callback;
  guint redraw_check :1;
  uint32_t wos_surfaceId;

};


struct wosSharedPool_t {
  struct wl_shm_pool *pool;
  size_t size;
  size_t count;
  void *buff;
};

typedef struct wosDisplay_t wDisplay;
typedef struct wosSharedPool_t wSharedPool;

struct _GstwesterosVideoSink
{
  GstVideoSink parent;

  wDisplay *wos_display;
  wSharedPool *wos_sharedPool;
  GstBufferPool *wos_gstPool;
  GMutex wos_lock;

  gint defaultWidth;
  gint defaultHeight;

  int windowX;
  int windowY;
  int windowWidth;
  int windowHeight;
  float zorder; 
  float opacity;
  bool visible;

  uint32_t format;
  bool geometrySet;
};

struct _GstwesterosVideoSinkClass 
{
  GstVideoSinkClass parent_class;
};

typedef struct _GstwesterosVideoSink      GstwesterosVideoSink;
typedef struct _GstwesterosVideoSinkClass GstwesterosVideoSinkClass;

GType gst_westeros_video_sink_get_type (void);

G_END_DECLS

#endif /* __GST_WESTEROSVIDEOSINK_H__ */
