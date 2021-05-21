// Copyright 2020 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Paul Scheffler <paulsc@iis.ee.ethz.ch>

module tb_simple_ssr_streamer;

  import snitch_ssr_pkg::*;

  // Test parameters
  localparam int unsigned AddrWidth = 32;
  localparam int unsigned DataWidth = 64;
  localparam int unsigned NumSsrs   = 3;
  localparam int unsigned RPorts    = 3;
  localparam int unsigned WPorts    = 1;


  // DUT parameters
  function automatic ssr_cfg_t [NumSsrs-1:0] gen_cfg_ssr();
    ssr_cfg_t CfgSsrDefault = '{
      Indirection:    1,
      IndirOutSpill:  1,
      NumLoops:       4,
      IndexWidth:     16,
      PointerWidth:   18,
      ShiftWidth:     3,
      IndexCredits:   3,
      DataCredits:    4,
      MuxRespDepth:   3,
      RptWidth:       4,
      default:        '0    // Isect parameters below
    };
    ssr_cfg_t [NumSsrs-1:0] ret = '{CfgSsrDefault, CfgSsrDefault, CfgSsrDefault};
    ret[0].IsectMaster    = 1'b1;
    ret[1].IsectMaster    = 1'b1;
    ret[1].IsectMasterIdx = 1'b1;
    ret[2].IsectSlave     = 1'b1;
    return ret;
  endfunction

  localparam ssr_cfg_t [NumSsrs-1:0] SsrCfgs  = gen_cfg_ssr();
  localparam ssr_cfg_t [NumSsrs-1:0] SsrRegs  = '{2, 1, 0};

  fixture_ssr_streamer #(
    .NumSsrs    ( NumSsrs   ),
    .RPorts     ( RPorts    ),
    .WPorts     ( WPorts    ),
    .AddrWidth  ( AddrWidth ),
    .DataWidth  ( DataWidth ),
    .SsrCfgs    ( SsrCfgs   ),
    .SsrRegs    ( SsrRegs   )
  ) fix ();

  initial begin
    fix.wait_for_reset_start();
    fix.wait_for_reset_end();

    // TODO

    // Done, no error errors occured
    $display("SUCCESS");
    $finish;
  end

endmodule
