// Copyright 2020 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Author: Paul Scheffler <paulsc@iis.ee.ethz.ch>

module snitch_ssr_intersector #(
  parameter type isect_mst_req_t = logic,
  parameter type isect_mst_rsp_t = logic,
  parameter type isect_slv_req_t = logic,
  parameter type isect_slv_rsp_t = logic
) (
  input  isect_mst_req_t [1:0]  mst_req_i,
  output isect_mst_rsp_t [1:0]  mst_rsp_o,
  input  isect_slv_req_t        slv_req_i,
  output isect_slv_rsp_t        slv_rsp_o
);

  logic isect_match, isect_merge, isect_mslag, isect_last;
  logic src_valid, dst_ready;

  assign isect_match  = (mst_req_i[1].idx == mst_req_i[0].idx);

  // Globally merge when both parties request merge
  // TODO: is this a sensible choice?
  assign isect_merge  = mst_req_i[1].merge & mst_req_i[0].merge;

  // Always select smaller index (not relevant for index matching)
  assign isect_mslag  = (mst_req_i[1].idx < mst_req_i[0].idx);

  // Source indices must both have valid data to proceed, and match iff in matching mode.
  assign src_valid    = slv_req_i[0].valid & slv_req_i[1].valid & (isect_merge | isect_match);

  // Destination can stall iff enabled
  assign dst_ready    = ~slv_req_i.ena | slv_req_i.ready;

  // Broadcast last when lagging stream signals it in merging, or either in matching
  assign isect_last   = isect_merge ?
      mst_req_i[isect_mslag].last : (mst_req_i[1].last | mst_req_i[0].last);

  assign mst_rsp_o[0] = '{
    zero:   isect_merge & isect_mslag,
    last:   isect_last,
    ready:  src_valid & dst_ready
  };

  assign mst_rsp_o[1] = '{
    zero:   isect_merge & ~isect_mslag,
    last:   isect_last,
    ready:  src_valid & dst_ready
  };

  assign slv_rsp_o = '{
    idx:    mst_req_i[isect_mslag].idx,
    last:   isect_last,
    valid:  src_valid
  };

endmodule
