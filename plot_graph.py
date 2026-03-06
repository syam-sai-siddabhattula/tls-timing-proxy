import pandas as pd
import matplotlib.pyplot as plt

# Load datasets
df1500 = pd.read_csv("ftu_1500_steady_state_results.csv")
df1200 = pd.read_csv("ftu_1200_steady_state_results.csv")
df900 = pd.read_csv("ftu_900_steady_state_results.csv")

# Group by payload size and compute mean total_us
g1500 = df1500.groupby("bytes")["total_us"].mean()
g1200 = df1200.groupby("bytes")["total_us"].mean()
g900 = df900.groupby("bytes")["total_us"].mean()

plt.figure()

plt.plot(g1500.index, g1500.values, marker='o')
plt.plot(g1200.index, g1200.values, marker='o')
plt.plot(g900.index, g900.values, marker='o')

# REMOVE log scale
plt.xscale("linear")

plt.xlabel("Payload Size (Bytes)")
plt.ylabel("Mean Total Processing Time (µs)")
plt.title("TLS Proxy Processing Time under Different MTU")
plt.legend(["MTU 1500", "MTU 1200", "MTU 900"])
plt.grid(True)

plt.show()
