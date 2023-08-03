#!/bin/sh

cd ${BINARIES_DIR}
if test -f "zynq-cosmos-xzq10-qut.dtb"; then
	cp zynq-cosmos-xzq10-qut.dtb devicetree.dtb
    rm zynq-cosmos-xzq10-qut.dtb
fi
cp $2/board/cosmos/fpga.bit fpga.bit
cp $2/board/cosmos/boot.bif boot.bif
cp $2/board/cosmos/fsbl.elf fsbl.elf
mkbootimage boot.bif boot.bin
rm boot.bif
rm fsbl.elf
rm fpga.bit
rm rootfs.cpio.gz
