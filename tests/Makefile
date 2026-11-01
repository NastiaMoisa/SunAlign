CC = gcc
CFLAGS = -std=c11 -O0 -g --coverage -Wall -Wextra -Werror -I../firmware/App -I../firmware/Devices
HEADERS = $(wildcard ../firmware/App/*.h ../firmware/Devices/*.h)
SOURCES = test_sunalign.c ../firmware/App/sunalign.c ../firmware/Devices/mpu6050.c
.PHONY: test coverage clean
test: build/tests build/lab3 build/serial build/clock_pair build/timebase
	rm -f build/*.gcda
	./build/tests
	./build/lab3
	./build/serial
	./build/clock_pair
	./build/timebase
build/tests: $(SOURCES) $(HEADERS) fixtures/mpu6050_capture.h
	mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) -lm -o $@
coverage: test
	gcovr --root .. --filter '../firmware/(App|Devices)/' --txt --txt-metric branch --output ../docs/metrics/lab3/coverage-branches.txt
	gcovr --root .. --filter '../firmware/(App|Devices)/' --txt --output ../docs/metrics/lab3/coverage-lines.txt
	gcovr --root .. --filter '../firmware/(App|Devices)/' --json-summary --output ../docs/metrics/lab3/coverage.json --fail-under-line 90 --fail-under-branch 75
clean:
	rm -rf build

build/lab3: test_lab3.c ../firmware/App/calibration.c ../firmware/App/measure.c ../firmware/App/sunalign.c $(HEADERS)
	mkdir -p build
	$(CC) $(CFLAGS) $(filter %.c,$^) -lm -o $@

build/serial: test_serial.c ../firmware/Platform/serial_text.c ../firmware/Platform/serial_text.h
	mkdir -p build
	$(CC) $(CFLAGS) -I../firmware/Platform $(filter %.c,$^) -o $@

build/clock_pair: test_clock_pair.c ../firmware/Platform/clock_pair.c ../firmware/Platform/clock_pair.h
	mkdir -p build
	$(CC) $(CFLAGS) -I../firmware/Platform $(filter %.c,$^) -o $@

build/timebase: test_timebase.c ../firmware/Platform/timebase.c ../firmware/Platform/timebase.h timebase_stub/main.h
	mkdir -p build
	$(CC) $(CFLAGS) -Itimebase_stub -I../firmware/Platform $(filter %.c,$^) -o $@
