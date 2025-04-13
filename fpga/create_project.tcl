# Kria KV260 Minimal Project Creation Script
# This script will:
# 1. Create a new Vivado project for the Kria KV260
# 2. Source the exported block design TCL file
# 3. Create the HDL wrapper for the block design
# 4. Add the constraints file
# 5. Set up basic project settings

# Set default values
set force_build false
set gen_bitstream false
set extra_reports false
set proj_name "kv260_jnb_neuroev"

set usage [join [list {*}{
  "Usage: 10x.tcl [-b] [-f] [-e]"
  "  -f --force:   recreate the project even if it already exists"
  "  -b --bit:     generate the bitstream and XSA after synthesis and implementation"
  "  -e --extra:   output extra timing and power reports (saved to timing.rpt and power.rpt)."
  "  -h --help:    print this message"
}] \n]

# Parse command-line arguments
for {set i 0} {$i < $argc} {incr i} {
  set arg [lindex $argv $i]
  switch -glob -- $arg {
    -e* - --extra {
      set extra_reports true
    }
    -f* - --force {
      set force_build true
    }
    -b* - --bit {
      set gen_bitstream true
    }
    -h* - --help {
      puts $usage
      exit 0
    }
    default {
      puts "\033\[0;31mError: Unknown option '$arg'.\033\[0m"
      puts $usage
      exit 1
    }
  }
}

# Get the path to this script [the git repo]/vivado
set base_dir [file dirname [file normalize [info script]]]
set src_dir [file normalize "${base_dir}/src"]
# set sim_dir [file normalize "${base_dir}/xsim"]
set build_dir ${base_dir}/build/${proj_name}

if {[file exists "$build_dir"] && !$force_build } {
  puts "\033\[0;34mProject directory exists, opening existing project $proj_name.\033\[0m"
  open_project "$build_dir/$proj_name.xpr"
} else {

  # Create project
  create_project ${proj_name} ${build_dir} -part xck26-sfvc784-2LV-c

  # Set the board part for Kria KV260 Vision AI Starter Kit
  set_property board_part "xilinx.com:kv260_som:part0:1.4" [current_project]
  set_property board_connections "som240_1_connector xilinx.com:kv260_carrier:som240_1_connector:1.3" [current_project]

  # Set project properties
  set_property target_language VHDL [current_project]
  set_property simulator_language VHDL [current_project]

  # List of source files to be added to the project
  set src_files [list]
  foreach f [glob "${src_dir}/*.vhd"] {
    lappend src_files [file normalize $f]
  }
  foreach f [glob "${src_dir}/imports/*.vhd"] {
    lappend src_files [file normalize $f]
  }
  foreach f [glob "${src_dir}/neural_network/*.vhd"] {
    lappend src_files [file normalize $f]
  }

  # List of constraints files to be added
  set constr_files [list \
    [file normalize "${base_dir}/pmod_constraints.xdc"] \
  ]

  # Create the block design and wrapper
  source ${base_dir}/kv260_design.tcl
  make_wrapper -top -fileset sources_1 -files [get_files kv260_design.bd] -import

  # Add the source files to the project and set as VHDL 2008. Also force top.vhd as the top-level.
  add_files -fileset sources_1 ${src_files}
  set_property file_type {VHDL 2008} [get_files -of_objects [get_filesets sources_1] -filter {is_generated == false} *.vhd]
  set_property top top [get_filesets sources_1]

  # Add the constraints files to the project
  puts "adding ${constr_files}"
  add_files -fileset constrs_1 ${constr_files}

  # Use ExtraTimingOpt implementation strategy for now.
  set_property strategy Performance_ExtraTimingOpt [get_runs impl_1]

  # Change some critical warnings to errors.
  set_msg_config -id {Route 35-39}   -new_severity ERROR; # IMPL:  The design did not meet timing requirements.
  set_msg_config -id {Timing 38-282} -new_severity ERROR; # IMPL:  The design failed to meet the timing requirements.
  set_msg_config -id {Synth 8-6859} -new_severity ERROR;  # SYNTH: Multi-driven pin.
  set_msg_config -id {Synth 8-3352} -new_severity ERROR;  # SYNTH: Multi-driven net.
  set_property SEVERITY ERROR [get_drc_checks MDRV-1];    # DRC:   Multi-driven net.
  set_msg_config -id {Synth 8-327}  -new_severity ERROR;  # SYNTH: Inferred latch.

  # Change some warnings to info.
  set_msg_config -id {Synth 8-4747} -new_severity INFO;   # SYNTH: Shared variables must be of a protected type.
}

if { $gen_bitstream } {
  # Mimic GUI behavior of automatically setting top and file compile order
  update_compile_order -fileset sources_1

  # Print messages (colorized)
  proc show_messages {path} {
    set file [open "$path" r]
    while {[gets $file line] != -1} {
      if {[string match "WARNING:*" $line]} {
        puts "\033\[0;93m$line\033\[0m"
      } elseif {[string match "CRITICAL WARNING:*" $line]} {
        puts "\033\[0;33m$line\033\[0m"
      } elseif {[string match "ERROR:*" $line]} {
        puts "\033\[0;31m$line\033\[0m"
      }
    }
    close $file
  }

  # Launch Synthesis
  reset_run   synth_1
  launch_runs synth_1
  wait_on_run synth_1

  set status [get_property STATUS [get_runs synth_1]]
  if { [string match "*ERROR" $status] } {
    puts "\033\[0;31m$status\033\[0m"
    puts "\033\[0;34mSynthesis messages:\033\[0m"
    show_messages ${build_dir}/${proj_name}.runs/synth_1/runme.log
    exit
  }

  # Launch Implementation
  reset_run   impl_1
  launch_runs impl_1 -to_step write_bitstream
  wait_on_run impl_1

  puts "\033\[0;34mSynthesis messages:\033\[0m"
  show_messages ${build_dir}/${proj_name}.runs/synth_1/runme.log
  puts "\033\[0;34mImplementation messages:\033\[0m"
  show_messages ${build_dir}/${proj_name}.runs/impl_1/runme.log

  # Export the hardware project as an XSA file
  write_hw_platform -fixed -include_bit -force -file "${base_dir}/${proj_name}.xsa"
}

# Generate implementation timing and power reports and write to disk
if { $extra_reports } {
  set status [get_property STATUS [get_runs impl_1]]
  if { [string match "*Complete!" $status] } {
    open_run impl_1
    report_timing_summary -delay_type min_max -report_unconstrained -check_timing_verbose \
                          -input_pins -max_paths 10 -file ${build_dir}/timing.rpt
    report_power -file ${build_dir}/power.rpt
  } else {
    puts "\033\[0;93mCan't generate timing or power report, implementation is not complete.\033\[0m"
  }
}
