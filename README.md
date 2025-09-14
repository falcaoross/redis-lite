Redis-lite

Features:

- AES-256-CBC encrypted snapshots (`dump.rdb`)
- Hybrid persistence: Snapshot + Append-Only File (`aof.log`)
- LRU cache with configurable capacity
- Commands supported: `SET`, `GET`, `DEL`, `INFO`, `SAVE`, `EXIT`
- Simple command-line interface

Tech Stack:
Language: C++17
Libraries: OpenSSL (AES), STL
Build System: CMake + vcpkg (Windows)

---

🏗️ Local Build Instructions (Windows)

1. Prerequisites

- Visual Studio 2022 or later (with C++ desktop development)
- CMake >= 3.20
- vcpkg package manager
- OpenSSL (installed via vcpkg)

. Clone Project

```powershell
git clone https://github.com/falcaoross/redis-lite.git
cd redis-lite
mkdir build
cd build
```

3.  Configure vcpkg Toolchain

```powershell
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
```

4. Build Project

```powershell
cmake --build . --config Release
```

5. Run

```powershell
cd Release
.\redis-lite.exe
```

---

💾 Commands

| Command         | Description                           |
| --------------- | ------------------------------------- |
| `SET key value` | Sets a key-value pair                 |
| `GET key`       | Retrieves a value                     |
| `DEL key`       | Deletes a key                         |
| `INFO`          | Shows cache info (size/capacity)      |
| `SAVE`          | Manually save snapshot + AOF          |
| `EXIT`          | Exit program (flushes snapshot + AOF) |

---

🔑 AES Snapshot & Hybrid AOF

- Snapshots (`dump.rdb`) are AES-256-CBC encrypted.
- Append-Only File (`aof.log`) stores incremental commands.
- Automatic snapshot occurs every `SNAPSHOT_INTERVAL` commands (default = 5).

---

☁️ Scaling on AWS

1. Package as Container

Create a Dockerfile:

```dockerfile
FROM mcr.microsoft.com/windows/servercore:ltsc2022
WORKDIR /app
COPY Release/redis-lite.exe .
COPY data ./data
ENTRYPOINT ["redis-lite.exe"]
```

Build & test locally:

```powershell
docker build -t redis-lite .
docker run -it redis-lite
```

---

2. Deploy on AWS

Option A: EC2

1. Launch Windows or Linux EC2 instance.
2. Install dependencies (C++ runtime, OpenSSL if Linux).
3. Copy the executable or container.
4. Run as a service or background process.

Option B: ECS / Fargate

1. Push Docker image to ECR:

```bash
aws ecr create-repository --repository-name redis-lite
aws ecr get-login-password | docker login --username AWS --password-stdin <aws_account_id>.dkr.ecr.<region>.amazonaws.com
docker tag redis-lite:latest <aws_account_id>.dkr.ecr.<region>.amazonaws.com/redis-lite:latest
docker push <aws_account_id>.dkr.ecr.<region>.amazonaws.com/redis-lite:latest
```

2. Create **ECS Cluster** and run task using this image.
3. Configure **persistent storage** using EFS for `dump.rdb` and `aof.log`.

---

3. Scaling Considerations

- **Horizontal scaling:** Deploy multiple instances behind a load balancer.
- **Persistence:** Use shared storage (EFS / S3) to sync snapshots and AOF across instances.
- **Backups:** Periodically copy `dump.rdb` + `aof.log` to S3 for disaster recovery.
- **Monitoring:** Use CloudWatch to track command rates, cache size, errors.

---

4. Notes

- This is a Redis clone for educational purposes.
- Not production-ready for high concurrency or durability.
- AES keys are hardcoded — for production, use KMS or environment variables.
