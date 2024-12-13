import matplotlib.pyplot as plt
import numpy as np

# 窗口大小
window_sizes = np.array([4, 8, 16, 24, 32])

# 第一组数据（总时间）
data1 = np.array([1642, 2054, 2013, 1937, 2071])

# 第二组数据（吞吐量）
data2 = np.array([3654.05, 2921.11, 2980.6, 3097.55, 2897.13])

# 设置柱状图宽度
bar_width = 0.35

# 设置x轴位置
x1 = np.arange(len(window_sizes))
x2 = x1 + bar_width

# 创建图形和子图
fig, ax1 = plt.subplots(figsize=(8, 6))

# 绘制柱状图（总时间）
bars1 = ax1.bar(x1, data1, width=bar_width, label='Total Time', color='cornflowerblue', alpha=0.7)

# 设置x轴标签
ax1.set_xticks(x1 + bar_width / 2)
ax1.set_xticklabels(window_sizes, fontsize=12)
ax1.set_xlabel('Window Sizes', fontsize=14)

# 设置y轴标签
ax1.set_ylabel('Total Time (ms)', fontsize=14)

# 绘制吞吐量数据（折线图）
ax2 = ax1.twinx()  # 使用第二个y轴
ax2.plot(x2, data2, label='Throughput', color='forestgreen', marker='o', markersize=8, linewidth=2)

# 设置y轴标签
ax2.set_ylabel('Throughput (items/s)', fontsize=14)

# 设置y轴范围，使吞吐量波动更平滑
ax2.set_ylim(min(data2) - 200, max(data2) + 200)  # 设置适当的上下界限，减少剧烈波动

# 设置标题
plt.title('Window Sizes vs Total Time and Throughput', fontsize=16)

# 设置图例
ax1.legend(loc='upper left', fontsize=12)
ax2.legend(loc='upper right', fontsize=12)

# 在柱状图上显示数据值
for bar in bars1:
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width() / 2., height + 30,
             '{:.0f}'.format(height),
             ha='center', va='bottom', fontsize=10)

# 在折线上显示数据值
for a, b in zip(x2, data2):
    ax2.text(a, b + 30, '{:.2f}'.format(b), ha='center', va='bottom', fontsize=10)

# 设置网格线
ax1.grid(True, axis='y', linestyle='--', alpha=0.7)

# 显示图形
plt.tight_layout()
plt.show()
