# TLS Timing Proxy (Bidirectional TLS Performance Analyzer)

This project implements a **TLS Proxy in C++ using OpenSSL** that forwards encrypted traffic between a client and a backend server while measuring detailed timing information.

It helps analyze:

- Network waiting time
- TLS decryption time
- TLS encryption time
- Forward path performance (Client to Server)
- Reverse path performance (Server to Client)
- Total file transfer time

This project is useful for learning TLS internals, performance testing, and research experiments (e.g., Raspberry Pi performance analysis).

---

# Project Architecture

```
Client  to  Proxy (Port 8443)  to  Backend Server (Port 9443)
```

The proxy sits in the middle and measures timing for each stage.

---

# System Requirements

You need:

- Linux (Ubuntu / Raspberry Pi OS recommended)
- g++
- OpenSSL
- Git (for uploading to GitHub)

---

# STEP 1 Install Required Packages

On Ubuntu / Raspberry Pi:

```bash
sudo apt update
sudo apt install g++ make libssl-dev openssl git
```

Verify OpenSSL:

```bash
openssl version
```

---

# STEP 2 Generate TLS Certificate (Required)

We need a self-signed certificate for testing.

Run:

```bash
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes
```

Press ENTER for all questions (for testing).

This creates:

- `cert.pem` (certificate)
- `key.pem` (private key)

---

#  STEP 3 — Create Project Folder

```bash
mkdir tls-timing-proxy
cd tls-timing-proxy
```

Put your `proxy.cpp` file inside this folder.

---

# STEP 4 Compile the Proxy

```bash
g++ proxy.cpp -o proxy -lssl -lcrypto
```

If successful, a file named `proxy` will be created.

---

# STEP 5 Run Backend TLS Server

Open Terminal 1:

```bash
openssl s_server -accept 9443 -cert cert.pem -key key.pem
```

This starts a TLS backend server on port 9443.

---

# STEP 6 Run the Proxy

Open Terminal 2:

```bash
./proxy
```

You should see:

```
Waiting for client...
```

---

# STEP 7 Run the Client

Open Terminal 3:

```bash
openssl s_client -connect <YOUR_IP>:8443
```

Example:

```bash
openssl s_client -connect 192.168.1.10:8443
```

Now type:

```
Hello
```

Press ENTER.

You will see timing results in the proxy terminal.

---

# Example Output

```
----- CLIENT to SERVER -----
Bytes: 128
Waiting time: 1700 us
Decrypt time: 95 us
Encrypt time: 82 us
Total forward processing: 190 us

----- SERVER to CLIENT -----
Bytes: 256
Waiting time: 2400 us
Decrypt time: 90 us
Encrypt time: 85 us
Total reverse processing: 185 us

=====================================
Total file transfer time: 4 ms
```

---

# What Do These Measurements Mean?

Waiting time  
Time spent idle waiting for network data.

Decrypt time  
Time spent inside SSL_read() decrypting TLS.

Encrypt time  
Time spent inside SSL_write() encrypting TLS.

Total forward processing  
Decrypt + Encrypt time (Client to Server).

Total reverse processing  
Decrypt + Encrypt time (Server to Client).

Total file transfer time  
Total duration of the communication session.

---

# Send Large Data Test

To send 100 KB:

```bash
dd if=/dev/zero bs=1K count=100 | openssl s_client -connect <YOUR_IP>:8443
```


# Troubleshooting

If proxy cannot connect:
- Make sure backend server is running first.

If certificate error:
- Make sure cert.pem and key.pem exist.

If permission denied:
```bash
chmod +x proxy
```