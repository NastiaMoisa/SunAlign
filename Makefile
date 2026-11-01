IMAGE ?= sunalign-build:local
BUILD_DIR ?= build-current

.PHONY: image build clean

image:
	docker build -t $(IMAGE) docker

build: image
	python3 tools/check_pinout.py
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware BUILD_DIR=$(BUILD_DIR) all

clean:
	$(MAKE) -C firmware BUILD_DIR=$(BUILD_DIR) clean

flash monitor:
	@echo 'VS Code: F1 -> Wokwi: Start Simulator (UART Monitor opens with the simulation)'

test coverage: image
	python3 tools/check_architecture.py
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) make -C tests $@
