# E20 Cache Simulator

## Overview
This project is an implementation of an **E20 cache simulator**, designed to monitor the behavior of a simulated processor’s memory subsystem. It builds on a previously implemented E20 instruction set simulator and extends it to model cache hierarchy, hits/misses, and write policies.  

The simulator supports **L1 and L2 cache configurations**, user-defined associativity, block sizes, and cache sizes, and outputs detailed logs of cache activity (`HIT`, `MISS`, `SW`, `LW`) during program execution.

---

## Features
- Supports **direct-mapped**, **set-associative**, and **fully associative** cache configurations.  
- Implements **write-through** and **write-allocate** policies.  
- Handles up to **two cache levels (L1 and L2)** with configurable parameters.  
- Logs all load (`lw`) and store (`sw`) cache events with program counter, address, and row.  
- Follows **LRU (Least Recently Used)** replacement policy for associative caches.  
- Command-line configuration using the `--cache` flag.

---

## Design & Implementation
The simulator is organized around two main classes: **`Row`** and **`Cache`**.

### Row Class
Represents a single cache row and manages tag tracking and block replacement:
- Maintains a private `std::list` for LRU tracking.  
- Implements:
  - `parse_block()` – checks if a tag exists and updates LRU.  
  - `move_to_latest_block()` – reuses parsing logic for clarity.  
  - `direct()` – handles direct-mapped cache logic.  
  - `set()` – handles associative caches, performing evictions when full.  

### Cache Class
Models an entire cache level (L1 or L2):
- Stores configuration (size, associativity, block size).  
- Contains a vector of `Row` objects.  
- Implements:
  - `access()` – checks cache hit/miss and returns the result.  
  - `write()` – writes data, performing direct/set access as appropriate.  
  - `insert()` – inserts a block without logging.  
  - `get_row()` – computes cache row from block ID for modular access.  

### Simulation Logic
The main `simulate()` function handles:
- `lw` (load word): checks L1, then L2 on misses, updating cache and PC.  
- `sw` (store word): performs write-through on both L1 and L2 if present.  

---

## Debugging & Testing
Testing was performed using **30 autograder test cases**, all of which passed successfully.  
The autograder provided detailed output that was used to identify and correct logic errors — particularly involving dual-cache scenarios.  

Key debugging strategies:
- Added `+` and `-` markers in outputs to trace pattern differences.  
- Used breakpoints within `simulate()` for `lw` and `sw` handling.  
- Corrected nested conditional logic causing duplicate `L2` logs.  
- Validated correctness across multiple cache input configurations.

---

## Tools & Resources
- **Language:** C++  
- **Libraries:** `<list>` (for LRU tracking)  
- **Compiler:** `g++` (via Anubis online playground and local environment)  
- **Reference:** [cplusplus.com](https://cplusplus.com/reference/list/list/) for `std::list` methods (`begin()`, `end()`, `pop_front()`, `push_back()`, etc.)  
- **Development Environment:** CLion & Anubis Playground  

---

## Reflection
This project deepened my understanding of computer architecture, especially in simulating real-world caching mechanisms such as LRU replacement and multi-level memory systems.  
Building upon my Project 1 simulator, I improved class modularity, debugging workflow, and performance tracking through iterative testing.

---

## How to Run
Compile all C++ source files and execute with the specified cache configuration:
```bash
g++ -Wall -o simcache *.cpp
./simcache test.bin --cache 2,1,1
