BUILD=build
PROJECT=SudokuLSGA

.PHONY: init clean test run

all: cl cpu

init:
	@cmake -S . -B $(BUILD)

cl: init
	@#cp src/cl/kernel.cl $(BUILD)/src/cl/kernel.cl
	@cmake --build $(BUILD) --target $(PROJECT)CL -j 10

cpu: init
	@cmake --build $(BUILD) --target $(PROJECT)CPU -j 10

clean:
	rm -rf .cache Testing build