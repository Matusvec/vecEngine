.PHONY: all clean run test demo

all:
	cmake -B build
	cmake --build build

run: all
	./build/flight

# Same as `run` — the assignment spec asks for a `make demo` target.
demo: run

# Build + execute the unit tests. Returns nonzero on any failure.
test:
	cmake -B build
	cmake --build build --target flight_tests
	./build/flight_tests

clean:
	rm -rf build

blackbox: all
	./build/blackbox
