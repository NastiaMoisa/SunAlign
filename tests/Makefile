CC = gcc
CFLAGS = -std=c11 -O0 -g --coverage -Wall -Wextra -Werror -I../firmware/App -I../firmware/Devices
SOURCES = test_sunalign.c ../firmware/App/sunalign.c ../firmware/Devices/mpu6050.c
.PHONY: test coverage clean
test: build/tests
	rm -f build/*.gcda
	./build/tests
build/tests: $(SOURCES) fixtures/mpu6050_capture.h
	mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) -lm -o $@
coverage: test
	gcovr --root .. --filter '../firmware/(App|Devices)/' --txt --txt-metric branch --output ../docs/verification/lab2/coverage-branches.txt
	gcovr --root .. --filter '../firmware/(App|Devices)/' --txt --output ../docs/verification/lab2/coverage-lines.txt
	gcovr --root .. --filter '../firmware/(App|Devices)/' --json-summary --output ../docs/verification/lab2/coverage.json --fail-under-line 90 --fail-under-branch 75
clean:
	rm -rf build
