#!/usr/bin/env bash

# Detect platform and set RID
case "$(uname -s)" in
    Linux*)   RID="linux-x64" ;;
    Darwin*)  RID="osx-arm64" ;;
    MINGW*|CYGWIN*|MSYS*) RID="win-x64" ;;
    *) echo "Unsupported platform: $(uname -s)" && exit 1 ;;
esac

# Set executable extension for Windows
if [[ "$RID" == "win-x64" ]]; then
    EXE_EXT=".exe"
else
    EXE_EXT=""
fi

echo "Building for platform: $RID"

mkdir -p ./bin

# --- build programs ---
clang -Wall -Wextra -Wpedantic -O3 -funroll-loops -march=native -o ./bin/2_mvp 2_mvp.c
clang -Wall -Wextra -Wpedantic -O3 -funroll-loops -march=native -o ./bin/3_simd 3_simd.c
clang -Wall -Wextra -Wpedantic -O3 -funroll-loops -march=native -pthread -o ./bin/4_threads 4_threads.c

# Publish the .NET version (self-contained, single-file) for current platform
pushd ./dotnet/src/WordCount.VersionOne >/dev/null
dotnet publish -c Release -r "$RID" --self-contained true \
  -p:PublishSingleFile=true -p:DebugType=None -p:StripSymbols=true >/dev/null
popd >/dev/null

DOTNET_APP="./dotnet/src/WordCount.VersionOne/bin/Release/net9.0/$RID/publish/WordCount.VersionOne$EXE_EXT"

# Make sure the .NET executable has execute permissions (Unix only)
if [[ "$RID" != "win-x64" ]]; then
    chmod +x "$DOTNET_APP"
fi

# --- build benchmark generator ---
clang -Wall -Wextra -Wpedantic -O3 -funroll-loops -march=native -o ./bin/setup_benchmark setup_benchmark.c

# --- generate benchmark input ---
rm -f bench.txt
./bin/setup_benchmark

# --- sanity checks (same expected answer across impls) ---
EXPECTED=65076996

# C versions
./bin/2_mvp bench.txt | grep -q "$EXPECTED" || { echo "2_mvp failed"; exit 1; }
./bin/3_simd bench.txt | grep -q "$EXPECTED" || { echo "3_simd failed"; exit 1; }
./bin/4_threads bench.txt | grep -q "$EXPECTED" || { echo "4_threads failed"; exit 1; }

# .NET
"$DOTNET_APP" --file bench.txt | grep -q "$EXPECTED" || { echo "dotnet failed"; exit 1; }

# --- run benchmark ---
hyperfine --warmup 2 --min-runs 3 \
    'python3 0_mvp.py bench.txt' \
    'python3 1_c_regex.py bench.txt' \
    './bin/2_mvp bench.txt' \
    './bin/3_simd bench.txt' \
    './bin/4_threads bench.txt' \
    "$DOTNET_APP --file bench.txt"