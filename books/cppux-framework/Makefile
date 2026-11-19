# Build harness for this book's sources.
# One file pair per algorithm under src/, programs in src/, tests/ and benchmarks/.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Isrc

ALGO_SRC  := $(filter-out %/demo.cpp %/teaching.cpp,$(wildcard src/*.cpp))
HDRS      := $(wildcard src/*.hpp)
SRC_PROGS := $(filter-out $(ALGO_SRC),$(wildcard src/*.cpp))
TEST_SRC  := $(wildcard tests/*.cpp)
BENCH_SRC := $(wildcard benchmarks/*.cpp)
PROGS     := $(patsubst src/%.cpp,bin/%,$(SRC_PROGS)) \
             $(patsubst tests/%.cpp,bin/%,$(TEST_SRC)) \
             $(patsubst benchmarks/%.cpp,bin/%,$(BENCH_SRC))

all: $(PROGS)
	@echo "built $(words $(PROGS)) programs into bin/"

bin/%: src/%.cpp $(ALGO_SRC) $(HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $< $(ALGO_SRC)

bin/%: tests/%.cpp $(ALGO_SRC) $(HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $< $(ALGO_SRC)

bin/%: benchmarks/%.cpp $(ALGO_SRC) $(HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $< $(ALGO_SRC)

demo:     ; @$(MAKE) --no-print-directory bin/demo && ./bin/demo
teaching: ; @$(MAKE) --no-print-directory bin/teaching && ./bin/teaching

test: $(patsubst tests/%.cpp,bin/%,$(TEST_SRC))
	@for t in $(patsubst tests/%.cpp,bin/%,$(TEST_SRC)); do echo "== $$t"; ./$$t || exit 1; done
	@echo "all tests passed"

bench: $(patsubst benchmarks/%.cpp,bin/%,$(BENCH_SRC))
	@for b in $(patsubst benchmarks/%.cpp,bin/%,$(BENCH_SRC)); do echo "== $$b"; ./$$b; done

clean: ; rm -rf bin

.PHONY: all demo teaching test bench clean
