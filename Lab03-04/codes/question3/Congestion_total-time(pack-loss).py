import matplotlib.pyplot as plt
import numpy as np

# 丢包率数据
packet_loss = np.array([0, 20, 40, 60, 80])

# 不同情况下的传输时间（ms）
no_congestion = np.array([2071, 2216, 2781, 2903, 3177])
with_congestion = np.array([2107, 2201, 2304, 2417, 2531])

# 创建图形
plt.figure(figsize=(10, 6))

# 绘制折线图
plt.plot(packet_loss, no_congestion, marker='o', label='No Congestion', color='blue', linestyle='-', linewidth=2)
plt.plot(packet_loss, with_congestion, marker='s', label='With Congestion', color='orange', linestyle='-', linewidth=2)

# 为每个点添加数据标签
for i in range(len(packet_loss)):
    plt.text(packet_loss[i], no_congestion[i], f'{no_congestion[i]}', ha='center', va='bottom', fontsize=10, color='blue')
    plt.text(packet_loss[i], with_congestion[i], f'{with_congestion[i]}', ha='center', va='bottom', fontsize=10, color='orange')

# 添加标签和标题
plt.xlabel('Packet Loss Rate(%)', fontsize=14)
plt.ylabel('total time(ms)', fontsize=14)
plt.title('Total Time Comparison with Congestion and Packet Loss Rates', fontsize=16)

# 添加网格线
plt.grid(True, which='both', linestyle='--', alpha=0.7)

# 显示图例
plt.legend()

# 调整布局以适应标签
plt.tight_layout()

# 显示图形
plt.show()
