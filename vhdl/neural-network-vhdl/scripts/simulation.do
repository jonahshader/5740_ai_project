# simulation.do

-- This script is intended to run simulations for the neural network VHDL design.

-- Set the library path
vlib work
vmap work work

-- Compile the source files
vcom -2008 ../src/components.vhdl
vcom -2008 ../src/Mux2to1.vhdl
vcom -2008 ../src/register.vhdl
vcom -2008 ../src/activation.vhdl
vcom -2008 ../src/neuron.vhdl
vcom -2008 ../src/NN.vhdl

-- Compile the testbench
vcom -2008 ../src/testbench/NN_tb.vhdl

-- Run the simulation
vsim -c work.NN_tb -do "run -all; quit;"