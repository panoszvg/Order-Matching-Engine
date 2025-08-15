# Order-Matching Engine

A WIP **C++17 trading engine** with TCP order ingress, price–time matching, JSON snapshots, and a Python Flask API. Redis/WebSocket integration is planned.

## Features
- Price–time priority order matching
- Boost.Asio TCP server (ports 9000-9003, 9100)
- Simple & FIX-like message parsers
- Snapshot export (JSON) for API/UI
- Dockerized for full-stack spin-up
- (Planned) Redis Pub/Sub + WebSocket

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

## Roadmap
- Redis + WebSocket live updates
- React UI for book/trade display

## Status
🚧 Work in progress