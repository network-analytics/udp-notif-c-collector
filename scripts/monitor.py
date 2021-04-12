import sys
import time
output = []
start = round(time.time()*1000)
while True: 
    time.sleep(0.010)
    infile = open("/proc/net/udp", "r")
    for line in infile:
        #print(line.split(" ")[0])
        #print(line.split(" ")[1])
        if line.split(" ")[1] == sys.argv[1]+":":
            spli = line.split(" ")
            #print(spli)
            hexa = spli[5].split(":")[1]
            intv = int(hexa, 16)
            output.append(intv)
            #if intv > 500000:
            #print("c "+spli[len(spli)-6]+" t")
            print(str(round(time.time()*1000)-start)+" "+str(float(intv)/1000000.0)+"\t c"+spli[len(spli)-6]+" t")
            sys.stdout.flush()
            #hexa = line.split(" ")[4].split(":")[1]
            #print(hexa)
