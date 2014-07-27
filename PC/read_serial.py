import serial
import locale
import itertools, collections

def consume(iterator, n):
    collections.deque(itertools.islice(iterator, n))

encoding = locale.getdefaultlocale()[1]

ser = serial.Serial('COM3', 115200, timeout=1)

data = ser.read(100)
data_list = list(data)

print(data_list)

iterator = range(0, len(data_list))

for i in iterator:

    c = data_list[i]
    
    if (c == '['):
        header = ord(data_list[i+1])
        count = int(header & 0b00001111)
        future = int((header >> 5) & 0b11);
        sd = int((header >> 6) & 0b1);
        ack = int((header >> 7) & 0b1);
        msg_type = ord(data_list[i+2])

        print("SOM: %c CNT: %02i, SD: %01i, ACK: %01i, TYPE: %02i" % (c, count, sd, ack, msg_type))

        dat = data_list[i+3:i+6]
        print(dat)
        crc = hex(int((ord(data_list[i+6]) << 8) | ord(data_list[i+7])))
        print(crc)

    
ser.close()
