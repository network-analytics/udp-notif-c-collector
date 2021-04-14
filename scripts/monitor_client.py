import sys
import time
output = []
start = round(time.time()*1000)
last = start
while True: 
    time.sleep(0.001)
    infile = open("/proc/net/udp", "r")
    for line in infile:
        #meer:line.split(" ")[0])
        #ietf:line.split(" ")[1])
        if line.split(" ")[0] == sys.argv[1]+":":
            spli = line.split(" ")
            #print(spli)
            hexa = spli[4].split(":")[1]
            #meer:hexa = spli[4].split(":")[1]
            #ietf:hexa = spli[5].split(":")[1]
            intv = int(hexa, 16)
            now = round(time.time()*1000)
            #output.append(intv)
            #if intv > 100000:
            #print("c "+spli[len(spli)-6]+" t")
            print(str(now-start)+" "+str(float(intv)/1000000.0)+"\t c"+spli[len(spli)-6]+" t")
            if now-last > 10:
                print("# now : "+str(now-start)+" last : "+str(last-start)+" diff : "+str(now-last))
            sys.stdout.flush()
            last = now
            #hexa = line.split(" ")[4].split(":")[1]
            #print(hexa)
