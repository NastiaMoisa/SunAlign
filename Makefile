IMAGE ?= sunalign-build:local

.PHONY: image build clean

image:
	docker build -t $(IMAGE) docker

build: image
	python3 tools/check_pinout.py
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean

flash monitor:
	@echo 'VS Code: F1 -> Wokwi: Start Simulator (UART Monitor opens with the simulation)'

test coverage: image
	python3 tools/check_architecture.py
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) make -C tests $@
