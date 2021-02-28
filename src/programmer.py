import serial
from time import sleep
import argparse

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
    parser = argparse.ArgumentParser(description="Upload code to an ATTiny10")
    parser.add_argument("--port", help="COM port to upload to", required=True)
    parser.add_argument("--file", help=".hex file to upload", required=True)
    parser.add_argument("--reset-12v", help="Set this flag to enable 12v reset", action="store_const", const=True, default=False, dest="reset12v")
    args = parser.parse_args()
    
    sp = serial.Serial(args.port, 115200, timeout=1)
    sleep(3) #wait for arduino to reset
    
    sp.read(10)
    if args.reset12v:
        send(sp, "SVX")
    else:
        send(sp, "SvX")
        
    data = get_data(args.file)
    send(sp, "S")
    send(sp, data)
    send(sp, "R")
    mem = sp.read(1024*2).decode()
    
    # check that we wrote what we wanted to write
    expected = data + "F" * (len(mem) - len(data))
    assert mem == expected, f"It failed\n{mem}\n{expected}"
    
    # diable programming interface
    send(sp, "X")
    sp.close()

if __name__ == "__main__":
    main()