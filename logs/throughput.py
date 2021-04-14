import pandas as pd
import sys

message_size = int(sys.argv[1])
threads = int(sys.argv[2])

df = pd.read_csv('throughput.csv', sep=',')

print(df['time'].describe())

avg = df['time'].mean()
packets = df['packets'].min()

throughput = (packets * message_size * 8)/(avg/1000)/1000000000
throughput_Mb = ((packets * message_size / avg)/1000)*8

estimated = round(throughput * threads, 4)
estimated_Mb = round(throughput_Mb * threads, 4)

throughput = round(throughput, 4)
throughput_Mb = round(throughput_Mb, 4)

print('\nthroughput: ' + str(throughput_Mb) + " Mb/s / thread")
print('\nthroughput: ' + str(throughput) + " Gb/s / thread")
print('\nestimated throughput: ' + str(throughput_Mb * threads) + " Mb/s | " + str(throughput * threads) + " Gb/s")
