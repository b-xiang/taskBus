#GNU radio Using Python2 , so , this file defines 
#taskBus interface for Python2 only. 
#for python3, please see example_python.
import sys;
from time import sleep

def init_client():
    if sys.platform == "win32":
        import os, msvcrt
        msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
        msvcrt.setmode(sys.stdin.fileno(), os.O_BINARY)


#cmdline to dict
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
            currValue = []
            currValue.extend(parts)
        else:
            currValue.append(currArg)
    args[currKey] = []
    args[currKey].extend(currValue)
    return args
#get header string from stream.
def taskBus_pull_header():
    header = sys.stdin.read(4)
    if (len(header)==4):
        if header[0]!='\x3c' or header[1]!='\x5a' or header[2]!='\x7e' or header[3]!='\x69':
            header = header[1:]
    while (len(header)<4):
        tb = sys.stdin.read(4-len(header))
        if (len(tb)):
            header = header + tb
        else:
            sleep(0.1)
        if (len(header)==4):
            if header[0]!='\x3c' or header[1]!='\x5a' or header[2]!='\x7e' or header[3]!='\x69':
                 header = header[1:]
    return header

def taskBus_pull_int():
    arrsub = sys.stdin.read(4)
    while (len(arrsub)<4):
        sleep(0.1)
        tb = sys.stdin.read(4-len(arrsub))
        if (len(tb)):
            arrsub = arrsub +tb
    n_sub = ord(arrsub[0]) + (ord(arrsub[1])<<8) + (ord(arrsub[2])<<16) + (ord(arrsub[3])<<24)
    return n_sub

def taskBus_pull_bytes(leng):
    arrsub = sys.stdin.read(leng)
    while (len(arrsub)<leng):
        sleep(0.1)
        tb = sys.stdin.read(leng-len(arrsub))
        if (len(tb)):
            arrsub = arrsub +tb
    return arrsub
        

def taskBus_pull():
    cab = {}
    #header
    cab["header"] = taskBus_pull_header()
    #subject
    cab["subject"] = taskBus_pull_int()
    #path    
    cab["path"] = taskBus_pull_int()
    #length
    cab["length"] = taskBus_pull_int()
    cab["package"] = taskBus_pull_bytes( cab["length"])
    return cab

def taskBus_push(sub,path,bytea):
    header = b'\x3c\x5a\x7e\x69'
    sys.stdout.write(header)
    sys.stdout.write(chr(sub&0xFF))
    sys.stdout.write(chr((sub>>8)&0xFF))
    sys.stdout.write(chr((sub>>16)&0xFF))
    sys.stdout.write(chr((sub>>24)&0xFF))
    sys.stdout.write(chr(path&0xFF))
    sys.stdout.write(chr((path>>8)&0xFF))
    sys.stdout.write(chr((path>>16)&0xFF))
    sys.stdout.write(chr((path>>24)&0xFF))
    lens = len(bytea)
    sys.stdout.write(chr(lens&0xFF))
    sys.stdout.write(chr((lens>>8)&0xFF))
    sys.stdout.write(chr((lens>>16)&0xFF))
    sys.stdout.write(chr((lens>>24)&0xFF))   
    sys.stdout.write(bytea)
    sys.stdout.flush()
