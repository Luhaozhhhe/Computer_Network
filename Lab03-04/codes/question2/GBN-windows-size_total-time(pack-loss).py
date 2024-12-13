import matplotlib.pyplot as plt
import numpy as np

# Packet loss rates (%)
packet_loss = np.array([0, 20, 40, 60, 80])

# Transmission times (ms) for different window sizes
window_4 = np.array([1642, 3317, 4792, 5148, 5397])
window_16 = np.array([2013, 2275, 2431, 2754, 3157])
window_32 = np.array([2071, 2174, 2217, 2364, 2497])

# Create the plot
plt.figure(figsize=(10, 6))

# Plot transmission times for each window size
plt.plot(packet_loss, window_4, marker='o', label='Window Size 4', color='blue', linestyle='-', linewidth=2)
plt.plot(packet_loss, window_16, marker='s', label='Window Size 16', color='orange', linestyle='-', linewidth=2)
plt.plot(packet_loss, window_32, marker='^', label='Window Size 32', color='green', linestyle='-', linewidth=2)

# Add data labels for each point with a slight vertical offset
for i in range(len(packet_loss)):
    plt.text(packet_loss[i], window_4[i] + 50, f'{window_4[i]:.0f}', ha='center', va='bottom', fontsize=10, color='blue')
    plt.text(packet_loss[i], window_16[i] + 50, f'{window_16[i]:.0f}', ha='center', va='bottom', fontsize=10, color='orange')
    plt.text(packet_loss[i], window_32[i] + 50, f'{window_32[i]:.0f}', ha='center', va='bottom', fontsize=10, color='green')

# Add labels and title
plt.xlabel('Packet Loss Rate (%)', fontsize=14)
plt.ylabel('Total Time (ms)', fontsize=14)
plt.title('Total Time for Different Window Sizes and Packet Loss', fontsize=16)

# Add grid lines
plt.grid(True, which='both', linestyle='--', alpha=0.7)

# Show legend
plt.legend()

# Display the plot
plt.tight_layout()
plt.show()
