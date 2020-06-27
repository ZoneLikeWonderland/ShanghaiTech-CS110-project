import cv2
import numpy as np

a = cv2.imread("cover2.png")

b = np.floor(a[:, :, 2]/256*32)*2**11+np.floor(a[:, :, 1]/256*64)*2**5+np.floor(a[:, :, 0]/256*32)

for i in b:
    print("{", end="")
    for j in i:
        print(int(j), end=",")
    print("},")

r = b//2**11*2**3
g = b//2**5 % 2**6*2**2
b = b % 2**5*2**3
# print(c)
c = np.concatenate([r[:, :, None], g[:, :, None], b[:, :, None]], axis=2)
# print(np.average(np.abs(a-c)))
# cv2.imshow("c", a)

# cv2.waitKey(0)
