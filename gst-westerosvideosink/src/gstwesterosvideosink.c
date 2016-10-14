/*
 * L&T TS 
 * Copyright (C) 2016 kalyan-nagabhirava <<kalyankumar.nagabhirava@lnttechservices.com>>
 * add other users
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstwesterosvideosink.h"
#include "westerosbufferpool.h"

GST_DEBUG_CATEGORY_STATIC (gst_westeros_video_sink_debug);
#define GST_CAT_DEFAULT gst_westeros_video_sink_debug

#define DEFAULT_WINDOW_X (0)
#define DEFAULT_WINDOW_Y (0)
#define DEFAULT_WINDOW_WIDTH (1280)
#define DEFAULT_WINDOW_HEIGHT (720)

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720


#if G_BYTE_ORDER == G_BIG_ENDIAN
#define SINK_CAPS "{xRGB, ARGB}"
#else
#define SINK_CAPS "{BGRx, BGRA}"
#endif



/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_WESTEROS_DISPLAY,
  PROP_WINDOW_SET
};



static GstStaticPadTemplate gst_westeros_video_sink_pad_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (SINK_CAPS))
    );


typedef struct
{
  uint32_t wos_format;
  GstVideoFormat gst_format;
} wos_VideoFormat;


static const wos_VideoFormat wos_video_formats[] = {
#if G_BYTE_ORDER == G_BIG_ENDIAN
  {WL_SHM_FORMAT_XRGB8888, GST_VIDEO_FORMAT_xRGB},
  {WL_SHM_FORMAT_ARGB8888, GST_VIDEO_FORMAT_ARGB},
#else
  {WL_SHM_FORMAT_XRGB8888, GST_VIDEO_FORMAT_BGRx},
  {WL_SHM_FORMAT_ARGB8888, GST_VIDEO_FORMAT_BGRA},
#endif
};


//---------------------------------------------------------------------------------------------------------------------------

#define gst_westeros_video_sink_parent_class parent_class

G_DEFINE_TYPE (GstwesterosVideoSink, gst_westeros_video_sink, GST_TYPE_VIDEO_SINK);

static void gst_westeros_video_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_westeros_video_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void gst_westeros_video_sink_finalize(GObject *object);

static GstStateChangeReturn gst_westeros_video_sink_change_state(GstElement *element, GstStateChange transition);

static gboolean gst_westeros_video_sink_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);

static GstFlowReturn gst_westeros_video_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

static gboolean gst_westeros_video_sink_query(GstElement *element, GstQuery *query);

static gboolean gst_westeros_video_sink_start(GstBaseSink *base_sink);

static gboolean gst_westeros_video_sink_stop(GstBaseSink *base_sink);

static GstFlowReturn gst_westeros_video_sink_render(GstBaseSink *base_sink, GstBuffer *buffer);

static GstFlowReturn gst_westeros_video_sink_preroll(GstBaseSink *base_sink, GstBuffer *buffer);

static gboolean gst_westeros_video_sink_propose_allocation (GstBaseSink * bsink, GstQuery * query);

static GstCaps *gst_westeros_video_sink_get_caps (GstBaseSink * bsink, GstCaps * filter);

static gboolean gst_westeros_video_sink_set_caps (GstBaseSink * bsink, GstCaps * caps);
//-----------------------------------------------------------------------------------------------------------------------------



static void
gst_westeros_video_sink_class_init (GstwesterosVideoSinkClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseSinkClass *gstbasesink_class = (GstBaseSinkClass *) klass; 

  gobject_class->set_property = gst_westeros_video_sink_set_property;
  gobject_class->get_property = gst_westeros_video_sink_get_property;
  gobject_class->finalize = gst_westeros_video_sink_finalize;

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_westeros_video_sink_get_caps);
  gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_westeros_video_sink_set_caps);


  gstbasesink_class->start= GST_DEBUG_FUNCPTR (gst_westeros_video_sink_start);
  gstbasesink_class->render= GST_DEBUG_FUNCPTR (gst_westeros_video_sink_render);
  gstbasesink_class->preroll= GST_DEBUG_FUNCPTR (gst_westeros_video_sink_preroll);   
  gstbasesink_class->propose_allocation = GST_DEBUG_FUNCPTR (gst_westeros_video_sink_propose_allocation);

/*Add Property*/
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_WINDOW_SET,
       g_param_spec_string ("window_set", "window set",
           "Window Set Format: x,y,width,height",
           NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PROP_WESTEROS_DISPLAY,
      g_param_spec_pointer ("westeros-display", "Wayland Display",
          "Wayland  Display handle created by the application ",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

#ifdef USE_GST1
/*Sink pad*/
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_westeros_video_sink_pad_template));

/*Debug Level*/
  GST_DEBUG_CATEGORY_INIT (gst_westeros_video_sink_debug, "westerossink", 0, 
      "westerossink element");

/*Plugin Details*/
  gst_element_class_set_details_simple (gstelement_class, 
      "Westeros Video Sink",
      "Sink",
      "Writes buffers to the westeros wayland compositor",
      "LNT-TS");
#endif

}

static void gst_westeros_video_sink_init (GstwesterosVideoSink * sink)
{
  sink->window =  NULL;
  sink->shared_pool = NULL;
  sink->pool  = NULL;
   #ifdef GLIB_VERSION_2_32 
   g_mutex_init( &sink->wos_lock );
   #else
   sink->wos_lock= g_mutex_new();
   #endif

   sink->VWidth= DEFAULT_WIDTH;
   sink->VHeight= DEFAULT_HEIGHT;


   sink->windowX= DEFAULT_WINDOW_X;
   sink->windowY= DEFAULT_WINDOW_Y;
   sink->windowWidth= DEFAULT_WINDOW_WIDTH;
   sink->windowHeight= DEFAULT_WINDOW_HEIGHT;

   //sink->window->wos_surfaceId = 0;

}

static void gst_westeros_video_sink_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstwesterosVideoSink *sink = GST_WESTEROSVIDEOSINK (object);

  switch (prop_id) {
    case PROP_SILENT:
    {
      //TODO
      break;
    }
    case PROP_WESTEROS_DISPLAY:
    {
     //TODO
        break;
    }
    case PROP_WINDOW_SET:
    {
         const gchar *str= g_value_get_string(value);
         gchar **parts= g_strsplit(str, ",", 4);
         
         if ( !parts[0] || !parts[1] || !parts[2] || !parts[3] )
         {
            GST_ERROR( "Bad window properties string" );
         }
         else
         {         
            sink->windowX= atoi( parts[0] );
            sink->windowY= atoi( parts[1] );
            sink->windowWidth= atoi( parts[2] );
            sink->windowHeight= atoi( parts[3] );
           #if 0 
            //if ( sink->window->wos_shell && sink->window->wos_surfaceId ) //ori
            if ( sink->window->wos_shell )
            {
               wl_simple_shell_set_geometry( sink->window->wos_shell, sink->window->wos_surfaceId, 
                                             sink->windowX, sink->windowY, 
                                             sink->windowWidth, sink->windowHeight );
                                             
               printf("gst_westeros_sink_set_property set window rect (%d,%d,%d,%d)\n", 
                       sink->windowX, sink->windowY, sink->windowWidth, sink->windowHeight );
                       
               if ( (sink->windowWidth > 0) && (sink->windowHeight > 0 ) )
               {
                  wl_simple_shell_set_visible( sink->window->wos_shell, sink->window->wos_surfaceId, true);
                  
                  wl_simple_shell_get_status( sink->window->wos_shell, sink->window->wos_surfaceId);
                  
                  wl_display_flush( sink->window->wos_display );
               }
            }            
                    
            sink->VWidth= sink->windowWidth;
            sink->VHeight= sink->windowHeight;
           #endif
         }

         g_strfreev(parts);
         break;
      }




    default:
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  }
}

static void gst_westeros_video_sink_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstwesterosVideoSink *filter = GST_WESTEROSVIDEOSINK (object);

  switch (prop_id) {
    case PROP_SILENT:
    {
      //TODO
      break;
    }
    case PROP_WESTEROS_DISPLAY:
    {
     //TODO
      break;
    }
    case PROP_WINDOW_SET:
     {
        break;
     }
    default:
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  }
}

static void destroy_wos_display (wDisplay *display)
{
  if(!display)
	return;
//destroy window params
  if (display->wos_callback)
    wl_callback_destroy (display->wos_callback);

  if (display->wos_buffer)
    wl_buffer_destroy (display->wos_buffer);

  if (display->wos_shell_surface)
    wl_shell_surface_destroy (display->wos_shell_surface);

  if (display->wos_surface)
    wl_surface_destroy (display->wos_surface);

//destroy display params
  if (display->wos_shm)
    wl_shm_destroy (display->wos_shm);

  if (display->wos_shell)
    wl_shell_destroy (display->wos_shell);

  if (display->wos_compositor)
    wl_compositor_destroy (display->wos_compositor);

  wl_display_flush (display->wos_display);
  wl_display_disconnect (display->wos_display);


  free (display);
}
static void destroy_shared_pool (struct wos_shared_pool *pool)
{
  munmap (pool->buff, pool->size);
  wl_shm_pool_destroy (pool->pool);
  free (pool);
}


static void gst_westeros_video_sink_finalize(GObject *object)
{
   GstwesterosVideoSink *sink = GST_WESTEROSVIDEOSINK(object);

   GST_DEBUG_OBJECT (sink, "Finalizing the sink..");

   if (sink)
     destroy_wos_display (sink->window);

   if (sink->shared_pool)
     destroy_shared_pool (sink->shared_pool);

  g_mutex_clear (&sink->wos_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);

}


static GstStateChangeReturn gst_westeros_video_sink_change_state(GstElement *element, GstStateChange transition)
{
   GstStateChangeReturn result= GST_STATE_CHANGE_SUCCESS;
   GstwesterosVideoSink *sink= GST_WESTEROSVIDEOSINK(element);
   gboolean passToDefault= true;

   GST_DEBUG_OBJECT(element, "westeros-sink: change state from %s to %s\n",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));


   if (GST_STATE_TRANSITION_CURRENT(transition) == GST_STATE_TRANSITION_NEXT(transition))
   {
      return GST_STATE_CHANGE_SUCCESS;
   }

   switch (transition)
   {
      case GST_STATE_CHANGE_NULL_TO_READY:
      {
	//TODO
         break;
      }

      case GST_STATE_CHANGE_READY_TO_PAUSED:
      {
	//TODO
         break;
      }

      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      {
	//TODO
         break;
      }

      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      {
	//TODO
         break;
      }

      case GST_STATE_CHANGE_PAUSED_TO_READY:
      {
	//TODO
         break;
      }

      case GST_STATE_CHANGE_READY_TO_NULL:
      {
	//TODO
         break;
      }

      default:
	//TODO
         break;
   }
}

static GstCaps *gst_westeros_video_sink_get_caps (GstBaseSink * bsink, GstCaps * filter)
{
  GstwesterosVideoSink *sink;
  GstCaps *sink_caps;

  sink = GST_WESTEROSVIDEOSINK (bsink);

  sink_caps = gst_pad_get_pad_template_caps (GST_VIDEO_SINK_PAD (sink));
  if (filter) 
  {
    GstCaps *intersectCaps = gst_caps_intersect_full (filter, sink_caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (sink_caps);
    sink_caps = intersectCaps;
  }
  return sink_caps;
}

static uint32_t gst_westeros_video_format_to_wos_format (GstVideoFormat gstFormat)
{
  guint index;

  for (index = 0; index < G_N_ELEMENTS (wos_video_formats); index++)
    if (wos_video_formats[index].gst_format == gstFormat)
      return wos_video_formats[index].wos_format;

  GST_WARNING ("westeros video format not found");
  return -1;
}



static gboolean gst_westeros_video_sink_format_from_caps (uint32_t * wos_format, GstCaps * caps)
{
  GstStructure *structure;
  GstVideoFormat gstFormat = GST_VIDEO_FORMAT_UNKNOWN;
  const gchar *format = NULL;

  structure = gst_caps_get_structure (caps, 0);
  format = gst_structure_get_string (structure, "format");
  gstFormat = gst_video_format_from_string (format);

  *wos_format = gst_westeros_video_format_to_wos_format (gstFormat);

  return (*wos_format != -1);
}


#ifndef GST_DISABLE_GST_DEBUG
static const gchar *gst_westeros_video_format_to_string (uint32_t wos_format)
{
  guint i;
  GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;

  for (i = 0; i < G_N_ELEMENTS (wos_video_formats); i++)
    if (wos_video_formats[i].wos_format == wos_format)
      format = wos_video_formats[i].gst_format;

  return gst_video_format_to_string (format);
}
#endif


static gboolean gst_westeros_video_sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstwesterosVideoSink *sink = GST_WESTEROSVIDEOSINK (bsink);
  static GstAllocationParams params = { 0, 0, 0, 15, };
  GstVideoInfo gst_videoInfo;
  GstStructure *gst_structure;
  guint size;
  GstBufferPool *newbufferpool, *oldbufferpool;

  sink = GST_WESTEROSVIDEOSINK (bsink);

  GST_LOG_OBJECT (sink, "set caps %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&gst_videoInfo, caps))
    goto invalid_video_format;

  sink->VWidth = gst_videoInfo.width;
  sink->VHeight = gst_videoInfo.height;
  size = gst_videoInfo.size;


  if (!gst_westeros_video_sink_format_from_caps (&sink->format, caps))
    goto invalid_video_format;

  if (!(sink->window->wos_formats & (1 << sink->format))) 
  {
    GST_DEBUG_OBJECT (sink, "%s not available", gst_westeros_video_format_to_string (sink->format));
    return FALSE;
  }

  /* create a new pool for the new configuration */
  newbufferpool = gst_westeros_buffer_pool_new (sink);

  if (newbufferpool == NULL) 
  {
    GST_DEBUG_OBJECT (sink, "Failed to create new pool");
    return FALSE;
  }


  gst_structure = gst_buffer_pool_get_config (newbufferpool);
  gst_buffer_pool_config_set_params (gst_structure, caps, size, 2, 0);
  gst_buffer_pool_config_set_allocator (gst_structure, NULL, &params);
  if (!gst_buffer_pool_set_config (newbufferpool, gst_structure))
    goto config_failed;

  oldbufferpool = sink->pool;
  sink->pool = newbufferpool;
  if (oldbufferpool)
    gst_object_unref (oldbufferpool);

  return TRUE;

config_failed:
  {
    GST_DEBUG_OBJECT (bsink, "failed to set config");
    return FALSE;
  }
invalid_video_format:
  {
    GST_DEBUG_OBJECT (sink,"Could not find video format from caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }

}


static gboolean gst_westeros_video_sink_propose_allocation (GstBaseSink * bsink, GstQuery * query)
{
  GstwesterosVideoSink *sink = GST_WESTEROSVIDEOSINK (bsink);
  GstBufferPool *gst_bufferpool;
  GstStructure *gst_config;
  GstCaps *caps;
  guint buffer_size;
  gboolean pool_wanted;

  gst_query_parse_allocation (query, &caps, &pool_wanted);

  if (caps == NULL)
    goto no_caps;

  g_mutex_lock (&sink->wos_lock);

  if ((gst_bufferpool = sink->pool))
    gst_object_ref (gst_bufferpool);

  g_mutex_unlock (&sink->wos_lock);

  if (gst_bufferpool != NULL) 
   {
    GstCaps *pcaps;

    gst_config = gst_buffer_pool_get_config (gst_bufferpool);
    gst_buffer_pool_config_get_params (gst_config, &pcaps, &buffer_size, NULL, NULL);

      /*check whether different caps*/
    if (!gst_caps_is_equal (caps, pcaps)) 
    {
      gst_object_unref (gst_bufferpool);
      gst_bufferpool = NULL;
    }
    gst_structure_free (gst_config);
  }

  if (gst_bufferpool == NULL && pool_wanted) 
  {
    GST_DEBUG_OBJECT (sink, "create new pool");
    GstVideoInfo gst_Videoinfo;

    if (!gst_video_info_from_caps (&gst_Videoinfo, caps))
    {
      GST_DEBUG_OBJECT (sink, "Invalid Capabilities");
      goto invalid_caps;
    }
    gst_bufferpool = gst_westeros_buffer_pool_new (sink);

    buffer_size = gst_Videoinfo.size;

    gst_config = gst_buffer_pool_get_config (gst_bufferpool);
    gst_buffer_pool_config_set_params (gst_config, caps, buffer_size, 2, 0);
    if (!gst_buffer_pool_set_config (gst_bufferpool, gst_config))
     {
      GST_DEBUG_OBJECT (sink, "Setting Config Failed");
      goto configuration_failed;
    }
  }
 if (gst_bufferpool) 
  {
    gst_query_add_allocation_pool (query, gst_bufferpool, buffer_size, 2, 0);
    gst_object_unref (gst_bufferpool);
  }

  return TRUE;
configuration_failed:
  {
    GST_DEBUG_OBJECT (bsink, "failed setting config");
    gst_object_unref (gst_bufferpool);
    return FALSE;
  }
no_caps:
  {
    GST_DEBUG_OBJECT (bsink, "no caps specified");
    return FALSE;
  }
invalid_caps:
  {
    GST_DEBUG_OBJECT (bsink, "invalid caps specified");
    return FALSE;
  }

}


static void wos_shm_format (void *data, struct wl_shm *wos_shm, uint32_t gst_format)
{
  wDisplay *d = data;
  d->wos_formats |= (1 << gst_format);
}

struct wl_shm_listener wos_shm_listenter = { wos_shm_format };



static void registry_handle_global (void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version)
{
  wDisplay *d = data;

  if (strcmp (interface, "wl_compositor") == 0) {
    d->wos_compositor = wl_registry_bind (registry, id, &wl_compositor_interface, 1);

  } else if (strcmp (interface, "wl_shell") == 0) {
    d->wos_shell = wl_registry_bind (registry, id, &wl_shell_interface, 1);

  } else if (strcmp (interface, "wl_shm") == 0) {
    d->wos_shm = wl_registry_bind (registry, id, &wl_shm_interface, 1);
    wl_shm_add_listener (d->wos_shm, &wos_shm_listenter, d);
  }
}



static const struct wl_registry_listener wos_registry_listener = { registry_handle_global };

static wDisplay* create_wos_display(GstwesterosVideoSink *sink)
{

  wDisplay* display = NULL;
//Display Params
  display = malloc (sizeof (wDisplay));
  display->wos_display = wl_display_connect (NULL);

  if (display->wos_display == NULL) {
    free (display->wos_display);
    return NULL;
  }

  display->wos_registry = wl_display_get_registry (display->wos_display);
  wl_registry_add_listener (display->wos_registry, &wos_registry_listener, display);

  wl_display_roundtrip (display->wos_display);
  if (display->wos_shm == NULL) {
    GST_ERROR ("No wl_shm global..");
    return NULL;
  }

  wl_display_roundtrip (display->wos_display);

  wl_display_get_fd (display->wos_display);

 
  return display;
}

static gboolean gst_westeros_video_sink_start(GstBaseSink *base_sink)
{
  GstwesterosVideoSink *sink = (GstwesterosVideoSink *) base_sink;
  gboolean result = TRUE;

  GST_DEBUG_OBJECT (sink, "start");


  if (!sink->window)
    sink->window = create_wos_display (sink);

  if (sink->window == NULL) {
    GST_ELEMENT_ERROR (base_sink, RESOURCE, OPEN_READ_WRITE,
        ("Could not initialise Westeros output"),
        ("Could not create Westeros display"));
    return FALSE;
  }

  return result;
}

static gboolean gst_westeros_video_sink_stop(GstBaseSink *base_sink)
{
  GstwesterosVideoSink *sink = (GstwesterosVideoSink*) base_sink;
  gboolean result = TRUE;

  GST_DEBUG_OBJECT (sink, "stop");
   
   //TODO

   return TRUE;
}




static void ping_handler (void *data, struct wl_shell_surface *wos_shell_surface, uint32_t serial)
{
  wl_shell_surface_pong (wos_shell_surface, serial);
}


static const struct wl_shell_surface_listener wos_shell_surface_listener = {
  ping_handler,
};

static void create_wos_window (GstwesterosVideoSink * sink, int width, int height)
{
  g_mutex_lock (&sink->wos_lock);

  wDisplay *display = sink->window;
  display->width = width;
  display->height = height;
  display->redraw_check = FALSE;


  display->wos_surface = wl_compositor_create_surface (display->wos_compositor);
  display->wos_shell_surface = wl_shell_get_shell_surface (display->wos_shell,display->wos_surface);
  g_return_if_fail (display->wos_shell_surface);
  wl_shell_surface_add_listener (display->wos_shell_surface,&wos_shell_surface_listener, display);
  wl_shell_surface_set_toplevel (display->wos_shell_surface);


  g_mutex_unlock (&sink->wos_lock);
}


static void
frame_redraw_callback_handler (void *data, struct wl_callback *wos_callback, uint32_t time)
{
  wDisplay *window = (wDisplay *) data;
  window->redraw_check = FALSE;
  wl_callback_destroy (wos_callback);
}

static const struct wl_callback_listener wos_frame_callback_listener = {
  frame_redraw_callback_handler
};


static GstFlowReturn gst_westeros_video_sink_render(GstBaseSink *base_sink, GstBuffer *gst_buffer)
{
  GstwesterosVideoSink *sink= GST_WESTEROSVIDEOSINK(base_sink);
  wDisplay *window = sink->window;
  GstVideoRectangle source, dest, final;
  GstBuffer *to_render;
  GstWosMetaData *wos_metadata;
  GstFlowReturn ret;
  GType type;

  static bool flag = true;
  if ((sink->window && flag == true))
 {
  	create_wos_window(sink,sink->VWidth, sink->VHeight);
	flag = false;
  }
  type = gst_wos_metadata_get_type ();
  wos_metadata = (GstWosMetaData*)gst_buffer_get_meta(gst_buffer,type);

  if (window->redraw_check) {
    wl_display_dispatch (window->wos_display);
  }

  if (wos_metadata && wos_metadata->sink == sink) 
  {
    GST_LOG_OBJECT (sink, "buffer %p from our pool, writing directly", gst_buffer);
    to_render = gst_buffer;
  } 
  else 
  {
    if (!sink->pool)
      goto pool_unavailable;

    GstMapInfo src;
    GST_LOG_OBJECT (sink, "buffer %p not from our pool, copying", gst_buffer);


    if (!gst_buffer_pool_set_active (sink->pool, TRUE))
      goto buffer_activate_failed;

    if((ret = gst_buffer_pool_acquire_buffer (sink->pool, &to_render, NULL)) != GST_FLOW_OK);
        goto buffer_unavailable;

    gst_buffer_map (gst_buffer, &src, GST_MAP_READ);
    gst_buffer_fill (to_render, 0, src.data, src.size);
    gst_buffer_unmap (gst_buffer, &src);

    wos_metadata = gst_buffer_get_wos_metadata(to_render);//note
  }

  dest.w = sink->window->width;
  dest.h = sink->window->height;
  source.w = sink->VWidth;
  source.h = sink->VHeight;

//note
  wl_surface_attach (sink->window->wos_surface, wos_metadata->wos_buffer, 0, 0);

  gst_video_sink_center_rect (source, dest, &final, FALSE);
  wl_surface_damage (sink->window->wos_surface, 0, 0, final.w, final.h);
  window->redraw_check = TRUE;
  window->wos_callback = wl_surface_frame (window->wos_surface);
  wl_callback_add_listener (window->wos_callback, &wos_frame_callback_listener, window);
  wl_surface_commit (window->wos_surface);
  wl_display_dispatch (window->wos_display);

  if (gst_buffer != to_render)
    gst_buffer_unref (to_render);
  return GST_FLOW_OK;

buffer_unavailable:
  {
    GST_WARNING_OBJECT (sink, "could not create image");
    return ret;
  }
pool_unavailable:
  {
    GST_ERROR_OBJECT (sink,"We don't have a bufferpool negotiated");
    return GST_FLOW_ERROR;
  }
buffer_activate_failed:
  {
    GST_ERROR_OBJECT (sink, "failed to activate bufferpool.");
    return GST_FLOW_ERROR;
  }

}

static GstFlowReturn gst_westeros_video_sink_preroll(GstBaseSink *base_sink, GstBuffer *buffer)
{
   GstwesterosVideoSink *sink= GST_WESTEROSVIDEOSINK(base_sink);
   GST_DEBUG_OBJECT(sink, "gst_westeros_video_sink_preroll : %p", buffer);
   return  gst_westeros_video_sink_render (base_sink, buffer);;

}


static gboolean gst_westeros_video_sink_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstwesterosVideoSink *filter;
  gboolean ret;

  filter = GST_WESTEROSVIDEOSINK (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}



static gboolean gst_westeros_video_sink_query(GstElement *element, GstQuery *query)
{

   switch (GST_QUERY_TYPE(query))
   {
      case GST_QUERY_LATENCY:
	{
         gst_query_set_latency(query, FALSE, 0, 10*1000*1000);
         return TRUE;
	}

      case GST_QUERY_POSITION:
         {
            GstFormat format;

            gst_query_parse_position(query, &format, NULL);

            if ( GST_FORMAT_BYTES == format )
            {
               return GST_ELEMENT_CLASS(parent_class)->query(element, query);
            }
            else
            {
        //TODO
               return TRUE;
            }
         }
         break;

      case GST_QUERY_CUSTOM:
      case GST_QUERY_DURATION:
      case GST_QUERY_SEEKING:
      case GST_QUERY_RATE:
        //TODO

      default:
         return GST_ELEMENT_CLASS(parent_class)->query (element, query);
   }
}


static GstFlowReturn gst_westeros_video_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstwesterosVideoSink *sink = GST_WESTEROSVIDEOSINK (parent);

}
static gboolean westerosvideosink_init (GstPlugin * westerosvideosink)
{
  GST_DEBUG_CATEGORY_INIT (gst_westeros_video_sink_debug, "westerosvideosink", 0, "Westerosvideosink");
  return gst_element_register (westerosvideosink, "westerosvideosink", GST_RANK_MARGINAL, GST_TYPE_WESTEROSVIDEOSINK);
}


GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    westerosvideosink,
    "westerosvideosink",
    westerosvideosink_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
