import matplotlib.pyplot as plt
import numpy as np

# Packet loss rates (%)
packet_loss = np.array([0, 20, 40, 60, 80])

# Throughput (bytes/s) for different window sizes
window_4 = np.array([3654.05, 1808.85, 1252.08, 1165.49, 1111.72])
window_16 = np.array([2980.6, 2637.33, 2468.09, 2178.63, 1900.52])
window_32 = np.array([2897.13, 2759.87, 2706.34, 2538.05, 2402.87])

# Set the width of the bars
bar_width = 0.2

# Set x-axis positions for the bars
x = np.arange(len(packet_loss))

# Create the plot
plt.figure(figsize=(10, 6))

# Plot the bars for each window size
plt.bar(x - bar_width, window_4, width=bar_width, label='Window Size 4', color='blue', alpha=0.7)
plt.bar(x, window_16, width=bar_width, label='Window Size 16', color='orange', alpha=0.7)
plt.bar(x + bar_width, window_32, width=bar_width, label='Window Size 32', color='green', alpha=0.7)

# Add data labels for each bar
for i in range(len(packet_loss)):
    plt.text(x[i] - bar_width, window_4[i] + 50, f'{window_4[i]:.2f}', ha='center', va='bottom', fontsize=10, color='blue')
    plt.text(x[i], window_16[i] + 50, f'{window_16[i]:.2f}', ha='center', va='bottom', fontsize=10, color='orange')
    plt.text(x[i] + bar_width, window_32[i] + 50, f'{window_32[i]:.2f}', ha='center', va='bottom', fontsize=10, color='green')

# Add labels and title
plt.xlabel('Packet Loss Rate (%)', fontsize=14)
plt.ylabel('Throughput (bytes/s)', fontsize=14)
plt.title('Throughput for Different Window Sizes and Packet Loss Rate', fontsize=16)

# Set x-ticks to the center of the grouped bars
plt.xticks(x, packet_loss, fontsize=12)

# Add grid lines
plt.grid(True, axis='y', linestyle='--', alpha=0.7)

# Show legend
plt.legend()

# Display the plot
plt.tight_layout()
plt.show()
