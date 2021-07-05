// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "data.h"
#include "conv2d.h"

/* static volatile uint32_t cluster2cluster_synch = 0; */

int main() {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();

    // Tiling factor
    const uint32_t TILE = 4; // must be equal cluster_num for cluster2cluster communication
    const uint32_t DB = 2;
    const uint32_t cluster2cluster = 0;

    struct cluster_mem_alloc {
        double ifmap[DB][l.ih][l.iw][l.ci/TILE];
        double weights[DB][l.co/cluster_num][l.ci/TILE][l.fh][l.fw];
        double ofmap[l.oh][l.ow][l.co/cluster_num];
        double ofmap_GM[l.oh][l.ow][l.co/cluster_num];
    };

    struct cluster_mem_alloc *mem = (void*)snrt_cluster_memory().start;

    if (snrt_is_dm_core()) {
        memset(&mem->ofmap, 0, sizeof(double) * OFMAP_SIZE / cluster_num);

        if (!cluster2cluster) {

            // ifmap has channel-major mem-layout, hence we need a 2d transfer
            // to load only a subset of channels
            snrt_dma_txid_t ifmap_txid = \
                snrt_dma_start_2d(&mem->ifmap[0], /* dst */
                                  ifmap_dram, /* src */
                                  sizeof(double) * l.ci / TILE, /* size */
                                  sizeof(double) * l.ci / TILE, /* dst_stride */
                                  sizeof(double) * l.ci,         /* src_stride */
                                  l.ih * l.iw /* repetitions */
                                  );

            snrt_dma_txid_t weights_txid = \
                snrt_dma_start_2d(&mem->weights[0], /* dst */
                                  &weights_dram[l.co/cluster_num*cluster_id], /* src */
                                  sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* size */
                                  sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* dst_stride */
                                  sizeof(double) * WEIGHTS_SIZE / l.co,         /* src_stride */
                                  l.co / cluster_num /* repetitions */
                                  );
        }
        else {

            // ifmap has channel-major mem-layout, hence we need a 2d transfer
            // to load only a subset of channels

            snrt_dma_txid_t ifmap_txid = \
                snrt_dma_start_2d(&mem->ifmap[0], /* dst */
                                  &ifmap_dram[0][0][l.ci/TILE*cluster_id], /* src */
                                  sizeof(double) * l.ci / TILE, /* size */
                                  sizeof(double) * l.ci / TILE, /* dst_stride */
                                  sizeof(double) * l.ci,         /* src_stride */
                                  l.ih * l.iw /* repetitions */
                                  );

            snrt_dma_txid_t weights_txid = \
                snrt_dma_start_2d(&mem->weights[0], /* dst */
                                  &weights_dram[l.co/cluster_num*cluster_id][l.ci/TILE*cluster_id], /* src */
                                  sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* size */
                                  sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* dst_stride */
                                  sizeof(double) * WEIGHTS_SIZE / l.co,         /* src_stride */
                                  l.co / cluster_num /* repetitions */
                                  );
        }
        snrt_dma_wait_all();
    }


    // First tile is already loaded, asynchronously load next tile
    // while performing kernel over loaded tile
    for (uint32_t t = 1; t < TILE; t++) {

        snrt_barrier();

        // Which buffer to read/write from/to
        uint32_t read_buf, write_buf;
        read_buf = (t + 1) % DB;
        write_buf = t % DB;

        // Load next tile in memory while performing kernel over current tile
        // (Double Buffering)
        if (snrt_is_dm_core()) {

            if (!cluster2cluster) {

                // DMA Core asynchronously loads next tile
                snrt_dma_txid_t ifmap_txid = \
                    snrt_dma_start_2d(&mem->ifmap[write_buf], /* dst */
                                      &ifmap_dram[0][0][t*l.ci/TILE], /* src */
                                      sizeof(double) * l.ci / TILE, /* size */
                                      sizeof(double) * l.ci / TILE, /* dst_stride */
                                      sizeof(double) * l.ci,       /* src_stride */
                                      l.ih * l.iw /* repetitions */
                                      );

                snrt_dma_txid_t weights_txid = \
                    snrt_dma_start_2d(&mem->weights[write_buf], /* dst */
                                      &weights_dram[l.co/cluster_num*cluster_id][t*l.ci/TILE], /* src */
                                      sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* size */
                                      sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* dst_stride */
                                      sizeof(double) * WEIGHTS_SIZE / l.co,         /* src_stride */
                                      l.co / cluster_num /* repetitions */
                                      );
            }
            else {

                /* while(cluster2cluster_synch % 2 != cluster_id % 2); */

                uint32_t src_cluster_id = (cluster_id + 1) % cluster_num;

                struct cluster_mem_alloc *src_mem = (void*)mem - (cluster_id - src_cluster_id) * 0x00040000;

                // DMA Core asynchronously loads next tile

                snrt_dma_txid_t ifmap_txid = \
                    snrt_dma_start_1d(&mem->ifmap[write_buf], /* dst */
                                      &src_mem->ifmap[read_buf], /* src */
                                      sizeof(double) * IFMAP_SIZE / TILE);

                snrt_dma_txid_t weights_txid = \
                    snrt_dma_start_2d(&mem->weights[write_buf], /* dst */
                                      &weights_dram[l.co/cluster_num*cluster_id][((cluster_id + t) % cluster_num)*l.ci/TILE], /* src */
                                      sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* size */
                                      sizeof(double) * WEIGHTS_SIZE / l.co / TILE, /* dst_stride */
                                      sizeof(double) * WEIGHTS_SIZE / l.co,         /* src_stride */
                                      l.co / cluster_num /* repetitions */
                                      );
            }


            snrt_dma_wait_all();

            /* if (cluster_id % 2 == cluster_id) { */
            /*     cluster2cluster_synch++; */
            /* } */
        }
        else {
            // Compute cores compute kernel over current tile
            conv2d_hwc_ssr_frep((double*)&mem->ifmap[read_buf], (double*)&mem->ofmap, (double*)&mem->weights[read_buf],
                                l.co / cluster_num, l.ci / TILE, l.oh, l.ow, l.ih, l.iw, l.fh, l.fw);
        }

    }

    snrt_cluster_barrier();

    // perform last kernel
    if (snrt_is_compute_core()) {
        uint32_t read_buf = (TILE - 1) % 2;
        conv2d_hwc_ssr_frep((double*)&mem->ifmap[read_buf], (double*)&mem->ofmap, (double*)&mem->weights[read_buf],
                            l.co / cluster_num, l.ci / TILE, l.oh, l.ow, l.ih, l.iw, l.fh, l.fw);
    }

    snrt_cluster_barrier();


    uint32_t errors = 0;

    if (snrt_is_dm_core()) {
        snrt_dma_start_2d(&mem->ofmap_GM, /* dst */
                          &ofmap_dram[0][0][l.co/cluster_num*cluster_id], /* src */
                          sizeof(double) * l.co / cluster_num, /* size */
                          sizeof(double) * l.co / cluster_num, /* dst_stride */
                          sizeof(double) * l.co,         /* src_stride */
                          l.oh * l.ow /* repetitions */
                          );
        snrt_dma_wait_all();


        for (uint32_t oh = 0; oh < l.oh; oh++) {
            for (uint32_t ow = 0; ow < l.ow; ow++) {
                for (uint32_t co = 0; co < l.co/cluster_num; co++) {

                    if(fabs(mem->ofmap_GM[oh][ow][co] - mem->ofmap[oh][ow][co]) > 0.001) {
                        errors++;
                    }
                }
            }
        }
        if (errors == 0) {
            printf("Cluster %d/%d: No Errors\n", cluster_id, cluster_num);
        } else {
            printf("Cluster %d/%d: %d Errors\n", cluster_id, cluster_num, errors);
        }

    }

    snrt_barrier();

    return errors;
}
