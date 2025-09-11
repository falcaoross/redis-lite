# 🚀 Redis-Lite (C++)

A lightweight in-memory key–value store inspired by Redis, built in C++.  
Supports SET, GET, DEL, LRU eviction, and persistence via append-only log.

---

## ✨ Features

- Fast lookups (hash map based, O(1) operations)
- LRU eviction (removes least recently used items when memory is full)
- Persistence (data survives restarts using append-only log)
- Interactive CLI (SET, GET, DEL, INFO, EXIT)
- Cloud-ready (extendable with AWS S3 persistence and ECS deployment)

---

## 🛠 Build Instructions

### Linux/macOS

```bash
git clone https://github.com/<your-username>/redis-lite.git
cd redis-lite
mkdir build && cd build
cmake ..
cmake --build . -- -j$(nproc)
./redis-lite
```

### Windows (PowerShell)

````powershell
git clone https://github.com/<your-username>/redis-lite.git
cd redis-lite
mkdir build
cd build
cmake ..
cmake --build . -- /m
.\Debug\redis-lite.exe

---

## ☁️ AWS Deployment

### 1. Dockerize the App

Create a file named Dockerfile:

```dockerfile
FROM ubuntu:22.04
WORKDIR /app
COPY . .
RUN apt-get update && apt-get install -y cmake g++ make
RUN mkdir build && cd build && cmake .. && cmake --build .
CMD ["./build/redis-lite"]
````

Build the image:

```bash
docker build -t redis-lite .
```

---

### 2. Push to AWS ECR

```bash
aws ecr create-repository --repository-name redis-lite
aws ecr get-login-password --region <your-region> | docker login --username AWS --password-stdin <your-account>.dkr.ecr.<your-region>.amazonaws.com
docker tag redis-lite:latest <your-account>.dkr.ecr.<your-region>.amazonaws.com/redis-lite:latest
docker push <your-account>.dkr.ecr.<your-region>.amazonaws.com/redis-lite:latest
```

---

### 3. Deploy on ECS (Fargate)

1. Go to ECS Console → Create Cluster.
2. Create a Task Definition using the pushed ECR image.
3. Run as a Service → your Redis-Lite is live in the cloud.

---

### 4. Optional: Persist Logs to S3

To back up the append-only log:

```bash
aws s3 cp aof.log s3://<your-bucket>/redis-lite-backup.log
```

You can schedule this as a cron job or ECS sidecar task.
