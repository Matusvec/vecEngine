.PHONY: all clean run

all:
	cmake -B build
	cmake --build build

run: all
	./build/flight

clean:
	rm -rf build
