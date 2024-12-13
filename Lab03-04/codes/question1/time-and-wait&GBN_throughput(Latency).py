import matplotlib.pyplot as plt
import numpy as np


loss_rates = np.array([0, 120, 240, 360, 480])

# 停等机制吞吐率数据
stop_and_wait_throughput = np.array([6858.83, 537.65, 279.65, 183.32, 133.78])

# 滑动窗口吞吐率数据
sliding_window_throughput = np.array([2897.13, 349.18, 162.29, 116.2, 80.07])


# 设置柱状图宽度
bar_width = 0.35

# 设置x轴位置
x_stop_and_wait = np.arange(len(loss_rates))
x_sliding_window = x_stop_and_wait + bar_width


# 绘制柱状图
bars1 = plt.bar(x_stop_and_wait, stop_and_wait_throughput, width=bar_width, label='Time-And-Wait',
                color='#6495ED', edgecolor='black')
bars2 = plt.bar(x_sliding_window, sliding_window_throughput, width=bar_width, label='GBN',
                color='#FFA500', edgecolor='black')


# 设置x轴标签
plt.xticks(x_stop_and_wait + bar_width / 2, loss_rates)

# 设置y轴标签和标题
plt.ylabel('Throughput (bytes/s)', fontsize=14)
plt.xlabel('Latency(ms)', fontsize=14)
plt.title('Time-And-Wait vs GBN(Latency&Throughput)', fontsize=16)


# 设置图例
plt.legend(fontsize=12)


# 在柱状图上添加数据标签
def add_labels(bars):
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width() / 2., height,
                 '{:.2f}'.format(height),
                 ha='center', va='bottom', fontsize=10)


add_labels(bars1)
add_labels(bars2)



# 设置图形大小
plt.gcf().set_size_inches(10, 6)

# 设置坐标轴刻度字体大小
plt.xticks(fontsize=12)
plt.yticks(fontsize=12)


# 显示图形
plt.show()