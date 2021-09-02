onerror {resume}

# set cluster0 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_0/i_cluster
# set cluster1 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_1/i_cluster
# set cluster2 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_2/i_cluster
# set cluster3 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_3/i_cluster
set cluster0 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_0/i_cluster
set cluster1 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_1/i_occamy_cluster_0/i_cluster
set cluster2 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_2/i_occamy_cluster_0/i_cluster
set cluster3 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_3/i_occamy_cluster_0/i_cluster
set cluster4 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_4/i_occamy_cluster_0/i_cluster
set cluster5 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_5/i_occamy_cluster_0/i_cluster
set cluster6 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_6/i_occamy_cluster_0/i_cluster
set cluster7 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_7/i_occamy_cluster_0/i_cluster
set snitch_cc_0 $cluster0/gen_core[0]/i_snitch_cc
set snitch_cc_1 $cluster1/gen_core[0]/i_snitch_cc
set snitch_cc_2 $cluster2/gen_core[0]/i_snitch_cc
set snitch_cc_3 $cluster3/gen_core[0]/i_snitch_cc
set snitch_cc_4 $cluster4/gen_core[0]/i_snitch_cc
set snitch_cc_5 $cluster5/gen_core[0]/i_snitch_cc
set snitch_cc_6 $cluster6/gen_core[0]/i_snitch_cc
set snitch_cc_7 $cluster7/gen_core[0]/i_snitch_cc
set fpu0 $snitch_cc_0/gen_fpu/i_snitch_fp_ss
set fpu1 $snitch_cc_1/gen_fpu/i_snitch_fp_ss
set fpu2 $snitch_cc_2/gen_fpu/i_snitch_fp_ss
set fpu3 $snitch_cc_3/gen_fpu/i_snitch_fp_ss
set fpu4 $snitch_cc_4/gen_fpu/i_snitch_fp_ss
set fpu5 $snitch_cc_5/gen_fpu/i_snitch_fp_ss
set fpu6 $snitch_cc_6/gen_fpu/i_snitch_fp_ss
set fpu7 $snitch_cc_7/gen_fpu/i_snitch_fp_ss


add wave -noupdate -divider FPUs

add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/op_i
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/in_ready_o
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/out_valid_o

add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/op_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/out_valid_o

add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/op_i
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/out_valid_o

add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/op_i
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU3 $fpu3/i_fpu/out_valid_o

add wave -noupdate -expand -group FPU4 $fpu4/i_fpu/op_i
add wave -noupdate -expand -group FPU4 $fpu4/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU4 $fpu4/i_fpu/out_valid_o

add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/op_i
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU2 $fpu2/i_fpu/out_valid_o

add wave -noupdate -expand -group FPU6 $fpu6/i_fpu/op_i
add wave -noupdate -expand -group FPU6 $fpu6/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU6 $fpu6/i_fpu/out_valid_o

add wave -noupdate -expand -group FPU7 $fpu7/i_fpu/op_i
add wave -noupdate -expand -group FPU7 $fpu7/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU7 $fpu7/i_fpu/out_valid_o


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
