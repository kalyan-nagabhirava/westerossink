plugin_LTLIBRARIES = libgstwesterosvideosink.la 

libgstwesterosvideosink_la_SOURCES = gstwesterosvideosink.c westerosbufferpool.c
libgstwesterosvideosink_la_CFLAGS = $(GST_CFLAGS)
libgstwesterosvideosink_la_LIBADD = $(GST_LIBS) -lwayland-client -lgstvideo-1.0 -lgstreamer-1.0 -lgstbase-1.0 -lwesteros_simplebuffer_client -lwesteros_simpleshell_client -lgstvideo-1.0 -ldl
libgstwesterosvideosink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstwesterosvideosink_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gstwesterosvideosink.h westerosbufferpool.h
