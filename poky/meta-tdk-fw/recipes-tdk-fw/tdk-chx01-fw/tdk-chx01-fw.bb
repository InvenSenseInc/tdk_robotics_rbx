DESCRIPTION = "TDK chx01 firmware application for Yocto build."
SECTION = "firmware"
DEPENDS = ""

LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/${LICENSE};md5=3775480a712fc46a69647678acb234cb"

#LICENSE = "TDK"
#LIC_FILES_CHKSUM = "file://${WORKDIR}/TDK_COPYING;md5=fa003d73b8ff4fa28086087276f380bd"

FILESEXTRAPATHS_prepend := "${THISDIR}/src:"

FILES_${PN} = " ${datadir} \
	        ${bindir} \
	      "

SRC_URI = " file://tdk-chx01-fw.c \
	    file://ch101.h \
	    file://firmware/v39.hex \
	    file://firmware/v39_IQ_debug-02.hex \
	    file://firmware/ch101_gpr_rxopt_v41b.bin \
	    file://firmware/ch101_gpr_rxopt_v41b-IQ_Debug.bin \
	    file://firmware/ch201_gprmt_v10a.bin \
	    file://firmware/ch201_gprmt_v10a-IQ_Debug.bin \
	    file://firmware/ch201_old.bin \
	  "

S = "${WORKDIR}"

do_compile() {
${CC} ${CFLAGS} ${LDFLAGS} ${WORKDIR}/tdk-chx01-fw.c -o tdk-chx01-fw
}

do_install() {
# create the /usr/bin folder in the rootfs with default permissions
install -d ${D}${bindir}
install -d ${D}${datadir}/tdk

# install the application into the /usr/bin folder and hex file to /usr/share with default permissions
install -m 755 ${WORKDIR}/tdk-chx01-fw ${D}${bindir}
install -m 644 ${WORKDIR}/firmware/v39.hex ${D}${datadir}/tdk
install -m 644 ${WORKDIR}/firmware/v39_IQ_debug-02.hex ${D}${datadir}/tdk
install -m 644 ${WORKDIR}/firmware/ch101_gpr_rxopt_v41b.bin ${D}${datadir}/tdk
install -m 644 ${WORKDIR}/firmware/ch101_gpr_rxopt_v41b-IQ_Debug.bin ${D}${datadir}/tdk
install -m 644 ${WORKDIR}/firmware/ch201_gprmt_v10a.bin ${D}${datadir}/tdk
install -m 644 ${WORKDIR}/firmware/ch201_gprmt_v10a-IQ_Debug.bin ${D}${datadir}/tdk
install -m 644 ${WORKDIR}/firmware/ch201_old.bin ${D}${datadir}/tdk
}
