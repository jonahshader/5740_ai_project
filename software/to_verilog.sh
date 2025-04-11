#!/bin/bash
# Uses GHDL to convert VHDL 2008 to Verilog using synth --out=verilog
# uses std_logic_unsigned, so -fsynopsys is needed
# TODO: try removing -fsynopsys
mkdir ghdl_out

ghdl -i --std=08 -fsynopsys --workdir=ghdl_out ../fpga/src/*.vhd
ghdl -i --std=08 -fsynopsys --workdir=ghdl_out ../fpga/src/imports/*.vhd
ghdl -i --std=08 -fsynopsys --workdir=ghdl_out ../fpga/src/verilator_tops/*.vhd
ghdl -m --std=08 -fsynopsys --workdir=ghdl_out game_test
ghdl synth --std=08 -fsynopsys --workdir=ghdl_out --out=verilog game_test > game_test.v
