FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN make

CMD ["./fss_manager", "-l", "./testing/manager_logfile.txt", "-c", "./testing/config_file.txt", "-n", "1"]
