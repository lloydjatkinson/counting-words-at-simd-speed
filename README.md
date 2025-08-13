# Counting Words at SIMD Speed
> My blog post: [Counting Words at SIMD Speed](https://healeycodes.com/counting-words-at-simd-speed)
<br>

Whitespace-delimited word count implemented five ways.

Results from my Apple M1 Pro:

- Python (byte loop): `89.6 s`
- Python + re: `13.7 s`
- C (scalar loop): `1.205 s`
- C + NEON: `249 ms`
- C + NEON + threads: `181 ms` (5.52 GiB/s)

Includes a 1 GiB corpus generator and Hyperfine benchmarks.

```
./bench.sh
Benchmark 1: python3 0_mvp.py bench.txt
  Time (mean ± σ):     89.556 s ±  7.214 s    [User: 87.205 s, System: 0.484 s]
  Range (min … max):   82.393 s … 96.820 s    3 runs

Benchmark 2: python3 1_c_regex.py bench.txt
  Time (mean ± σ):     13.739 s ±  0.136 s    [User: 13.433 s, System: 0.158 s]
  Range (min … max):   13.659 s … 13.896 s    3 runs

Benchmark 3: ./bin/2_mvp bench.txt
  Time (mean ± σ):      1.205 s ±  0.008 s    [User: 1.015 s, System: 0.115 s]
  Range (min … max):    1.198 s …  1.214 s    3 runs

Benchmark 4: ./bin/3_simd bench.txt
  Time (mean ± σ):     249.2 ms ±   6.3 ms    [User: 79.5 ms, System: 100.5 ms]
  Range (min … max):   243.8 ms … 262.6 ms    11 runs

Benchmark 5: ./bin/4_threads bench.txt
  Time (mean ± σ):     181.1 ms ±   4.1 ms    [User: 96.5 ms, System: 93.5 ms]
  Range (min … max):   177.4 ms … 193.7 ms    16 runs

Summary
  ./bin/4_threads bench.txt ran
    1.38 ± 0.05 times faster than ./bin/3_simd bench.txt
    6.65 ± 0.16 times faster than ./bin/2_mvp bench.txt
   75.84 ± 1.86 times faster than python3 1_c_regex.py bench.txt
  494.38 ± 41.35 times faster than python3 0_mvp.py bench.txt
```

<br>

### Files
- `0_mvp.py`, `1_c_regex.py`: Python baselines
- `2_mvp.c`: scalar C
- `3_simd.c`: ARM NEON SIMD
- `4_threads.c`: multithreaded NEON
- `setup_benchmark.c`: generates `bench.txt` (1 GiB)
- `bench.sh`: builds everything and runs benchmarks

<br>

## Requirements
- macOS on Apple Silicon (ARM64)
- `clang`, `python3`, `hyperfine`

<br>

## Quick start
```bash
bash bench.sh
```

This produces `bench.txt`, builds binaries into `bin/`, and benchmarks all implementations.

Note: the Python ones are very slow – you might want to comment those out!
