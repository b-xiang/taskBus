#taskBus interface for Python3 only. 
#if you want to use python2, please take a look at example_gnuradio
import sys;
from time import sleep
def init_client():
    if sys.platform == "win32":
        import os, msvcrt
        msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
        msvcrt.setmode(sys.stdin.fileno(), os.O_BINARY)
def args2dict(argv):
    #1. Interpret cmd line
    totalLen = len(argv)
    args = {}
    currKey = '_CMD_'
    currValue = []
    for it in range(totalLen):
        currArg = argv[it].strip();
        if currArg.startswith('-'):
            args[currKey] = []
            args[currKey].extend(currValue)
            parts = currArg.split('=')
            if len(parts)>0:
                currKey = parts[0]
                del parts[0]
            currValue.clear()
            currValue.extend(parts)
        else:
            currValue.append(currArg)
    args[currKey] = []
    args[currKey].extend(currValue)
    return args

def taskBus_pull_header():
    header = sys.stdin.buffer.read(4)
    if (len(header)==4):
        if header[0]!=0x3C or header[1]!=0x5A or header[2]!=0x7E or header[3]!=0x69:
            del header[0]
    while (len(header)<4):
        tb = sys.stdin.buffer.read(4-len(header))
        if (len(tb)):
            header.extend(tb)
        else:
            sleep(0.1)

        if (len(header)==4):
            if header[0]!=0x3C or header[1]!=0x5A or header[2]!=0x7E or header[3]!=0x69:
                del header[0]
    return header

def taskBus_pull_int(endian):
    arrsub = sys.stdin.buffer.read(4)
    while (len(arrsub)<4):
        sleep(0.1)
        tb = sys.stdin.buffer.read(4-len(arrsub))
        if (len(tb)):
            arrsub.extend(tb)
    n_sub = int.from_bytes(arrsub,endian)
    return n_sub

def taskBus_pull_bytes(leng):
    arrsub = sys.stdin.buffer.read(leng)
    while (len(arrsub)<leng):
        sleep(0.1)
        tb = sys.stdin.buffer.read(leng-len(arrsub))
        if (len(tb)):
            arrsub.extend(tb)    
    return arrsub
        

def taskBus_pull(endian="little"):
    cab = {}
    #header
    cab["header"] = taskBus_pull_header()
    #subject
    cab["subject"] = taskBus_pull_int(endian)
    #path    
    cab["path"] = taskBus_pull_int(endian)
    #length
    cab["length"] = taskBus_pull_int(endian)
    cab["package"] = taskBus_pull_bytes( cab["length"])
    return cab

def taskBus_push(sub,path,bytea,endian="little"):
    header = b'\x3c\x5a\x7e\x69'
    sys.stdout.buffer.write(header)
    sys.stdout.buffer.write(sub.to_bytes(4,endian))
    sys.stdout.buffer.write(path.to_bytes(4,endian))
    lens = len(bytea)
    sys.stdout.buffer.write(lens.to_bytes(4,endian))
    sys.stdout.buffer.write(bytea)
    sys.stdout.buffer.flush()
