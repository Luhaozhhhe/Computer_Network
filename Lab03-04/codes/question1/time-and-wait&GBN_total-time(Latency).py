import matplotlib.pyplot as plt



loss_rates = [0, 120, 240, 360, 480]

# 停等机制延迟数据
stop_and_wait_delays = [860, 10971, 21093, 32175, 44091]

# 滑动窗口延迟数据
sliding_window_delays = [2071, 17183, 36971,51634, 74935]


# 绘制图形
plt.plot(loss_rates, stop_and_wait_delays, marker='o', label='Time-And-Wait', color='#6495ED')
plt.plot(loss_rates, sliding_window_delays, marker='s', label='GBN', color='#FFA500')

# 在数据点上添加标签
for x, y in zip(loss_rates, stop_and_wait_delays):
    plt.text(x, y, str(y), ha='right', va='bottom', fontsize=10)
for x, y in zip(loss_rates, sliding_window_delays):
    plt.text(x, y, str(y), ha='right', va='bottom', fontsize=10)


plt.xlabel('Latency(ms)', fontsize=14)
plt.ylabel('Total Time(ms)', fontsize=14)
plt.title('Time-And-Wait vs GBN(Latency&Total Time)', fontsize=16)
plt.legend(fontsize=12)
plt.grid(True)

# 设置图形大小
plt.gcf().set_size_inches(10, 6)

# 设置坐标轴刻度字体大小
plt.xticks(fontsize=12)
plt.yticks(fontsize=12)

plt.show()