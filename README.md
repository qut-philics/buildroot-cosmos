# buildroot-cosmos

## Dependencies

Buildroot requires a number of dependencies which are documented [here](https://buildroot.org/downloads/manual/manual.html#requirement-mandatory). The following commands install additional required packages for a fresh Ubuntu 22.04 installation on WSL2:

```
sudo apt-get update
sudo apt install git build-essential cmake pkg-config libssl-dev libncurses-dev unzip bc
```

## Installing

```
git clone --recurse-submodules https://github.com/qut-cosmos/buildroot-cosmos
cd buildroot-cosmos
make BR2_EXTERNAL=../br-external zynq_cosmos_defconfig
```

## Building

From the `buildroot` directory, execute the command `make` to build the image.

## Build artefacts

All build artefacts are located in `buildroot/output/images`.

## Installation on Cosmos XZQ10 board

The only file required to boot the board is `boot.bin`. This file must be placed in the root directory of a FAT formatted SD card and inserted into the SD socket of the Cosmos board.

For ease of prototyping, a number of system components included in `boot.bin` may be optionally overridden by also including in the root directory of the SD card the U-boot `boot.scr` script, along with zero or more of the following components:

* fpga.bit (bitfile to load onto PL at boot)
* uImage (Linux kernel and initramfs) 
* devicetree.dtb (device tree blob)

