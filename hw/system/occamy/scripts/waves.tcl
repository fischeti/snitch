set quadrant /tb_bin/i_dut/i_occamy/i_occamy_quadrant_s1_0

for {set i 0} {$i < 2} {incr i} {
    add wave -group "Cluster${i}" -port $quadrant/i_occamy_cluster_${i}/*
    for {set ii 0} {$ii < 9} {incr ii} {
        add wave -group "Cluster${i}" -group "Snitch${ii}" $quadrant/i_occamy_cluster_${i}/i_cluster/gen_core[${ii}]/i_snitch_cc/i_snitch/*
    }
}


configure wave -namecolwidth  250
configure wave -valuecolwidth 100
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -timelineunits ns
