# Order Book Simulator

This is a simple simulation of an order book. I built it to better understand C++ data structures and how they’re used in high frequency trading. It started as a CLI tool and now also has a tiny HTTP server + web UI.

## Engine design
I first used two deques filled with `Order` objects. That felt right—just two queues I could add to from both ends and keep sorted. But resorting each deque after every limit order was O(n log n), which isn’t fast.

I then redesigned the engine to use two maps (buys and asks) that map prices to deques of `Order` objects. Now limit order insertions are O(log n). This change alone knocked an entire decimal place off the nanoseconds.

## Benchmarks (std::chrono)
1000 random orders, 75% limit / 25% market:

- 2 deques: 5,326,678 ns total (~5,326 ns/order)
- map + deque per price: see table below

| Orders | Build | Total ns | ns/order |
|--------|-------|----------|---------|
| 1,000  | default | 763,993 | ~764 |
| 1,000  | -O2     | 101,296 | ~101 |
| 1,000  | -O3     | 193,240 | ~193 |
| 10,000 | default | 6,196,660 | ~619 |
| 10,000 | -O2     | 1,384,500 | ~138 |
| 10,000 | -O3     | 1,370,503 | ~137 |

(O2 beat O3 at 1k orders; they’re similar at 10k.) Raw stamps live in `stamps.txt`.

## Running
- CLI: build and run `CLI_interface.cpp` with `orderbook.cpp`. The CLI uses ANSI colors intended for bash; untested elsewhere.
- HTTP/UI: build and run `api_server.cpp` (or `Dockerfile`), then open the web UI served from `/` to place orders and see the book/depth chart.

## Notes
- Matching is price/time priority using `std::map` for price levels and `deque` for FIFO at each level.
- This is a learning tool. No auth/rate limits; don’t expose it publicly without safeguards.
- What I learned: swapping containers matters a lot; compiler flags (-O2) matter; simple profiling with `std::chrono` is enough to see meaningful gains.
