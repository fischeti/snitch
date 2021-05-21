// Copyright 2020 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Paul Scheffler <paulsc@iis.ee.ethz.ch>

`include "tcdm_interface/typedef.svh"
`include "tcdm_interface/assign.svh"
`include "snitch_ssr/typedef.svh"

module fixture_ssr_streamer import snitch_ssr_pkg::*; #(
  parameter int unsigned NumSsrs    = 0,
  parameter int unsigned RPorts     = 0,
  parameter int unsigned WPorts     = 0,
  parameter int unsigned AddrWidth  = 0,
  parameter int unsigned DataWidth  = 0,
  parameter ssr_cfg_t [NumSsrs-1:0]   SsrCfgs = '0,
  parameter logic [NumSsrs-1:0][4:0]  SsrRegs = '0
);

  // ------------
  //  Parameters
  // ------------

  // Fixture parameters
  localparam bit  TcdmLog   = 0;
  localparam bit  MatchLog  = 1;
  localparam time Timeout   = 200ns;

  // Timing parameters
  localparam time TCK = 10ns;
  localparam time TA  = 2ns;
  localparam time TT  = 8ns;
  localparam int unsigned RstCycles = 10;

  // TCDM derived parameters
  localparam int unsigned WordBytes      = DataWidth/8;
  localparam int unsigned WordAddrBits   = $clog2(WordBytes);
  localparam int unsigned WordAddrWidth  = AddrWidth - WordAddrBits;

  // TCDM derived types
  typedef logic [AddrWidth-1:0]   addr_t;
  typedef logic [DataWidth-1:0]   data_t;
  typedef logic [DataWidth/8-1:0] strb_t;
  typedef logic                   user_t;
  `TCDM_TYPEDEF_ALL(tcdm, addr_t, data_t, strb_t, user_t);

  // SSR lane types
  `SSR_LANE_TYPEDEF_ALL(ssr, data_t)

  // Configuration written through proper registers
  typedef struct packed {
    logic [31:0] idx_shift;
    logic [31:0] idx_base;
    logic [31:0] idx_size;
    logic [3:0][31:0] stride;
    logic [3:0][31:0] bound;
    logic [31:0] rep;
  } cfg_regs_t;

  // Status register type
  typedef struct packed {
    cfg_status_upper_t upper;
    logic [31-$bits(cfg_status_upper_t):0] ptr;
  } cfg_status_t;

  // -----------------
  //  Clock and reset
  // -----------------

  logic clk;
  logic rst_n;

  // Clock and reset generator
  clk_rst_gen #(
    .ClkPeriod    ( TCK       ),
    .RstClkCycles ( RstCycles )
  ) i_clk_rst_gen (
    .clk_o  ( clk   ),
    .rst_no ( rst_n )
  );

  // Wait for reset to start
  task automatic wait_for_reset_start;
    @(negedge rst_n);
  endtask

  // Wait for reset to end
  task automatic wait_for_reset_end;
    @(posedge rst_n);
    @(posedge clk);
  endtask

  // -----
  //  DUT
  // -----

  // DUT signals
  logic [11:0]              cfg_word_i;
  logic                     cfg_write_i;
  logic [31:0]              cfg_rdata_o;
  logic [31:0]              cfg_wdata_i;
  logic  [RPorts-1:0][4:0]  ssr_raddr_i;
  ssr_rdata_t [RPorts-1:0]  ssr_rdata_o;
  logic  [RPorts-1:0]       ssr_rvalid_i;
  logic  [RPorts-1:0]       ssr_rready_o;
  logic  [RPorts-1:0]       ssr_rdone_i;
  logic  [WPorts-1:0][4:0]  ssr_waddr_i;
  data_t [WPorts-1:0]       ssr_wdata_i;
  logic  [WPorts-1:0]       ssr_wvalid_i;
  logic  [WPorts-1:0]       ssr_wready_o;
  logic  [WPorts-1:0]       ssr_wdone_i;
  tcdm_req_t [NumSsrs-1:0]  mem_req_o;
  tcdm_rsp_t [NumSsrs-1:0]  mem_rsp_i;
  logic [AddrWidth-1:0]     tcdm_start_address_i = '0;

  // Device Under Test (DUT)

  snitch_ssr_streamer #(
    .NumSsrs      ( NumSsrs     ),
    .RPorts       ( RPorts      ),
    .WPorts       ( WPorts      ),
    .AddrWidth    ( AddrWidth   ),
    .DataWidth    ( DataWidth   ),
    .SsrCfgs      ( SsrCfgs     ),
    .SsrRegs      ( SsrRegs     ),
    .tcdm_user_t  ( user_t      ),
    .tcdm_req_t   ( tcdm_req_t  ),
    .tcdm_rsp_t   ( tcdm_rsp_t  ),
    .ssr_rdata_t  ( ssr_rdata_t )
  ) i_snitch_ssr_streamer (
    .clk_i      ( clk   ),
    .rst_ni     ( rst_n ),
    .cfg_word_i,
    .cfg_write_i,
    .cfg_rdata_o,
    .cfg_wdata_i,
    .ssr_raddr_i,
    .ssr_rdata_o,
    .ssr_rvalid_i,
    .ssr_rready_o,
    .ssr_rdone_i,
    .ssr_waddr_i,
    .ssr_wdata_i,
    .ssr_wvalid_i,
    .ssr_wready_o,
    .ssr_wdone_i,
    .mem_req_o,
    .mem_rsp_i,
    .tcdm_start_address_i
  );

endmodule
