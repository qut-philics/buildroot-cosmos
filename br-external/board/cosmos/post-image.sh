#!/bin/sh

cd ${BINARIES_DIR}
if test -f "zynq-cosmos-xzq10-qut.dtb"; then
	cp zynq-cosmos-xzq10-qut.dtb devicetree.dtb
    rm zynq-cosmos-xzq10-qut.dtb
fi
cp $2/board/cosmos/fsbl.elf fsbl.elf
mkbootimage $2/board/cosmos/boot.bif boot.bin
