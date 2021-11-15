import os
import sys
import subprocess


def performance():
    os.system("echo workers,width,height,precision,iterations,time")

    for workers in [1, 2, 4, 8, 16, 32, 44]:
        for scale in [16, 64, 256, 512, 1024]:
            width = height = scale
            precision = 0.000001
            command = "./main {} {} {} {} 0".format(
                workers, width, height, precision)
            subprocess.run(command, shell=True)


def checkOut(out):
    stdOut = out.stdout.decode("utf-8").split(",")
    width = int(stdOut[1])
    height = int(stdOut[2])
    dataString = stdOut[6].split("\n")
    data = []

    #Build 2D array of all values from string
    for rowString in dataString:
        row = rowString.split(" ")
        if(len(row) == 1):#Skip just spaces or empty index's
            continue
        # print(row)
        data.append([])
        for x in row:
            if(x != ""):
                data[-1].append(float(x))

    for x in range(width):
        for y in range(height):

            cCell = data[x][y]  # Current cell
            if(x == 0 or y == 0 or x == width-1 or y == height-1): # Program is set to have bounderies at 0 and 1 that are constant
                if(cCell != 0.0 and cCell != 1.0):
                    return {"success": False, "reason": "Boundery value changed: {}".format(cCell)}
            else:
                if(cCell >= data[x-1][y] or cCell >= data[x][y-1] or cCell >= data[x-1][y-1]):
                    return {"success": False, "reason": "Invalid values, values must decrease as index increases"}

    return {"success": True, "reason": ""}


def validate():
    width = height = 16
    precision = 0.000001
    for workers in [1, 2, 4, 8, 16, 32, 44]:
        command = "./main {} {} {} {} 1".format(
            workers, width, height, precision)
        processOutput = subprocess.run(
            command, shell=True, capture_output=True)
        check = checkOut(processOutput)

        print("Dim: {}x{}, Success: {}, Reason: {}".format(
            width, height, check["success"], check["reason"]))


def main(mode):
    compileExitCode = os.system("gcc main.c -o main -lpthread -O3")
    if(mode == "performance"):
        performance()
    elif(mode == "validate"):
        validate()


if(__name__ == "__main__"):
    main(sys.argv[1])
