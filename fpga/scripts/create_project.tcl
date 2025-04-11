# Kria KV260 Minimal Project Creation Script
# This script will:
# 1. Create a new Vivado project for the Kria KV260
# 2. Source the exported block design TCL file
# 3. Create the HDL wrapper for the block design
# 4. Add the constraints file
# 5. Set up basic project settings

# Set project name and directory
set projName "kria_minimal"
set projDir "../${projName}"  # Create project in root directory, one level up from scripts

# Create project
create_project ${projName} ${projDir} -part xck26-sfvc784-2LV-c

# Set the board part for Kria KV260 Vision AI Starter Kit
set_property board_part xilinx.com:kv260_vision_starter:part0:1.4 [current_project]

# Set project properties
set_property target_language VHDL [current_project]

# Source the block design TCL
source [file join [file dirname [info script]] "kv260_design.tcl"]

# Create the BD wrapper
make_wrapper -files [get_files [get_property FILE_NAME [get_bd_designs]]] -top
add_files -norecurse [file join [get_property DIRECTORY [get_property PARENT [get_files [get_property FILE_NAME [get_bd_designs]]]]] hdl [set wrapper_file "[file tail [file rootname [get_property FILE_NAME [get_bd_designs]]]].vhd"]]

# Add constraints file
# Constraints file in root directory
add_files -fileset constrs_1 -norecurse [file join [file dirname [file dirname [info script]]] "pmod_constraints.xdc"]

# Set the top module to the wrapper
set_property top_file [get_files [set wrapper_file]] [current_fileset]
set_property top [file rootname $wrapper_file] [current_fileset]
update_compile_order -fileset sources_1

# Add custom HDL files
add_files [file join [file dirname [file dirname [info script]]] "src"]

puts "Project ${projName} has been successfully created."
puts "Block design imported and constraints added."
puts "You can now run synthesis and implementation."

# Uncomment these lines if you want the script to automatically launch synthesis
# launch_runs synth_1 -jobs 8
# wait_on_run synth_1
