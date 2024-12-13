import matplotlib.pyplot as plt
import numpy as np

# 延迟数据
delays = np.array([0, 120, 240, 360, 480])

# 不同窗口大小的传输时间（ms）
window_4 = np.array([1642, 17540, 33784, 37481, 41097])
window_16 = np.array([2013, 16471, 30714, 36047, 40977])
window_32 = np.array([2071, 15943, 28941, 35410, 39871])

# 创建图形
plt.figure(figsize=(10, 6))

# 绘制折线图
plt.plot(delays, window_4, marker='o', label='Window Size 4', color='blue', linestyle='-', linewidth=2)
plt.plot(delays, window_16, marker='s', label='Window Size 16', color='orange', linestyle='-', linewidth=2)
plt.plot(delays, window_32, marker='^', label='Window Size 32', color='green', linestyle='-', linewidth=2)

# 为每个点添加数据标签
for i in range(len(delays)):
    plt.text(delays[i], window_4[i], f'{window_4[i]:.0f}', ha='center', va='bottom', fontsize=10, color='blue')
    plt.text(delays[i], window_16[i], f'{window_16[i]:.0f}', ha='center', va='bottom', fontsize=10, color='orange')
    plt.text(delays[i], window_32[i], f'{window_32[i]:.0f}', ha='center', va='bottom', fontsize=10, color='green')

# 添加标签和标题
plt.xlabel('Latency(ms)', fontsize=14)
plt.ylabel('total time(ms)', fontsize=14)
plt.title('Total Time for Different Window Sizes and Latency', fontsize=16)

# 添加网格线
plt.grid(True, which='both', linestyle='--', alpha=0.7)

# 显示图例
plt.legend()

# 调整布局以适应标签
plt.tight_layout()

# 显示图形
plt.show()
