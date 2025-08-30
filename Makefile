.PHONY: build

build:
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
	cmake --build build --parallel

run: build
	build/leo-engine-showcase

clean:
	rm -rf build
