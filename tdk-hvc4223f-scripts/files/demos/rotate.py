################################################################################
# File:         .py
# Date:         2020.11.30
# Author:       HS
#
# Description:  
#
################################################################################
import serial # Import pySerial for serial communication
import struct
import time

ser = serial.Serial(None,9600,bytesize =  serial.EIGHTBITS,parity =    serial.PARITY_EVEN,stopbits =  serial.STOPBITS_ONE, timeout =   0.5)

# CRC-8 calculation Maxim / Dallas
def Crc8(b, crc):
    b2 = b
    
    if (b < 0): b2 = b + 256
    for i in range(8):
        odd = ((b2^crc) & 1) == 1
        crc >>= 1
        b2 >>= 1
        if (odd): crc ^= 0x8C 
    return crc

# CalculateCrc calls Crc8 (Dallas/Maxim crc) for one byte 
# A list of hex numbers is passed as parameter  
def CalculateCrc(hex_liste):
    i=0
    crc_i = 0
    for i in hex_liste: crc_i=Crc8(i,crc_i)
    return crc_i    
    


ser.port='COM4'   # add your com port here
ser.open()# Open serial port 

### define the used commands ###
cmd_both_off = [0x01,0x03, 0x00, 0x00, 0x00]  # all off: speed=0, offset=0
cmd_both_off.append(CalculateCrc(cmd_both_off))

cmd_both_fwd_8000_offset_0 = [0x01,0x03, 0x40, 0x1F, 0x00]  # FWD: speed=8000 rpm, offset=0
cmd_both_fwd_8000_offset_0.append(CalculateCrc(cmd_both_fwd_8000_offset_0))

cmd_both_fwd_8000_offset_100 = [0x01,0x03, 0x40, 0x1F, 0x64]  # FWD: speed=8000 rpm, offset=100
cmd_both_fwd_8000_offset_100.append(CalculateCrc(cmd_both_fwd_8000_offset_100))

cmd_both_fwd_8000_offset_50 = [0x01,0x03, 0x40, 0x1F, 0x32]  # FWD: speed=8000 rpm, offset=100
cmd_both_fwd_8000_offset_50.append(CalculateCrc(cmd_both_fwd_8000_offset_50))

cmd_both_fwd_8000_offset_minus_100 = [0x01,0x03, 0x70, 0x17, 0x9C]  # FWD: speed=8000 rpm, offset=-100
cmd_both_fwd_8000_offset_minus_100.append(CalculateCrc(cmd_both_fwd_8000_offset_minus_100))

cmd_both_bwd_8000_offset_100 = [0x01,0x03, 0xC0, 0xE0, 0x64]  # BWD: speed=-8000 rpm, offset=100
cmd_both_bwd_8000_offset_100.append(CalculateCrc(cmd_both_bwd_8000_offset_100))

cmd_both_bwd_8000_offset_50 = [0x01,0x03, 0xC0, 0xE0, 0x32]  # BWD: speed=-8000 rpm, offset=50
cmd_both_bwd_8000_offset_50.append(CalculateCrc(cmd_both_bwd_8000_offset_50))

cmd_both_bwd_8000_offset_minus_50 = [0x01, 0x03, 0xC0, 0xE0, 0xCE]  # BWD: speed=-8000 rpm, offset=-50
cmd_both_bwd_8000_offset_minus_50.append(CalculateCrc(cmd_both_bwd_8000_offset_minus_50))

cmd_both_set_current_limit_200mA = [0x03, 0x03, 0xC8, 0x00]  # both motors, 200 mA
cmd_both_set_current_limit_200mA.append(CalculateCrc(cmd_both_set_current_limit_200mA))

cmd_both_set_current_limit_300mA = [0x03, 0x03, 0x2C, 0x01]  # both motors, 300 mA
cmd_both_set_current_limit_300mA.append(CalculateCrc(cmd_both_set_current_limit_300mA))

cmd_mot1_fwd_8000_offset_0 = [0x01,0x01, 0x40, 0x1F, 0x00]  # FWD: speed=8000 rpm, offset=0
cmd_mot1_fwd_8000_offset_0.append(CalculateCrc(cmd_mot1_fwd_8000_offset_0))

cmd_mot2_fwd_8000_offset_0 = [0x01,0x02, 0x40, 0x1F, 0x00]  # FWD: speed=8000 rpm, offset=0
cmd_mot2_fwd_8000_offset_0.append(CalculateCrc(cmd_mot2_fwd_8000_offset_0))

cmd_mot1_bwd_8000_offset_0 = [0x01,0x01, 0xC0, 0xE0, 0x00]  # BWD: speed=-8000 rpm, offset=0
cmd_mot1_bwd_8000_offset_0.append(CalculateCrc(cmd_mot1_bwd_8000_offset_0))

cmd_mot2_bwd_8000_offset_0 = [0x01,0x02, 0xC0, 0xE0, 0x00]  # BWD: speed=-8000 rpm, offset=0
cmd_mot2_bwd_8000_offset_0.append(CalculateCrc(cmd_mot2_bwd_8000_offset_0))

cmd_both_reset_errors = [0x09,0x03]  # reset errors
cmd_both_reset_errors.append(CalculateCrc(cmd_both_reset_errors))


#### demonstration sequence
##print("Start")
##ser.write(serial.to_bytes(cmd_both_reset_errors))
##time.sleep(1)
##ser.write(serial.to_bytes(cmd_both_off))
##time.sleep(0.1)
##ser.write(serial.to_bytes(cmd_both_set_current_limit_200mA))
##time.sleep(0.1)
##ser.write(serial.to_bytes(cmd_both_fwd_8000_offset_50))
##time.sleep(26)
##ser.write(serial.to_bytes(cmd_both_off))
##print("Finish")

##print("Start")
##ser.write(serial.to_bytes(cmd_both_reset_errors))
##time.sleep(1)
##ser.write(serial.to_bytes(cmd_both_off))
##time.sleep(0.1)
##ser.write(serial.to_bytes(cmd_both_set_current_limit_200mA))
##time.sleep(0.1)
##ser.write(serial.to_bytes(cmd_both_bwd_8000_offset_50))
##time.sleep(26)
##ser.write(serial.to_bytes(cmd_both_off))
##print("Finish")

print("Start")
ser.write(serial.to_bytes(cmd_both_reset_errors))
time.sleep(1)
ser.write(serial.to_bytes(cmd_both_off))
time.sleep(0.1)
ser.write(serial.to_bytes(cmd_both_set_current_limit_200mA))
time.sleep(0.1)
ser.write(serial.to_bytes(cmd_mot2_bwd_8000_offset_0))
time.sleep(0.1)
ser.write(serial.to_bytes(cmd_mot1_fwd_8000_offset_0))
time.sleep(10)
ser.write(serial.to_bytes(cmd_both_off))

print("And back")
ser.write(serial.to_bytes(cmd_both_reset_errors))
time.sleep(1)
ser.write(serial.to_bytes(cmd_both_off))
time.sleep(0.1)
ser.write(serial.to_bytes(cmd_both_set_current_limit_200mA))
time.sleep(0.1)
ser.write(serial.to_bytes(cmd_mot1_bwd_8000_offset_0))
time.sleep(0.1)
ser.write(serial.to_bytes(cmd_mot2_fwd_8000_offset_0))
time.sleep(10)
ser.write(serial.to_bytes(cmd_both_off))

print("Finish")

ser.close() # Close serial port
