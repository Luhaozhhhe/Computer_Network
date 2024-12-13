import matplotlib.pyplot as plt
import numpy as np

# 延迟数据（毫秒）
latency = np.array([0, 120, 240, 360, 480])

# 不同情况下的传输时间（单位：ms）
no_congestion = np.array([2071, 7671, 12703, 16513, 22971])
with_congestion = np.array([2107, 2471, 2897, 3261, 3719])

# 创建图形
plt.figure(figsize=(10, 6))

# 绘制折线图
plt.plot(latency, no_congestion, marker='o', label='No Congestion', color='blue', linestyle='-', linewidth=2)
plt.plot(latency, with_congestion, marker='s', label='With Congestion', color='orange', linestyle='-', linewidth=2)

# 为每个点添加数据标签
for i in range(len(latency)):
    plt.text(latency[i], no_congestion[i], f'{no_congestion[i]}', ha='center', va='bottom', fontsize=10, color='blue')
    plt.text(latency[i], with_congestion[i], f'{with_congestion[i]}', ha='center', va='bottom', fontsize=10, color='orange')

# 设置标签和标题
plt.xlabel('Latency (ms)', fontsize=14)
plt.ylabel('total time(ms)', fontsize=14)
plt.title('Total Time Comparison for Different Latencies', fontsize=16)

# 添加网格线
plt.grid(True, which='both', linestyle='--', alpha=0.7)

# 显示图例
plt.legend()

# 调整布局
plt.tight_layout()

# 显示图形
plt.show()
