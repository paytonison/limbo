# practice

Small C++ exercises and experiments. Each program is standalone and can be
compiled directly with a C++17 compiler (no build system included).

## Requirements

- C++17 toolchain (`clang++` or `g++`)
- Eigen 3.3+ (header-only) for `practice/transformer.cpp`

## Quick build/run

```sh
# FizzBuzz
clang++ -std=c++17 -O2 practice/fizz.cpp -o fizz && ./fizz

# Rock / Paper / Scissors
clang++ -std=c++17 -O2 practice/roshambo.cpp -o roshambo && ./roshambo
```

## Projects

### `practice/fizz.cpp`

Classic FizzBuzz printing `1..10`.

### `practice/roshambo.cpp`

CLI Rock/Paper/Scissors against a RNG "robot".

### `practice/json-parser/`

`practice/json-parser/main.cpp` is a small CLI that:

- Validates input (`.jsonl` or `.csv`, or `-` for stdin)
- Counts lines
- Prints a preview of the first `N` lines (default `5`)

Build and run:

```sh
clang++ -std=c++17 -O2 practice/json-parser/main.cpp -o json-parser
./json-parser --help
./json-parser practice/json-parser/test.jsonl --preview 3
cat practice/json-parser/test.jsonl | ./json-parser - --preview=2
```

### `practice/corpus_stats/`

Directory/file scanner that totals:

- files scanned
- total lines
- total bytes

Optional flags include recursion (`-r/--recursive`), extension filters
(`--ext .jsonl`), error continuation (`-c/--continue`), and preview lines
(`--preview N`).

Build and run:

```sh
clang++ -std=c++17 -O2 practice/corpus_stats/src/*.cpp \
  -I practice/corpus_stats/include -o corpus_stats

./corpus_stats --help
./corpus_stats practice/ -r --ext .cpp --preview 5
```

### `practice/transformer.cpp`

A minimal transformer encoder block (multi-head self-attention + MLP) written
with Eigen; it runs a small dummy example and prints the output shape.

Build and run (adjust the Eigen include path):

```sh
clang++ -std=c++17 -O3 -I /path/to/eigen3 practice/transformer.cpp -o transformer
./transformer
```

## Contributing

If you spot a bug or want to add another small exercise, feel free to open an
issue or PR. Please keep changes small and include a build/run example.

## License

No repository-wide license is specified. Some files may include their own
license headers (for example, `practice/transformer.cpp`).
