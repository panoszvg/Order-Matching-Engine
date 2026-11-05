# Order-Matching Engine

A **C++17 trading engine** with TCP order ingress, pluggable matching strategies, JSON snapshots, and a Python Flask API.

## Features

- Pluggable matching strategies:
  - **Price-time priority** — default strategy for most symbols
  - **Top/priority pro-rata** — priority slice to the earliest order, remaining quantity split proportionally across resting orders at a price level, remainder distributed round-robin in time order
- Boost.Asio TCP server (ports 9000-9003, 9100)
- Simple & FIX-like message parsers
- Snapshot export (JSON) for API/UI
- Google Test/Mock coverage across core, strategy, messages, and TCP handlers
- Dockerized for full-stack spin-up

## Project Structure
```
Order-Matching-Engine/
├── src/                           # Core C++ source code for the matching engine
|    ├── order_matching_engine/    # C++17 engine: book, strategy, TCP (Boost.Asio), parsers
|	 └── flask_api/                # Python Flask REST API (5000)
├── docker-compose.yml             # Multi-container setup
├── .gitignore
└── README.md
```

## Quick Start
1. Clone the repository:
   ```bash
   git clone https://github.com/<your-username>/Order-Matching-Engine.git
   cd Order-Matching-Engine
   ```
2. Build and run with Docker:
   ```bash
   docker-compose up --build
   ```

## Ports

| Port | Purpose |
| --- | --- |
| 9000-9001 | JSON order ingress |
| 9002-9003 | FIX 4.4 order ingress |
| 9100 | Admin commands (snapshot export/get) |
| 5000 | Flask REST API |

## Running Tests

```bash
cd src/order_matching_engine
make test
```

## Trying it out

A few small scripts under `src/order_matching_engine/tcp/` send traffic to a running engine:

- `client.py`: sends random JSON/FIX orders across all symbols
- `prorata_client.py`: sends random orders clustered on a coarse-tick symbol (`SOFR`) to exercise pro-rata matching
- `export.py <SYMBOL>`: fetches and exports a book snapshot via the admin port

## Status
Core matching (price-time + pro-rata), FIX/JSON ingress, and snapshot export are in place and tested. Next up: latency measurement and profiling.
