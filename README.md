# buildroot-cosmos

This repository hosts the Buildroot toolchain required to build a Linux OS and associated utilities targeting the Enclustra Cosmos XZQ10 FPGA SoC board used by the QUT PHiLICS system. Specifically it builds an image which includes a Zynq first-stage bootloader (FSBL), U-boot bootloader, Linux kernel and root filesystem, suitable for booting on the Cosmos XZQ10, and which includes configuration and utilities suiting the QUT PHiLICS system.

## Getting started

### Dependencies

All building and testing has been performed on a Ubuntu 22.04 installation. For Windows users, a WSL2 installation of Ubuntu 22.04 is recommended, however, it should also be possible to use a Docker container.

Buildroot requires a number of dependencies which are documented [here](https://buildroot.org/downloads/manual/manual.html#requirement-mandatory). The following commands install additional required packages for a fresh Ubuntu 22.04 installation on WSL2:

```
sudo apt-get update
sudo apt install git build-essential cmake pkg-config libssl-dev libncurses-dev unzip bc
```

### Installing

```
git clone --recurse-submodules https://github.com/qut-philics/buildroot-cosmos
cd buildroot-cosmos/buildroot
make BR2_EXTERNAL=../br-external zynq_cosmos_defconfig
```

### Building

From the `./buildroot-cosmos/buildroot` directory, execute the command `make` to build the image.

### Build artefacts

All generated build artefacts are located in `./buildroot-cosmso/buildroot/output/images`.

### Installation on the Cosmos XZQ10 board

The only file required to boot the board is `boot.bin`. This file must be placed in the root directory of a FAT-formatted SD card and inserted into the SD socket of the Cosmos board.

For ease of prototyping, a number of system components included in `boot.bin` may be optionally overridden by also placing, in the root directory of the SD card, the U-boot `boot.scr` script, along with zero or more of the following components:

* fpga.bit (bitfile to load onto PL at boot)
* uImage (Linux kernel and initramfs) 
* devicetree.dtb (device tree blob)

## Basic usage

You can connect to the running system via the USB-UART connection next to the SD card socket (115.2 kBaud, 8N1), or SSH via the Gigabit ethernet port. By default the system is configured to acquire an IP address via DHCP, and use the hostname `cosmos.local`.

The default username/password `qut/qut` can be used to login. Root credentials are `root/root`.

## Customising the image components

### Basic customisation

The following commands allows you to customise the root filesystem, Linux kernel and U-boot bootloader respectively:

```
make menuconfig
make linux-menuconfig
make uboot-menuconfig
```
Most project should only need to make minor changes to the root filesystem using the `make menuconfig` command. In general, it should not be necessary to modify the kernel or U-boot images (unless, for example, you intend to modify the existing devicetree). Note that if you do modify the kernel or U-boot configurations, your modifications will be lost on a `make clean` (see later sections for explanations on how to make persistent changes).

### Default configurations

Customisations and configurations specific to the QUT PHiLICS project and Cosmos XZQ10 board are stored in a Buildroot external tree, located in the `./buildroot-cosmos/br-external` directory. The default configurations for each of the system components, respectively, are located in:

```
/br-external/configs/zynq_cosmos_defconfig
/br-external/board/cosmos/kernel.config
/br-external/board/uboot.config
```

#### Updating the Buildroot defconfig

After making any changes via `make menuconfig`, execute `make savedefconfig` to update the defconfig file to include your changes.

If you are creating a project-specific defconfig which you would like to store permanently, rather than updating the default defconfig, you might like to change the path in `Build options -> Location to save buildroot config` to point to a new defconfig file for your project. This file should be located in `/br-external/configs`. You can then recreate a clean copy of your project in the future by executing `make BR2_EXTERNAL=../br-external your_new_defconfig` after cloning the repository.

#### Updating Linux or U-boot default configurations

After customising the Linux or U-boot configurations, these changes can be permanently stored by copying to `.config` files from `/buildroot/output/build/linux-custom` or `/buildroot/output/build/uboot-2023.04` to `/br-external/board/cosmos/kernel.config` or `/br-external/board/cosmos/uboot.config` respectively.

#### FPGA bitstream

By default, the `boot.bin` image is built including the FPGA bitstream selected via the `/br-external/package/philics-pl` package. This bitstream is loaded by the FSBL onto the FPGA fabric at boot. This bitstream can be subsequently overwritten by U-boot via the mechanisms described above, however, if a new default bitstream is permanently desired for the project, an alternative bitstream can be selected via the `philics-pl` package, or if it does not already exist, added to the `philics-pl` package.

## QUT packages

This repository includes a number of packages that are built as part of the root file system that provide utilities specific to the PHiLICS system. This includes binaries to run from within Linux, as well as baremetal applications that can be loaded and run on the second ARM core.

These packages are all located within `/br-external/package`. Any additional software built for the platform should be added to this directory as an additional package. These packages can be selected/de-selected for inclusion in the root filesystem image fom the Buildroot `External options` menu.

### Adding packages

You should refer to the Buildroot manual for detailed information about adding packages to the external tree. For examples you can refer to the `zrproc` package for an example of a Linux binary written in C, `pacpwm-config` for a Linux binary written in Rust, and `zrproc-firmware` for building baremetal applications. After adding a new package you will need to include a line in `/br-external/Config.in` referencing it, and select it from the Buildroot `External options` menu for it to be included in the build.

### Individually building packages

When developing new packages it can be useful to be able to independently rebuild these packages. This can be achieved via the `make *-rebuild` command. For example for the `zrproc-firmware` package:

```
make zrproc-firmware-rebuild
```

Should you need to ensure a full, clean, rebuild of a package, the build outputs for an individual package may be deleted from the `/buildroot/output/build` directory.

### Baremetal applications

The `zrproc-firmware` package contains a set of sub-projects that build baremetal applications intended to be loaded by `zrproc` to run on CPU1 and to implement real-time control applications for the PHiLICS system. Each sub-project is placed in a separate folder in `/br-external/package/zrproc-firmware/src/firmware`, and is installed into `/usr/lib/zrproc` in the resulting root filesystem image.

To add a new baremetal application, a new folder should be added in the `firmware` directory, following the template demonstrated by existing projects. The binary built must match the name of the subfolder created in order for the file to be correctly found and installed by the build system. Updating the source filename and first line of the Makefile to reflect the application name should be the only modifications required in most cases.

Note that these baremetal applciations link against the `libxil.a` library located in the `/zrproc-firmware/src/bsp` directory. This library was build was built using the `philics-bsp` repository and Xilinx SDK 2017.3, against a default hardware configuration of the Zynq-7000 processor system (PS) (i.e. the PHiLICS project). While it is not expected that this library should need to be updated, it may be required if significant changes are made to the PS configuration in the future.

### Useful QUT-developed tools

* bitstream-timestamp - prints timestamp of the loaded FPGA bitstream (included in all PHiLICS designs)
* zrproc <stop | start FIRMWARE | status> - loads and executes baremetal firmware onto CPU1




