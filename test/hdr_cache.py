import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import copy
import cv2

def luminance(rgb):
    return 0.2 * rgb[0] + 0.6 * rgb[1] + 0.1 * rgb[2]

def pdf_to_cdf(pdf):
    length = pdf.shape[0]
    cdf = copy.copy(pdf)
    for i in range(1, length):
        cdf[i] += cdf[i-1]
    return cdf

def lower_bound(arr, val):
    length = arr.shape[0]
    for i in range(1, length):
        if arr[i]>=val:
            return i-1
    return length - 1

def sample_from_cdf(cdf):
    r = np.random.rand()
    idx = lower_bound(cdf, r)
    return idx

def build_cache(cdf_x_condition, cdf_y_marginal, width, height):
    cache = np.zeros((height, width))
    return cache

# ------------------------------------------------------------------------- #

# 读图片
img = cv2.cvtColor(cv2.imread('../assets/textures/hdr/newport_loft.hdr'), cv2.COLOR_BGR2RGB) / 255.0
img_gamma = np.power(img[:,:], 1.0/2.2)  # 输出用
#img = np.power(img[:,:], 2.0)  # 对比度更高效果更好
h, w = img.shape[:2]
print(f'width: {w}, height:{h}')  # 512, 256

# 以亮度做概率 计算归一化的概率密度
pdf = img[:,:,0] * 0.2 + img[:,:,1] * 0.6 + img[:,:,2] * 0.1
pdf /= np.sum(pdf)

# 计算 y 的边缘概率密度
pdf_y_marginal = np.zeros(h)
for i in range(h):
    pdf_y_marginal[i] = np.sum(pdf[i, :])

# 计算 y 的边缘概率分布
cdf_y_marginal = pdf_to_cdf(pdf_y_marginal)

# pdf_x_condition[i][j] 表示在 y=i 的条件下，取 x=j 的条件概率
pdf_x_condition = copy.copy(pdf)
for i in range(h):
    pdf_x_condition[i] /= pdf_y_marginal[i]
    
# 计算 x 的条件概率的分布
cdf_x_condition = np.zeros(pdf_x_condition.shape)
for i in range(h):
    cdf_x_condition[i] = pdf_to_cdf(pdf_x_condition[i])

# 采样
N = 500
points = []
for i in range(N):
    y = sample_from_cdf(cdf_y_marginal)
    x = sample_from_cdf(cdf_x_condition[y])
    points.append([x, y])
    if i%20==0:
        print(f'sampling: {i}/{N}') 
print('done')

# 输出
print('ploting...')
plt.subplot(2, 2, 1)
plt.imshow(img_gamma)

plt.subplot(2, 2, 2)
plt.imshow(pdf)

plt.subplot(2, 2, 3)
plt.imshow(img_gamma)
for p in points:
    plt.plot(p[0], p[1], '.', color='red')
    
plt.subplot(2, 2, 4)
plt.imshow(pdf)
for p in points:
    plt.plot(p[0], p[1], '.', color='red')
    
plt.show()