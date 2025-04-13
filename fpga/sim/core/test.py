#!/usr/bin/env python3
"""Core test creator."""

from vunit import VUnit
from os.path import dirname, join

root = dirname(__file__)

# Create VUnit instance by parsing command line arguments
vu = VUnit.from_argv()

# Add VUnit's builtin HDL utilities for checking, logging, communication...
vu.add_vhdl_builtins()

# Create library 'lib'
lib = vu.add_library("lib")

# Add testbench files
lib.add_source_files([join(root, '*.vhd'),
                     join(root, '../../src/*.vhd')])

# Run vunit function
vu.main()