import serial
from time import sleep

def get_data(path):
    with open(path) as f:
        data = f.readlines()
    res = ""
    for l in data:
        d = l[9:-3]
        res += d
    return res
    
def send(sp, msg):
    for c in msg:
        sp.write(c.encode())
        back = sp.read().decode()
        if back != c:
            print(back, c)
    
def main():
    sp = serial.Serial('COM4', 115200, timeout=.5)
    sleep(3) #wait for arduino to reset
    
    sp.read(10)
    send(sp, "S")
    data = get_data("C:\\Users\\Anton\\Documents\\Atmel Studio\\7.0\\attiny10blink\\attiny10blink\\Debug\\attiny10blink.hex")
    send(sp, data)
    send(sp, "R")
    mem = sp.read(1024*2).decode()
    
    # check that we wrote what we wanted to write
    assert mem == data + "F" * (len(mem) - len(data)), "It failed"
    
    # diable programming interface
    send(sp, "X")
    sp.flush()
    sp.close()

if __name__ == "__main__":
    main()