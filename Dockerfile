
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .


RUN g++ -O2 -std=c++20 -I include -I third_party -I third_party/nlohmann src/*.cpp -o server -DCPPHTTPLIB_OPENSSL_SUPPORT -lssl -lcrypto -lpthread


FROM ubuntu:22.04

RUN apt-get update && apt-get install -y libssl3 ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/server .

EXPOSE 8080

CMD ["./server"]