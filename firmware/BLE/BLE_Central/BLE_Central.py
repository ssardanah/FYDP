from RPLCD import CharLCD
from RPi import GPIO
import binascii
import struct
import time
from bluepy.btle import UUID, Peripheral

GPIO.setwarnings(False)

detection_UUID = UUID(0x180C)
p = Peripheral('f0:24:d3:40:e2:00', 'random')

lcd = CharLCD(numbering_mode=GPIO.BOARD, cols=16, rows=2, pin_rs=37, pin_e=35,pins_data=[33, 31, 29, 23])


try:
    p.connect('f0:24:d3:40:e2:00', 'random')
    ch = p.getCharacteristic(uuid = detection_UUID)[0]
    if(ch.supportsRead()):
        while 1:
            vesselPresence = binascii.b2a_hex(ch.read())
            vesselPresence = binascii.unhexlify(vesselPresence)
            vesselPresence = struct.unpack('f',vesselPresence)[0]
            display = "Detected: " + str(vesselPresence)
            print(display)
            lcd.write_string(display)
            lcd.cursor_pos = (1,3)
finally:
    p.disconnect()

