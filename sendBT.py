from select import select
import time
import httplib, urllib
import sys
import time
import serial
import argparse

ADDR = 0
NAME = 1
PORT = 2

devFile = '.dev'
devSerial = {}
devices = []

ap = argparse.ArgumentParser()
ap.add_argument("-b", "--baudrate", type=int, help="Baudrate")
args = vars(ap.parse_args())

#------------------------------------------------------------------------------------------------

def sendToThingSpeak(line):
    fls = line.split()
    sendSet = {}
    try:
        for idx,val in enumerate(fls):
            if val == "S":
                sendSet['field1'] = str(fls[idx+1])
            if val == "T":
                sendSet['field3'] = str(fls[idx+1])
            if val == "H":
                sendSet['field2'] = str(fls[idx+1]) 
        sendSet['key']  = 'S95F5P1XW1DEZ5KA'
    except:
        print "Incorrect message"
    
    print sendSet

    params = urllib.urlencode(sendSet)
    headers = {"Content-type": "application/x-www-form-urlencoded","Accept": "text/plain"}
    conn = httplib.HTTPConnection("api.thingspeak.com:80")
    
    try:
        conn.request("POST", "/update", params, headers)
        response = conn.getresponse()
        print response.status, response.reason
        data = response.read()
        conn.close()
    except:
        print "connection failed"	

def waitKey(timeout):
    rlist, wlist, xlist = select([sys.stdin], [], [], timeout)
    
    if rlist:
        return False
    return True

def listen():
    greenhouse = raw_input("Greenhouse (0-%d): " % (len(devices)-1))
    curDev = devices[int(greenhouse)]

    while True:
        if (devSerial[curDev[ADDR]].isOpen):
            data = devSerial[curDev[ADDR]].readline()
            print data,
        else: 
            print "Serial /dev/rfcomm%s error! Message Aborted." % curDev[PORT]
        
        if not waitKey(0.03):
            return
 
def loop(interval):
    greenhouse = raw_input("Greenhouse (0-%d): " % (len(devices)-1))
    curDev = devices[int(greenhouse)]

    before = 0
    while True:
        now = time.time()
        if (devSerial[curDev[ADDR]].isOpen):
            if (now - before >= interval):
                data = devSerial[curDev[ADDR]].readline()
                sendToThingSpeak(data)

                before = now
        else: 
            print "Serial /dev/rfcomm%s error! Message Aborted." % curDev[PORT]
            
        if not waitKey(0.03):
            return

#------------------------------------------------------------------------------------------------

with open(devFile, 'r+') as fd:
    lines = fd.readlines()
    for i in range(len(lines)):
        lines[i] = lines[i].replace('\n', '')
        devLine = lines[i].split(' ')
        devices.append(devLine)

for dev in devices:
    newSerial = serial.Serial("/dev/rfcomm%s" % dev[PORT], args.get("baudrate", None))
    devSerial[dev[ADDR]] = newSerial

while True:
    com = raw_input("Command [send|listen|loop|exit]: ")

    if (com == 'exit'):
        break
    elif (com == 'listen'):
        listen()
        continue
    elif (com == 'loop'):
        loop(16)
        continue
    elif not (com == 'send'):
        continue
    
    greenhouse = raw_input("Greenhouse (0-%d): " % (len(devices)-1))
    msg = raw_input("Command: ")

    curDev = devices[int(greenhouse)]

    if (devSerial[curDev[ADDR]].isOpen):
        print "Sending {%s} to serial port /dev/rfcomm%s." % (msg, curDev[PORT])
        devSerial[curDev[ADDR]].write(msg)
        devSerial[curDev[ADDR]].write('\0')
        print "AT sent to greenhouse %s in address %s." % (curDev[NAME], curDev[ADDR])
    else:
        print "Serial /dev/rfcomm%s error! Message Aborted." % curDev[PORT]
