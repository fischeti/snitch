onerror {resume}

set cluster0 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_0/i_cluster
set cluster1 /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0/i_occamy_cluster_1/i_cluster
set snitch_cc_0 $cluster0/gen_core[0]/i_snitch_cc
set snitch_cc_1 $cluster1/gen_core[0]/i_snitch_cc
set cpu0 $snitch_cc_0/i_snitch
set fpu0 $snitch_cc_0/gen_fpu/i_snitch_fp_ss
set fpu1 $snitch_cc_1/gen_fpu/i_snitch_fp_ss
set ssr0 $snitch_cc_0/gen_ssrs/i_snitch_ssr_streamer/gen_ssrs[0]/i_ssr
set ssr1 $snitch_cc_0/gen_ssrs/i_snitch_ssr_streamer/gen_ssrs[1]/i_ssr

add wave -noupdate -expand -group {All Cores} $cpu0/wfi_q
add wave -noupdate -expand -group {All Cores} $cpu0/pc_q
add wave -noupdate -expand -group {All Cores} $cpu0/instret_q

add wave -noupdate -divider Regfile
add wave -noupdate -expand -group {REGFILE} $cpu0/gpr_raddr
add wave -noupdate -expand -group {REGFILE} $cpu0/gpr_rdata
add wave -noupdate -expand -group {REGFILE} $cpu0/gpr_waddr
add wave -noupdate -expand -group {REGFILE} $cpu0/gpr_wdata
add wave -noupdate -expand -group {REGFILE} $cpu0/gpr_we


add wave -noupdate -divider FPUs
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/clk_i
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/rst_ni
add wave -noupdate -expand -group FPU0 $fpu0/trace_port_o
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/op_i
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/in_ready_o
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/out_valid_o
add wave -noupdate -expand -group FPU0 $fpu0/i_fpu/out_ready_i
add wave -noupdate -expand -group {FPU0 LSU} $fpu0/i_snitch_lsu/lsu_qready_o
add wave -noupdate -expand -group {FPU0 LSU} $fpu0/i_snitch_lsu/lsu_qvalid_i
add wave -noupdate -expand -group {FPU0 LSU} $fpu0/i_snitch_lsu/lsu_pready_i
add wave -noupdate -expand -group {FPU0 LSU} $fpu0/i_snitch_lsu/lsu_pvalid_o
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/clk_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/rst_ni
add wave -noupdate -expand -group FPU1 $fpu1/trace_port_o
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/op_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/in_valid_i
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/in_ready_o
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/out_valid_o
add wave -noupdate -expand -group FPU1 $fpu1/i_fpu/out_ready_i
add wave -noupdate -expand -group {FPU1 LSU} $fpu1/i_snitch_lsu/lsu_qready_o
add wave -noupdate -expand -group {FPU1 LSU} $fpu1/i_snitch_lsu/lsu_qvalid_i
add wave -noupdate -expand -group {FPU1 LSU} $fpu1/i_snitch_lsu/lsu_pready_i
add wave -noupdate -expand -group {FPU1 LSU} $fpu1/i_snitch_lsu/lsu_pvalid_o

add wave -noupdate -divider SSR
# add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/cfg_word_i
# add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/cfg_rdata_o
# add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/cfg_wdata_i
# add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/cfg_write_i

add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/mem_addr_o
add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/mem_write_o
add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/mem_valid_o
add wave -noupdate -expand -group {SSR0} $ssr0/i_addr_gen/mem_ready_i
add wave -noupdate -expand -group {SSR0} $ssr0/data_rsp
add wave -noupdate -expand -group {SSR0} $ssr0/data_req

add wave -noupdate -expand -group {SSR1} $ssr1/i_addr_gen/mem_addr_o
add wave -noupdate -expand -group {SSR1} $ssr1/i_addr_gen/mem_write_o
add wave -noupdate -expand -group {SSR1} $ssr1/i_addr_gen/mem_valid_o
add wave -noupdate -expand -group {SSR1} $ssr1/i_addr_gen/mem_ready_i
add wave -noupdate -expand -group {SSR1} $ssr1/data_rsp
add wave -noupdate -expand -group {SSR1} $ssr1/data_req

add wave -noupdate -divider {Perf Counters}
set perf_counter $cluster0/i_snitch_cluster_peripheral
add wave -noupdate -expand -group {PERF COUNTER} $perf_counter/reg2hw.perf_counter_enable
add wave -noupdate -expand -group {PERF COUNTER} $perf_counter/reg2hw.perf_counter
add wave -noupdate -expand -group {PERF COUNTER} $perf_counter/core_events_i
add wave -noupdate -expand -group {PERF COUNTER} $perf_counter/tcdm_events_i

add wave -noupdate -divider {DMA}
add wave -noupdate -expand -group {DMA0} $cluster0/gen_core[8]/i_snitch_cc/gen_dma/i_axi_dma_tc_snitch_fe/dma_perf_o
add wave -noupdate -expand -group {DMA0} $cluster0/wide_in_req_i
add wave -noupdate -expand -group {DMA0} $cluster0/wide_in_resp_o
add wave -noupdate -expand -group {DMA0} $cluster0/wide_out_req_o
add wave -noupdate -expand -group {DMA0} $cluster0/wide_out_resp_i
add wave -noupdate -expand -group {DMA0} $cluster0/gen_core[8]/i_snitch_cc/gen_dma/i_axi_dma_tc_snitch_fe/axi_dma_req_o
add wave -noupdate -expand -group {DMA0} $cluster0/gen_core[8]/i_snitch_cc/gen_dma/i_axi_dma_tc_snitch_fe/axi_dma_res_i
add wave -noupdate -expand -group {DMA0} $cluster0/i_axi_to_mem_dma/axi_req_i
add wave -noupdate -expand -group {DMA0} $cluster0/i_axi_to_mem_dma/axi_resp_o


add wave -noupdate -expand -group {DMA1} $cluster1/gen_core[8]/i_snitch_cc/gen_dma/i_axi_dma_tc_snitch_fe/dma_perf_o
add wave -noupdate -expand -group {DMA1} $cluster1/wide_in_req_i
add wave -noupdate -expand -group {DMA1} $cluster1/wide_in_resp_o
add wave -noupdate -expand -group {DMA1} $cluster1/wide_out_req_o
add wave -noupdate -expand -group {DMA1} $cluster1/wide_out_resp_i
add wave -noupdate -expand -group {DMA1} $cluster1/gen_core[8]/i_snitch_cc/gen_dma/i_axi_dma_tc_snitch_fe/axi_dma_req_o
add wave -noupdate -expand -group {DMA1} $cluster1/gen_core[8]/i_snitch_cc/gen_dma/i_axi_dma_tc_snitch_fe/axi_dma_res_i
add wave -noupdate -expand -group {DMA1} $cluster1/i_axi_to_mem_dma/axi_req_i
add wave -noupdate -expand -group {DMA1} $cluster1/i_axi_to_mem_dma/axi_resp_o


# WaveRestoreCursors {{Cursor 1} {10685 ns} 1} {{Cursor 2} {10273 ns} 1} {{Cursor 3} {1560471 ps} 0}
# quietly wave cursor active 3
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
# WaveRestoreZoom {1544281 ps} {1585087 ps}
