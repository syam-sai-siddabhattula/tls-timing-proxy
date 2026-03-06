# TLS Proxy MTU Fragmentation Analysis

A step-by-step experimental study of how MTU (Maximum Transmission Unit) affects:

- TCP packet fragmentation
- Packet count
- TLS proxy processing time
- Statistical confidence intervals

This project demonstrates how network-layer configuration influences application-layer TLS performance.

---

# Project Overview

We built a small TLS proxy system and measured:

1. Packet-level behavior (tcpdump / Wireshark)
2. TLS processing time (encryption/decryption timing)
3. Statistical reliability (95% confidence intervals)

The goal is to prove:

Reducing MTU increases packet fragmentation, increases packet count, and impacts TLS proxy processing time.

---

# Hardware Setup

## Client Machine
- Raspberry Pi 3
- Runs TLS client
- Sends payloads

## Proxy + Backend Machine
- Raspberry Pi 5
- Runs:
  - TLS Proxy
  - OpenSSL backend server
  - tcpdump packet capture

Both connected via Ethernet (`eth0`).

---

# Software Requirements

Install on BOTH machines:

```bash
sudo apt update
sudo apt install openssl
sudo apt install tcpdump
sudo apt install wireshark
sudo apt install tshark
sudo apt install python3 python3-pandas python3-matplotlib python3-numpy
sudo apt install ethtool
```

---

# Generate TLS Certificate (Required)

Run on Proxy machine:

```bash
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes
```

Example input:

```
Country Name: US
State: Missouri
Locality: St Louis
Organization: TLS-Lab
Common Name: localhost
```

Generated files:

```
cert.pem
key.pem
```

These must be in the same directory as:

```
openssl s_server
./proxy
```

---

# Disable TCP Offloading (Very Important)

Run on BOTH machines:

```bash
sudo ethtool -K eth0 tso off
sudo ethtool -K eth0 gso off
sudo ethtool -K eth0 gro off
```

Verify:

```bash
ethtool -k eth0
```

All offloading features must show `off`.

---

# System Architecture

```
Client (Pi3)
    ↓
TLS Proxy (Pi5 :8443)
    ↓
OpenSSL Backend (Pi5 :9443)
```

---

# Compilation

## Compile Proxy

```bash
g++ -O3 -march=native -DNDEBUG proxy.cpp -o proxy -lssl -lcrypto
```

## Compile Client

```bash
g++ -O3 -march=native -DNDEBUG client.cpp -o client -lssl -lcrypto
```

---

# Experiment Design

## Payload Strategy

```
Start: 128 bytes
Step:  128 bytes
Stop:  3000 bytes
```

Fragmentation boundaries:

| MTU | Approx TCP Payload |
|------|-------------------|
| 1500 | ~1460 bytes |
| 1200 | ~1160 bytes |
| 900  | ~860 bytes  |

When payload crosses this value, fragmentation begins.

---

# Running The Experiment

## Step 1 — Set MTU

Example for MTU 1500:

```bash
sudo ip link set dev eth0 down
sudo ip link set dev eth0 mtu 1500
sudo ip link set dev eth0 up
```

Verify:

```bash
ip link show eth0
```

Repeat full experiment for MTU 1500, 1200, and 900.

---

## Step 2 — Start Backend

```bash
openssl s_server -accept 9443 -cert cert.pem -key key.pem -quiet -tls1_2 -no_ticket
```

---

## Step 3 — Start Proxy

```bash
./proxy
```

---

## Step 4 — Start Packet Capture

```bash
sudo tcpdump -i eth0 port 8443 -w mtu1500_capture.pcap
```

---

## Step 5 — Run Client

```bash
./client
```

---

## Step 6 — Stop Capture

Press:

```
Ctrl + C
```

---

## Step 7 — Save Results

```bash
mv steady_state_results.csv ftu_1500_steady_state_results.csv
```

Repeat for MTU 1200 and 900.

---

# Packet-Level Analysis

To perform packet-level validation of MTU behavior, run:

```bash
python3 packet_analysis.py
```

This script processes the captured `.pcap` files and extracts:

- Average TCP segment size
- Total packet count
- Overall transfer duration

---

## Figure 2: Packet-Level MTU Comparison

The screenshot below shows the actual packet-level results obtained from the captured traffic.

![Packet Analysis Screenshot](Screenshots/Figure_1.png)

---

## Observed Results

The measured data confirms the impact of MTU on TCP segmentation:

- **MTU 1500**
  - Largest average TCP segment size
  - Lowest packet count
- **MTU 1200**
  - Reduced segment size
  - Increased packet count compared to 1500
- **MTU 900**
  - Smallest segment size
  - Highest packet count

The results demonstrate that decreasing MTU directly reduces the maximum TCP segment size and increases the total number of packets required to transmit the same amount of data.

Although overall transfer time does not increase strictly in proportion to packet count (due to LAN conditions and system-level factors), the structural fragmentation behavior is clearly validated.

This packet-level confirmation is critical because it verifies that:

- MTU settings were correctly applied
- TCP segmentation behavior changed as intended
- Fragmentation increases with smaller MTU values

These findings provide the network-layer evidence necessary to interpret the TLS proxy timing results.

---

# Timing Graph with 95% Confidence Interval

To generate the timing analysis graph, run:

```bash
python3 plot_graph.py
```

This script processes the steady-state result files and computes:

- Mean TLS proxy processing time for each payload size
- Standard deviation
- 95% Confidence Interval (CI)
- Comparative analysis across MTU values (1500, 1200, 900)

The resulting graph visualizes how MTU configuration influences TLS proxy performance.

![TLS Timing Graph](Screenshots/Figure_2.png)

## Graph Description

- **X-axis:** Payload Size (Bytes)
- **Y-axis:** Mean Total Processing Time (µs)
- **Error Bars:** 95% Confidence Interval
- **Lines:** Represent different MTU configurations

## Interpretation

- As MTU decreases, the fragmentation boundary shifts to smaller payload sizes.
- A noticeable increase in processing time appears when payload size exceeds the effective TCP payload limit.
- Lower MTU values cause earlier fragmentation, which leads to increased packet handling overhead.
- The confidence intervals are relatively small, indicating stable and reproducible measurements.

This graph demonstrates the relationship between network-layer MTU configuration and application-layer TLS proxy performance.

# Final Outcome

This project demonstrates:

- Controlled MTU-based fragmentation behavior
- Packet-level validation
- Application-layer TLS timing analysis
- Statistical reliability
- Reproducible methodology