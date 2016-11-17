#
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
# Copyright 2016 L&T Technology Services
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

SUBDIRS = 
AM_CPPFLAGS = -pthread -Wall -I./
plugin_LTLIBRARIES = libgstwesterossinkbin.la
libgstwesterossinkbin_la_SOURCES =  gstwesterosvideosink.c  westerosbufferpool.c
libgstwesterossinkbin_la_CFLAGS =  $(GST_CFLAGS)
libgstwesterossinkbin_la_LDFLAGS = $(GST_LIBS) 
libgstwesterossinkbin_la_LIBADD = -lwayland-client -lgstvideo-1.0 -lgstreamer-1.0 -lgstbase-1.0 -lwesteros_simplebuffer_client -lwesteros_simpleshell_client -lgstvideo-1.0 -ldl
libgstwesterossinkbin_la_LDFLAGS += -module -avoid-version
libgstwesterossinkbin_la_LIBTOOLFLAGS = --tag=disable-static
