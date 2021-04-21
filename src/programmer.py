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
    if len(res) % 4:
        res += "F" * (4 - len(res) % 4)
    return res
    
def send(sp, msg):
    for c in msg:
        sp.write(c.encode())
        back = sp.read().decode()
        assert back == c, f"Transfer failed. Got {back} sent {c}"
            
    
def main():
    parser = argparse.ArgumentParser(description="Upload code to an ATTiny10")
    parser.add_argument("--port", "-p", help="COM port to upload to", required=True)
    parser.add_argument("--file", "-f", help=".hex file to upload", required=True)
    parser.add_argument("--reset-12v", help="Set this flag to enable 12v reset", action="store_const", const=True, default=False, dest="reset12v")
    args = parser.parse_args()
    
    print("reading file..")
    data = get_data(args.file)
    print("file read")
    
    sp = serial.Serial(args.port, 115200, timeout=1)    
    sp.read(10)
    
    print("setting reset fuse")
    if args.reset12v:
        send(sp, "SVX")
        send(sp, "SVX")
    else:
        send(sp, "SvX")
        send(sp, "SvX")
    print("reset fuse set")
    
    print("sending program")
    send(sp, "S")
    send(sp, data)
    send(sp, "R")
    mem = sp.read(1024*2).decode()
    print("program sent")
    # check that we wrote what we wanted to write
    expected = data + "F" * (len(mem) - len(data))
    assert mem == expected, f"It failed\n{mem}\n{expected}"
    print("program read back")
    print("upload sucessful!")
    # diable programming interface
    send(sp, "X")
    sp.close()

if __name__ == "__main__":
    main()