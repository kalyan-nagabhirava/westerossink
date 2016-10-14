
SUBDIRS = 
AM_CPPFLAGS = -pthread -Wall -I./
plugin_LTLIBRARIES = libgstwesterossinkbin.la
libgstwesterossinkbin_la_SOURCES =  gstwesterosvideosink.c  westerosbufferpool.c
libgstwesterossinkbin_la_CFLAGS =  $(GST_CFLAGS)
libgstwesterossinkbin_la_LDFLAGS = $(GST_LIBS) 
libgstwesterossinkbin_la_LIBADD = -lwayland-client -lgstvideo-1.0 -lgstreamer-1.0 -lgstbase-1.0 -lwesteros_simplebuffer_client -lwesteros_simpleshell_client -lgstvideo-1.0 -ldl
libgstwesterossinkbin_la_LDFLAGS += -module -avoid-version
libgstwesterossinkbin_la_LIBTOOLFLAGS = --tag=disable-static
