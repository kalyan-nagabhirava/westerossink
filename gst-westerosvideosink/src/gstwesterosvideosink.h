/*
 * GStreamer
 * Copyright (C) 2016 kalyan-nagabhirava  <kalyankumar.nagabhirava@lnttechservice.com>
 * 
 */

#ifndef __GST_WESTEROSVIDEOSINK_H__
#define __GST_WESTEROSVIDEOSINK_H__

#include <gst/gst.h>

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

struct _GstwesterosVideoSink
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  gboolean silent;
};

struct _GstwesterosVideoSinkClass 
{
  GstElementClass parent_class;
};

GType gst_westeros_video_sink_get_type (void);

G_END_DECLS

#endif /* __GST_WESTEROSVIDEOSINK_H__ */
