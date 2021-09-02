onerror {resume}

set cluster0 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_0/i_cluster
set cluster1 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_1/i_cluster
set cluster2 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_2/i_cluster
set cluster3 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_3/i_cluster
set snitch_cc_0 $cluster0/gen_core[0]/i_snitch_cc
set snitch_cc_1 $cluster1/gen_core[0]/i_snitch_cc
set snitch_cc_2 $cluster2/gen_core[0]/i_snitch_cc
set snitch_cc_3 $cluster3/gen_core[0]/i_snitch_cc
set fpu0 $snitch_cc_0/gen_fpu/i_snitch_fp_ss
set fpu1 $snitch_cc_1/gen_fpu/i_snitch_fp_ss
set fpu2 $snitch_cc_2/gen_fpu/i_snitch_fp_ss
set fpu3 $snitch_cc_3/gen_fpu/i_snitch_fp_ss

add wave -noupdate -divider FPUs

add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/clk_i
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/rst_ni
add wave -noupdate -expand -group FPU0 $fpu0/trace_port_o
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/op_i
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/in_ready_o
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/out_valid_o
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/out_ready_i

add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/clk_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/rst_ni
add wave -noupdate -expand -group FPU1 $fpu1/trace_port_o
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/op_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/in_ready_o
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/out_valid_o
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/out_ready_i

add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/clk_i
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/rst_ni
add wave -noupdate -expand -group FPU2 $fpu2/trace_port_o
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/op_i
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/in_ready_o
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/out_valid_o
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/out_ready_i

add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/clk_i
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/rst_ni
add wave -noupdate -expand -group FPU3 $fpu3/trace_port_o
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/op_i
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/in_ready_o
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/out_valid_o
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/out_ready_i


configure wave -namecolwidth 195
configure wave -valuecolwidth 232
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ps
update
