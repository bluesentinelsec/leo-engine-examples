.PHONY: build run clean web

build:
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
	cmake --build build --parallel

# Run target: adjust per platform
run: build
ifeq ($(shell uname),Darwin)
	open build/leo-engine-showcase.app
else
	build/leo-engine-showcase
endif

clean:
	rm -rf build

web: clean
	docker build . -t dev:latest
	docker run -it -p 8000:8000 dev:latest

