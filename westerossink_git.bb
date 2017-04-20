SUMMARY = "Westeros plugins implementation for RDK emulator"
DESCRIPTION = "Westeros sink plugin  "

SECTION = "console/utils"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://../../LICENSE;beginline=1;endline=201"

PR = "r0"
PV = "1.0"
SRCREV = "${AUTOREV}"


SRC_URI ="git://github.com/kalyan-nagabhirava/westerossink.git;branch=master"


S = "${WORKDIR}/git/gst-westerosvideosink/src/"

FILES_${PN} += "${libdir}/gstreamer-*/*.so"
FILES_${PN}-dev += "${libdir}/gstreamer-*/*.la"
FILES_${PN}-dbg += "${libdir}/gstreamer-*/.debug/*"
FILES_${PN}-staticdev += "${libdir}/gstreamer-*/*.a "

DEPENDS = "${@base_contains('DISTRO_FEATURES', 'gstreamer1', 'gstreamer1.0', 'gstreamer', d)}"

ENABLE_GST1 = "--enable-gstreamer1=${@base_contains('DISTRO_FEATURES', 'gstreamer1', 'yes', 'no', d)}"
EXTRA_OECONF += "${ENABLE_GST1}"

inherit autotools pkgconfig
