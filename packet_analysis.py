import subprocess
import re

def analyze_pcap(filename):
    print(f"\nAnalyzing {filename} ...")

    # Extract packet timestamps and tcp lengths (data packets only)
    cmd = [
        "tshark",
        "-r", filename,
        "-Y", "tcp.port==8443 && tcp.len>0",
        "-T", "fields",
        "-e", "frame.time_epoch",
        "-e", "tcp.len"
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)
    lines = result.stdout.strip().split("\n")

    if not lines or lines == ['']:
        print("No data packets found.")
        return None

    times = []
    lengths = []

    for line in lines:
        parts = line.split("\t")
        if len(parts) == 2:
            times.append(float(parts[0]))
            lengths.append(int(parts[1]))

    packet_count = len(lengths)
    avg_segment_size = sum(lengths) / packet_count
    transfer_time_ms = (max(times) - min(times)) * 1000

    return {
        "packet_count": packet_count,
        "avg_segment_size": round(avg_segment_size, 2),
        "transfer_time_ms": round(transfer_time_ms, 2)
    }


files = [
    "mtu1500_FTUcapture.pcap",
    "mtu1200_FTUcapture.pcap",
    "mtu900_FTUcapture.pcap"
]

print("\n=== Packet-Level MTU Comparison ===")
print("MTU\tAvg TCP Segment\tPacket Count\tTransfer Time (ms)")

for f in files:
    data = analyze_pcap(f)
    if data:
        mtu = re.search(r'\d+', f).group()
        print(f"{mtu}\t{data['avg_segment_size']}\t\t{data['packet_count']}\t\t{data['transfer_time_ms']}")
