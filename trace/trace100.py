import sys

trace_files = ["1618_3.csv"]

trace_fileszip = ["1618_3zip10.csv"]


for trace_name, trace_nameszip in zip(trace_files, trace_fileszip):
	fi = open(trace_name,"r")
	fo = open(trace_nameszip,"w")

	total_request = []
	for line in fi:
		data = line.strip().split(" ")
		data[0]=int(int(data[0])*10)
		#total_request.append((str(data[0]),data[1],data[2],data[3],data[4],data[5]))
		total_request.append((str(data[0]),data[1],data[2],data[3],data[4]))

	for data in total_request: 
		#fo.write(data[0] + " " + data[1] + " " + data[2] + " " + data[3] + " " + data[4] + " " + data[5] + "\n")
		fo.write(data[0] + " " + data[1] + " " + data[2] + " " + data[3] + " " + data[4] + "\n")
