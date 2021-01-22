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

cmd_1_list  = [0x06,0x01]  # mot 1 get motor speed in electrical revolutions per minute [rpm]

cmd_crc=CalculateCrc(cmd_1_list) # Calculate the CRC over the list of command bytes

print("cmd_1_list",cmd_1_list) # Print the command to the screen
print("CRC from cmd_1_list=",hex(cmd_crc)) # Print the CRC to the screen
cmd_1_list.append(cmd_crc) # Append the crc to the command
print("cmd_1_list with CRC byte added",cmd_1_list) # print the command with crc appended

ser.write(serial.to_bytes(cmd_1_list)) # Write the command to the HVC, including CRC byte at the end

answer = ser.read(7)

revolutions = int.from_bytes(answer[2:6], "little")

print("\nHVC reply \nCMD   = ",answer[0:1]) # Print the reply
print("MOT= ",answer[1:2])
print("Revolutions= ",revolutions)
print("CRC   = ",answer[6:7])

crc_check=CalculateCrc(answer[:6])
crc_check=struct.pack("B", crc_check&0xFF)
#print("CRCc  = ",(crc_check))

if crc_check == answer[6:7]:
   print("CRC check OK")
else:
   print("CRC check FAIL")


print("Finish")

ser.close() # Close serial port
