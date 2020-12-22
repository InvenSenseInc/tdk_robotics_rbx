DESCRIPTION = "TDK Micronas Motor controller Scripts"
SECTION = "Motor Controller scripts"
DEPENDS = ""
LICENSE = "Apache-2.0"

LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"

PR = "r0"
RV = "1.0"

FILES_${PN} = "${datadir}"

SRC_URI = "file://commands/CMD1_All_off.py"
SRC_URI += "file://commands/CMD1_Both_on.py"
SRC_URI += "file://commands/CMD2_set_hold.py"
SRC_URI += "file://commands/CMD3_set_128mA.py"
SRC_URI += "file://commands/CMD4_get_speed.py"
SRC_URI += "file://commands/CMD5_get_motor_current.py"
SRC_URI += "file://commands/CMD6_get_revolutions.py"
SRC_URI += "file://commands/CMD6_reset_revolutions.py"
SRC_URI += "file://commands/CMD8_get_motor_diagnostics.py"
SRC_URI += "file://commands/CMD9_reset_errros.py"
SRC_URI += "file://commands/CMD9_set_baudrate_115200_to_9600.py"
SRC_URI += "file://commands/CMD9_set_baudrate_9600_to_115200.py"
SRC_URI += "file://commands/crc8-serial-test-rb3_v1.py"
SRC_URI += "file://demos/drive_square.py"
SRC_URI += "file://demos/rotate.py"
SRC_URI += "file://Modifications_on_MezzA_board.txt"
SRC_URI += "file://HVC4223F_UART_IF.xlsx"

S = "${WORKDIR}"

do_install() {
# create the /usr/share/tdk/motor_controller folder in the rootfs with default permissions
install -d ${D}${datadir}
install -d ${D}${datadir}/tdk
install -d ${D}${datadir}/tdk/motor_controller

# install script files to /usr/share/tdk/motor_controller with default permissions
install -m 644 ${WORKDIR}/commands/CMD1_All_off.py ${D}${datadir}/tdk/motor_controller        
install -m 644 ${WORKDIR}/commands/CMD1_Both_on.py ${D}${datadir}/tdk/motor_controller                   
install -m 644 ${WORKDIR}/commands/CMD2_set_hold.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD3_set_128mA.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD4_get_speed.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD5_get_motor_current.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD6_get_revolutions.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD6_reset_revolutions.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD8_get_motor_diagnostics.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD9_reset_errros.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD9_set_baudrate_115200_to_9600.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/CMD9_set_baudrate_9600_to_115200.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/commands/crc8-serial-test-rb3_v1.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/demos/drive_square.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/demos/rotate.py ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/Modifications_on_MezzA_board.txt ${D}${datadir}/tdk/motor_controller
install -m 644 ${WORKDIR}/HVC4223F_UART_IF.xlsx ${D}${datadir}/tdk/motor_controller
}
