import matplotlib.pyplot as plt
import numpy as np

# 延迟数据（毫秒）
delay = np.array([0, 120, 240, 360, 480])

# 不同情况下的吞吐率（单位：bytes/s）
no_congestion = np.array([2897.13, 782.20, 472.35, 363.35, 261.19])
with_congestion = np.array([2847.63, 2428.15, 2071.09, 1839.91, 1613.33])

# 创建图形
plt.figure(figsize=(10, 6))

# 设置柱状图宽度
width = 40

# 绘制柱状图
plt.bar(delay - width/2, no_congestion, width=width, label='No Congestion', color='blue', alpha=0.7)
plt.bar(delay + width/2, with_congestion, width=width, label='With Congestion', color='orange', alpha=0.7)

# 为每个条形添加数据标签
for i in range(len(delay)):
    plt.text(delay[i] - width/2, no_congestion[i] + 50, f'{no_congestion[i]:.2f}', ha='center', fontsize=10, color='blue')
    plt.text(delay[i] + width/2, with_congestion[i] + 50, f'{with_congestion[i]:.2f}', ha='center', fontsize=10, color='orange')

# 设置标签和标题
plt.xlabel('Delay (ms)', fontsize=14)
plt.ylabel('Throughput (bytes/s)', fontsize=14)
plt.title('Throughput Comparison for Different Latencies', fontsize=16)

# 添加网格线
plt.grid(True, which='both', linestyle='--', alpha=0.7)

# 显示图例
plt.legend()

# 调整布局
plt.tight_layout()

# 显示图形
plt.show()
