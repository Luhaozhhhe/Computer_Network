import matplotlib.pyplot as plt
import numpy as np

# 丢包率数据
packet_loss = np.array([0, 20, 40, 60, 80])

# 不同情况下的吞吐率（bytes/s）
no_congestion = np.array([2897.13, 2571.97, 2219.04, 2198.57, 2078.16])
with_congestion = np.array([2847.63, 2726.01, 2604.15, 2482.39, 2370.59])

# 设置柱状图宽度
bar_width = 0.35
# 设置x轴位置
x1 = np.arange(len(packet_loss))
x2 = x1 + bar_width

# 创建图形
plt.figure(figsize=(10, 6))

# 绘制柱状图
plt.bar(x1, no_congestion, width=bar_width, label='No Congestion', color='blue', alpha=0.7)
plt.bar(x2, with_congestion, width=bar_width, label='With Congestion', color='orange', alpha=0.7)

# 为每个柱子添加数据标签
for i in range(len(packet_loss)):
    plt.text(x1[i], no_congestion[i] + 30, f'{no_congestion[i]:.2f}', ha='center', va='bottom', fontsize=10, color='blue')
    plt.text(x2[i], with_congestion[i] + 30, f'{with_congestion[i]:.2f}', ha='center', va='bottom', fontsize=10, color='orange')

# 添加标签和标题
plt.xlabel('Packet Loss Rate (\%)', fontsize=14)
plt.ylabel('Throughput (bytes/s)', fontsize=14)
plt.title('Throughput Comparison with Congestion and Packet Loss Rates', fontsize=16)

# 设置x轴标签
plt.xticks(x1 + bar_width / 2, packet_loss)

# 添加网格线
plt.grid(True, which='both', axis='y', linestyle='--', alpha=0.7)

# 显示图例
plt.legend()

# 调整布局以适应标签
plt.tight_layout()

# 显示图形
plt.show()
