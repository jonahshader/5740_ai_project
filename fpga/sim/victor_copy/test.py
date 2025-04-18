#!/usr/bin/env python3
"""victor_copy test creator."""

from os.path import dirname, join, basename
import glob

from vunit import VUnit


root = dirname(__file__)

# Create VUnit instance by parsing command line arguments
vu = VUnit.from_argv()

# Add VUnit's builtin HDL utilities for checking, logging, communication...
vu.add_vhdl_builtins()

# Create library 'lib'
lib = vu.add_library("lib")

# Add testbench files
lib.add_source_files(join(root, '*.vhd'))

# Add imports
lib.add_source_files(join(root, '../../src/imports/*.vhd'))

# Add neural_network files
lib.add_source_files(join(root, '../../src/neural_network/*.vhd'))

# Add src files except top.vhd
src_files = [f for f in glob.glob(
    join(root, '../../src/*.vhd')) if basename(f) != "top.vhd"]
lib.add_source_files(src_files)

# Run vunit function
vu.main()
