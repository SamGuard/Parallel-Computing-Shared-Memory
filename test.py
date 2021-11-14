import os
import sys
import subprocess


def performance():
    os.system("echo workers,width,height,precision,iterations,time")

    for workers in [1, 2, 4, 8, 16, 32, 44]:
        for scale in [16, 64, 256, 512, 1024]:
            width = height = scale
            precision = 0.000001
            command = "./main {} {} {} {}".format(
                workers, width, height, precision)
            subprocess.run(command, shell=True)


def main(mode):
    compileExitCode = os.system("gcc main.c -o main -lpthread -O3")
    if(mode == "performance"):
        performance()


if(__name__ == "__main__"):
    main(sys.argv[1])
