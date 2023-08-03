echo Executing boot.scr...
setenv bootargs 'console=ttyPS0,115200'
if test -e mmc 0 fpga.bit;
  then echo Found fpga.bit. Loading to memory...; 
  fatload mmc 0 0x100000 fpga.bit; 
  echo Programming PL...;
  fpga loadb 0 0x100000 $filesize; 
fi
if test -e mmc 0 uImage; 
  then echo Found uImage. Loading to memory...; 
  fatload mmc 0 0x3000000 uImage; 
fi
if test -e mmc 0 devicetree.dtb;
  then echo Found devicetree.dtb. Loading to memory...;
  fatload mmc 0 0x2000000 devicetree.dtb; 
fi
echo Booting kernel image...
bootm 0x3000000 - 0x2000000
