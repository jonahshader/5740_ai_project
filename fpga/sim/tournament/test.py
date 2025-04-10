#!/usr/bin/env python3
"""Tournament test creator."""

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
# TODO: use glob for ../..
lib.add_source_files([join(root, '*.vhd'),
                     join(root, '../../tournament.vhd'),
                     join(root, '../../ga_types.vhd'),
                     join(root, '../../game_types.vhd'),
                     join(root, '../../custom_utils.vhd')])

# Run vunit function
vu.main()