###################################################################
## Script listens to serial port and writes contents into a file ##
###################################################################

##Install pySerialL 
# sudo easy_install pyserial
##Change the port address: 
##On termial list all ports: 
# ls /dev/tty.*
# ls /dev/cu.*
# Choose /dev/"arduino port"


import serial

serial_port = '/dev/ttyACM0'; # change to arduino board

baud_rate = 9600; #In arduino, Serial.begin(baud_rate)
write_to_file_path = "output.txt";

output_file = open(write_to_file_path, "w+");
ser = serial.Serial(serial_port, baud_rate)
while True:
    line = ser.readline();
    line_int = int(line, 2) 
    #line = line.decode("utf-8") #ser.readline returns a binary, convert to string
    print(line_int);
    output_file.write(line_int);