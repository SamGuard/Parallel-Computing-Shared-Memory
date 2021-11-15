import os
import sys
import subprocess
from decimal import *
getcontext().prec = 8


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

    # Build 2D array of all values from string
    for rowString in dataString:
        row = rowString.split(" ")
        if(len(row) == 1):  # Skip just spaces or empty index's
            continue
        # print(row)
        data.append([])
        for x in row:
            if(x != ""):
                data[-1].append(Decimal(x))

    for x in range(width):
        for y in range(height):

            cCell = data[x][y]  # Current cell
            # Program is set to have bounderies at 0 and 1 that are constant
            if(x == 0 or y == 0 or x == width-1 or y == height-1):
                if(cCell != 0.0 and cCell != 1.0):
                    return {"success": False, "reason": "Boundery value changed: {}".format(cCell), "data": data}
            else:
                if(cCell > data[x-1][y] or cCell > data[x][y-1]):
                    return {"success": False, "reason": "Invalid values, values must decrease as index increases", "data": [cCell, data[x-1][y], data[x][y-1]]}

    return {"success": True, "reason": "", "data": data}

def compareOut(grid0, grid1, precision):
    totalDifference = 0

    for i in range(len(grid0)):
        for j in range(len(grid0[i])):
            cell0 = grid0[i][j]
            cell1 = grid1[i][j]
            totalDifference += abs(cell0 - cell1)
    meanDifference = totalDifference / (len(grid0) * len(grid0[0]))

    if(meanDifference > precision):
        return False
    return True

def validate():
    width = height = 16
    precision = 0.000001
    sequentialData = []
    for workers in [1, 2, 4, 8, 16, 32, 44]:
        command = "./main {} {} {} {} 1".format(
            workers, width, height, precision)
        processOutput = subprocess.run(
            command, shell=True, capture_output=True)
        check = checkOut(processOutput)
        if(check["success"] == False):
            continue
        if(workers == 1):
            sequentialData = check["data"]
        else:
            if(not compareOut(sequentialData, check["data"], 0.00001)):
                check["success"] = False
                check["Reason"] = "Sequential and parallel programs differ by too much"
        

        os.system("Dim: {}x{}, Success: {}".format(
            width, height, check["success"]))
        if(check["success"] == False):
            os.system("Reason: {}, Data: ".format(check["reason"]))
            os.system(check["data"])


def main(mode):
    compileExitCode = os.system("gcc main.c -o main -lpthread -O3")
    if(mode == "performance"):
        performance()
    elif(mode == "validate"):
        validate()


if(__name__ == "__main__"):
    main(sys.argv[1])
