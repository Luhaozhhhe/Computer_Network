import matplotlib.pyplot as plt
import numpy as np

# 丢包率数据
packet_loss = np.array([0, 120, 240, 360, 480])

# 不同窗口大小的吞吐率（bytes/s）
window_4 = np.array([3654.05, 342.07, 177.59, 160.08, 145.99])
window_16 = np.array([2980.6, 364.27, 195.35, 166.45, 146.42])
window_32 = np.array([2897.13, 376.34, 207.32, 169.44, 150.48])

# 设置柱状图宽度
bar_width = 0.25

# 设置x轴位置
x = np.arange(len(packet_loss))

# 创建图形
plt.figure(figsize=(10, 6))

# 绘制柱状图
plt.bar(x - bar_width, window_4, width=bar_width, label='Window Size 4', color='blue', alpha=0.7)
plt.bar(x, window_16, width=bar_width, label='Window Size 16', color='orange', alpha=0.7)
plt.bar(x + bar_width, window_32, width=bar_width, label='Window Size 32', color='green', alpha=0.7)

# 为每个柱子添加数据标签
for i in range(len(packet_loss)):
    plt.text(x[i] - bar_width, window_4[i] + 20, f'{window_4[i]:.2f}', ha='center', fontsize=10, color='blue')
    plt.text(x[i], window_16[i] + 20, f'{window_16[i]:.2f}', ha='center', fontsize=10, color='orange')
    plt.text(x[i] + bar_width, window_32[i] + 20, f'{window_32[i]:.2f}', ha='center', fontsize=10, color='green')

# 添加标签和标题
plt.xlabel('Latency (ms)', fontsize=14)
plt.ylabel('Throughput (bytes/s)', fontsize=14)
plt.title('Throughput for Different Window Sizes and Latencies', fontsize=16)

# 设置x轴标签位置
plt.xticks(x, packet_loss, fontsize=12)

# 添加网格线
plt.grid(True, which='both', axis='y', linestyle='--', alpha=0.7)

# 显示图例
plt.legend()

# 调整布局以适应标签
plt.tight_layout()

# 显示图形
plt.show()
