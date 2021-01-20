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

cmd_1_list  = [0x0A,0x04]  # set baudrate 115200

cmd_crc=CalculateCrc(cmd_1_list) # Calculate the CRC over the list of command bytes

print("Initial baudrate 9600, now set to 115200")
print("cmd_1_list",cmd_1_list) # Print the command to the screen
print("CRC from cmd_1_list=",hex(cmd_crc)) # Print the CRC to the screen
cmd_1_list.append(cmd_crc) # Append the crc to the command
print("cmd_1_list with CRC byte added",cmd_1_list) # print the command with crc appended

ser.write(serial.to_bytes(cmd_1_list)) # Write the command to the HVC, including CRC byte at the end

answer = ser.read(3)

print("\nHVC reply \nCMD   = ",answer[0:1]) # Print the reply
print("MOT= ",answer[1:2])
print("CRC   = ",answer[2:3])

crc_check=CalculateCrc(answer[:2])
crc_check=struct.pack("B", crc_check&0xFF)
#print("CRCc  = ",(crc_check))

if crc_check == answer[2:3]:
   print("CRC check OK")
else:
   print("CRC check FAIL")


print("Finish")

ser.close() # Close serial port


ser = serial.Serial(None,115200,bytesize =  serial.EIGHTBITS,parity =    serial.PARITY_EVEN,stopbits =  serial.STOPBITS_ONE, timeout =   0.5)
ser.port='COM4'   # add your com port here
ser.open()# Open serial port 

cmd_1_list  = [0x08,0x01]  # get motor 1 diagnostics

cmd_crc=CalculateCrc(cmd_1_list) # Calculate the CRC over the list of command bytes
print("\n\nCOM port running at 115200 baud")
print("get motor 1 diagnostics")
print("cmd_1_list",cmd_1_list) # Print the command to the screen
print("CRC from cmd_1_list=",hex(cmd_crc)) # Print the CRC to the screen
cmd_1_list.append(cmd_crc) # Append the crc to the command
print("cmd_1_list with CRC byte added",cmd_1_list) # print the command with crc appended

ser.write(serial.to_bytes(cmd_1_list)) # Write the command to the HVC, including CRC byte at the end

answer = ser.read(8)

temperature = int.from_bytes(answer[2:4], "little", signed=True)
voltage = int.from_bytes(answer[4:6], "little")
print("\nHVC reply \nCMD   = ",answer[0:1]) # Print the reply
print("MOT= ",answer[1:2])
print("Temperature [°C]= ",temperature)
print("Voltage [mV]= ",voltage)
print("Errors =",answer[6:7])
print("CRC   = ",answer[7:8])

crc_check=CalculateCrc(answer[:7])
crc_check=struct.pack("B", crc_check&0xFF)
#print("CRCc  = ",(crc_check))

if crc_check == answer[7:8]:
   print("CRC check OK")
else:
   print("CRC check FAIL")



cmd_1_list  = [0x08,0x02]  # get motor diagnostics

cmd_crc=CalculateCrc(cmd_1_list) # Calculate the CRC over the list of command bytes
print("\n\nget motor 2 diagnostics")
print("cmd_1_list",cmd_1_list) # Print the command to the screen
print("CRC from cmd_1_list=",hex(cmd_crc)) # Print the CRC to the screen
cmd_1_list.append(cmd_crc) # Append the crc to the command
print("cmd_1_list with CRC byte added",cmd_1_list) # print the command with crc appended

ser.write(serial.to_bytes(cmd_1_list)) # Write the command to the HVC, including CRC byte at the end

answer = ser.read(8)

temperature = int.from_bytes(answer[2:4], "little", signed=True)
voltage = int.from_bytes(answer[4:6], "little")

print("\nHVC reply \nCMD   = ",answer[0:1]) # Print the reply
print("MOT= ",answer[1:2])
print("Temperature [°C]= ",temperature)
print("Voltage [mV]= ",voltage)
print("Errors =",answer[6:7])
print("CRC   = ",answer[7:8])

crc_check=CalculateCrc(answer[:7])
crc_check=struct.pack("B", crc_check&0xFF)
#print("CRCc  = ",(crc_check))

if crc_check == answer[7:8]:
   print("CRC check OK")
else:
   print("CRC check FAIL")

print("Finish")

ser.close() # Close serial port
