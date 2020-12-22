inherit autotools pkgconfig
DESCRIPTION = "TDK Chirp Application"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"

PR = "r0"
RV = "1.0"

#DEPENDS = "liblog"
DEPENDS += "libcutils"
DEPENDS += "libutils"
DEPENDS += "common-headers"
DEPENDS += "glib-2.0"

LDFLAGS += "-lm -L${WORKDIR} -Wl,-Bstatic -lInvnAlgoRangeFinder -Wl,-Bdynamic -lgcc_s"

PACKAGES = "${PN}"

FILES_${PN} += "/usr/local/bin/*"

FILESPATH =+ "${WORKSPACE}:"
SRC_URI = "file://tdk-chx01-get-data.c"
SRC_URI += "file://Makefile.am"
SRC_URI += "file://configure.ac"
SRC_URI += "file://invn_algo_rangefinder.h"
SRC_URI += "file://invn/common/invn_types.h"
SRC_URI += "file://libInvnAlgoRangeFinder.a"

S = "${WORKDIR}"

do_install() {
    install -m 0755 ${S}/build/tdk-chx01-get-data-app -D ${D}/usr/local/bin/tdk-chx01-get-data-app
}

#do_package_qa[noexec] = "1"

