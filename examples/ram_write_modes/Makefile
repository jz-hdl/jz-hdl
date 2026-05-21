.PHONY: help check tang_nano_9k tang_nano_20k pa35t-edu ice_sugar_v15 build simulate synthesis clean

.DEFAULT_GOAL := help

JZ_HDL := ../../build/compiler/jz-hdl
YOSYS := yosys
PNR_GOWIN ?= nextpnr-himbaechel
PNR_XILINX ?= nextpnr-himbaechel
PNR_ICE40 ?= nextpnr-ice40

BOARD_GOAL := $(firstword $(MAKECMDGOALS))

ifeq ($(BOARD_GOAL),tang_nano_9k)
ARCH := gowin
BOARD := tangnano9k
FAMILY := GW1N-9C
DEVICE := GW1NR-LV9QN88PC6/I5
HDL := src/tang_nano_9k.jz
endif

ifeq ($(BOARD_GOAL),tang_nano_20k)
ARCH := gowin
BOARD := tangnano20k
FAMILY := GW2A-18C
DEVICE := GW2AR-LV18QN88C8/I7
HDL := src/tang_nano_20k.jz
endif

ifeq ($(BOARD_GOAL),pa35t-edu)
ARCH := xilinx
DEVICE := xc7a35tfgg484-2
XRAY_DB_DIR ?= $(HOME)/Development/fpga-dev/prjxray/database/artix7
HDL := src/pa35t-edu.jz
endif

ifeq ($(BOARD_GOAL),ice_sugar_v15)
ARCH := ice40
DEVICE := up5k
PACKAGE := sg48
HDL := src/ice_sugar_v15.jz
endif

ifeq ($(ARCH),gowin)
PNR := $(PNR_GOWIN)
GOWIN_PATH := $(shell dirname $(shell command -v gowin_pack 2>/dev/null) 2>/dev/null)
endif
ifeq ($(ARCH),ice40)
PNR := $(PNR_ICE40)
endif
ifeq ($(ARCH),xilinx)
PNR := $(PNR_XILINX)
endif

tang_nano_9k:
	@true

tang_nano_20k:
	@true

pa35t-edu:
	@true

ice_sugar_v15:
	@true

help:
	@echo "Boards: tang_nano_9k tang_nano_20k pa35t-edu ice_sugar_v15"
	@echo "Commands: make <board> build | synthesis | simulate"

check:
	@fail=0; \
	test -f $(JZ_HDL) || { echo "jz-hdl missing"; fail=1; }; \
	command -v $(YOSYS) >/dev/null 2>&1 || { echo "yosys missing"; fail=1; }; \
	command -v $(PNR) >/dev/null 2>&1 || { echo "$(PNR) missing"; fail=1; }; \
	if [ "$(ARCH)" = "gowin" ]; then command -v gowin_pack >/dev/null 2>&1 || { echo "gowin_pack missing"; fail=1; }; fi; \
	if [ "$(ARCH)" = "ice40" ]; then command -v icepack >/dev/null 2>&1 || { echo "icepack missing"; fail=1; }; fi; \
	if [ "$(ARCH)" = "xilinx" ]; then command -v fasm2frames >/dev/null 2>&1 || { echo "fasm2frames missing"; fail=1; }; command -v xc7frames2bit >/dev/null 2>&1 || { echo "xc7frames2bit missing"; fail=1; }; fi; \
	if [ $$fail -ne 0 ]; then exit 1; fi

simulate:
	@mkdir -p out
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --simulate src/simulation.jz --vcd -o out/simulation.vcd

build: check
	@test -n "$(HDL)" || (echo "board not selected"; exit 1)
	@mkdir -p out reports
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --explain --lint $(HDL)
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --alias-report $(HDL) -o reports/alias.txt
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --tristate-report $(HDL) -o reports/tristate.txt
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --memory-report $(HDL) -o reports/memory.txt
ifeq ($(ARCH),gowin)
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --verilog $(HDL) -o out/project.v --cst out/project.cst --sdc out/project.sdc
endif
ifeq ($(ARCH),ice40)
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --verilog $(HDL) -o out/project.v --pcf out/project.pcf --sdc out/project.sdc
endif
ifeq ($(ARCH),xilinx)
	$(JZ_HDL) --sandbox-root=./ --allow-traversal --verilog $(HDL) -o out/project.v --xdc out/project.xdc --sdc out/project.sdc
endif

synthesis: build
ifeq ($(ARCH),gowin)
	$(YOSYS) -q -p "read_verilog ./out/project.v; synth_gowin -top top -json ./out/synthesis.json"
	$(PNR) --json out/synthesis.json --write out/bitstream.json --device $(DEVICE) --vopt family=$(FAMILY) --vopt cst=./out/project.cst --sdc ./out/project.sdc
	PATH="$(GOWIN_PATH):$(PATH)" gowin_pack -d $(FAMILY) -o out/project.fs out/bitstream.json
endif
ifeq ($(ARCH),ice40)
	$(YOSYS) -q -p "read_verilog ./out/project.v; synth_ice40 -top top -json ./out/synthesis.json"
	$(PNR) --$(DEVICE) --package $(PACKAGE) --json out/synthesis.json --pcf out/project.pcf --asc out/project.asc
	icepack out/project.asc out/project.bin
endif
ifeq ($(ARCH),xilinx)
	$(YOSYS) -q -p "read_verilog ./out/project.v; synth_xilinx -top top -json ./out/synthesis.json"
	$(PNR) --device $(DEVICE) --json out/synthesis.json --vopt xdc=out/project.xdc --vopt fasm=out/project.fasm
	fasm2frames --db-root $(XRAY_DB_DIR) --part $(DEVICE) out/project.fasm > out/project.frames
	xc7frames2bit --part-file $(XRAY_DB_DIR)/$(DEVICE)/part.yaml --part-name $(DEVICE) --frm-file out/project.frames --output-file out/project.bit
endif

clean:
	rm -rf out reports

%:
	@true
