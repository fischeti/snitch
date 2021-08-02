
## Gianna Setup
# export CXX=g++-9.2.0
# export CC=gcc-9.2.0
# export CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/pack/gcc-9.2.0-af/linux-x64/bin/gcc
# export LLVM_SYS_100_PREFIX=/scratch/pauling/install/llvm/llvm-project-10.0.0/install

## Noah Setup
export CXX=g++-9.2.0
export CC=gcc-9.2.0
export CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/pack/gcc-9.2.0-af/linux-x64/bin/gcc
export LLVM_SYS_100_PREFIX=/home/sem21f15/.llvm10

export RISCV=/usr/pack/riscv-1.0-kgf/experimental/pulp-gcc-2.2.0-v2sf/bin/
# export RISCV=/home/pauling/.riscv-smallfloat/bin/
# export RISCV=/home/pauling/.riscv/
# export RISCV=/home/pauling/.local/bin/
# export RISCV=/usr/scratch/pilatus/smach/riscv-transprec/bin
# export RISCV=/home/balasr/.pulp_riscv/1.0.16/
# export RISCV=/usr/scratch/pilatus/smach/riscv_install_smallfloat/bin
export PATH=$RISCV:$PATH

export PATH=/home/zarubaf/verilator/bin:$PATH
export PATH=/usr/scratch/zanua/pauling/projects/occamy/dtc:$PATH
export PATH=/usr/scratch/zanua/pauling/projects/occamy/riscv-isa-sim/build:$PATH

alias cmake="cmake-3.18.1"

# export SNITCH_LOG=info
# export SNITCH_LOG=warn
# export SNITCH_LOG=error
export SNITCH_LOG=debug



# export PATH="/home/pauling/.local/install/riscv32-snitch-llvm-centos7-12/bin:$PATH"
