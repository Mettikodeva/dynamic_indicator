import numpy as np
import os
import time
# SIGNAL_COLOR = 1
def str3dlist(data):
    return str(data).replace("[","{").replace("]","}").replace(" ","")

def generate_signal():

    data_left = []
    leds = [[0]*3]*15

    # for i in range(5):
    f = open("data_left.txt", "w")
    f.write("{")

    for i in range(15):
        leds[15-i-1] = [255,200,10]
        data_left.append(leds)
        print(leds)
        # if i != 14:
        f.writelines(str3dlist(leds)+",\\\n")
        # else:
        #     f.writelines(str3dlist(leds)+"\\\n")
        time.sleep(0.1)
        os.system("clear")
    # print(data_left)
    # print(len(data_left))
    # time.sleep(5)
    for i in range(1,11):
        for j in range(15):
            leds[15-j-1] = (np.array([255,200,10])*(25.6*(10-i))/256).round().astype(int).tolist()
            
        data_left.append(leds)
        if i != 10:
            f.writelines(str3dlist(leds)+",\\\n")
        else:
            f.writelines(str3dlist(leds)+"\\\n")
        time.sleep(0.01)
        os.system("clear")

    print(len(data_left))
    # print(data_left)
    # print(data_left)
    print(data_left[0])
    f.write("}")
    f.close()

def generate_breathing():
    pass

def generate_strobe():
    pass