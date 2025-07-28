# Stage 1: Build
FROM debian:bullseye AS build

RUN apt-get update && apt-get install -y \
    g++ make cmake git \
    libspdlog-dev libboost-all-dev libfmt-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ ./src/
WORKDIR /app/src
RUN make

# Stage 2: Runtime
FROM debian:bullseye-slim AS runtime

RUN apt-get update && apt-get install -y \
    libspdlog-dev libboost-system-dev libfmt7 \
	procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/src .

EXPOSE 9000
EXPOSE 9001
EXPOSE 9002
EXPOSE 9003
CMD ["./order_matching_engine"]
