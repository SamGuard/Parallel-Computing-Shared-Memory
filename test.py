import os
import sys
import subprocess


def performance():
    print("workers,width,height,iterations,time")

    for workers in [1, 2, 4, 8, 16, 32, 44]:
        for scale in [16, 32, 64, 128, 256, 512]:
            width = height = scale
            for precision in [1, 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001]:
                command = "./main {} {} {} {}".format(
                    workers, width, height, precision)
                subprocess.run(command, shell=True)


def main(mode):
    compileExitCode = os.system("gcc main.c -o main -lpthread -O3")
    if(mode == "performance"):
        performance()


if(__name__ == "__main__"):
    main(sys.argv[1])
