FROM ubuntu:22.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      g++ make pkg-config \
      libboost-dev libboost-system-dev \
      ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN g++ -std=c++17 -O2 api_server.cpp orderbook.cpp -lboost_system -lpthread -o api_server

# Render provides PORT; fallback to 9000 for local testing
ENV PORT=9000

CMD ["./api_server"]
