import sys

trace_files = ['1618_3.csv', '1615_0.csv']

# trace_files = ["proj_0.csv", "1614_0.csv"]
# trace_files_from0 = ["proj_0from0.csv", "1614_0from0.csv"]

trace_files_from0 = ['my1618_3.csv', 'my1615_0.csv']

for traceName, traceNameFrom0 in zip(trace_files, trace_files_from0):
    fi = open(traceName, "r")
    fo = open(traceNameFrom0, "w")

    total_request = []
    count = 0
    temp_time = 0

    for line in fi:
        data = line.strip().split(" ")
        if count == 0:
            temp_time = int(data[0])
            count = 1
            data[0] = 1

        else:
            data[0] = int(data[0]) - temp_time + 1
        # total_request.append((str(data[0]),data[1],data[2],data[3],data[4],data[5]))
        total_request.append((str(data[0]), data[1], data[2], data[3], data[4]))
    #
    for data in total_request:
        # fo.write(data[0] + " " + data[1] + " " + data[2] + " " + data[3] + " " + data[4] + " " + data[5] + "\n")
        fo.write(data[0] + " " + data[1] + " " + data[2] + " " + data[3] + " " + data[4] + "\n")

    print(traceName, 'completed')

