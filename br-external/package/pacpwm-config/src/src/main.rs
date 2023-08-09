use std::fs::OpenOptions;
use memmap2::{MmapRaw, MmapOptions};

use clap::{Parser, ArgGroup};
use clap_num::maybe_hex;

// Base address in OCM for PS-PL DMA
const OCM_BASEADDR_CTRL: u64 = 0xFFFFF800; 

// Offsets for PACPWM configuration
const OFFSET_CMPA: usize = 0x00;      //u16
const OFFSET_CLKDIV: usize = 0x02;    //u16
const OFFSET_PHASEINC: usize = 0x04;  //u32

// Base address for AXI-PL registers
const AXI_BASEADDR_REGS: u64 = 0x80000000;

// Offsets for AXI registers
const OFFSET_REG_TRIGGERS: usize = 0x10; //u32
const OFFSET_REG_TRIPCTRL: usize = 0x20; //u32
const OFFSET_REG_TRIPMASK: usize = 0x24; //u32


/// Simple program to greet a person
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
#[clap(group(
    ArgGroup::new("at_least_one")
        .required(true)
        .multiple(true)
        .args(&["phaseinc", "compare", "clkdiv", "reset"]),
))]
struct Args {
    /// Phase increment (32-bit)
    #[arg(short = 'i', long, value_parser=maybe_hex::<u32>)]
    phaseinc: Option<u32>,

    /// Modulator compare value (16-bit)
    #[arg(short = 'c', long, value_parser=maybe_hex::<u16>)]
    compare: Option<u16>,

    /// Modulator clock divider. Specify 0 for no division, 1 for /2 etc.
    #[arg(short = 'd', long, value_parser=maybe_hex::<u16>)]
    clkdiv: Option<u16>,

    /// Mask all trip sources and reset tripped status.
    #[arg(short, long)]
    reset: bool, 
}

fn main() {
    let args = Args::parse();

    // Open /dev/mem read/write to access device memory
    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .open("/dev/mem");

    if let Err(e) = file {
        println!("ERROR: Failed to open /dev/mem. Reason: {}", e.to_string());
        return
    }
    let file = file.unwrap(); // Unwrap is OK here due to test for Err above
    
    // mmap OCM region of interest
    let mmap = MmapOptions::new()
        .offset(OCM_BASEADDR_CTRL)
        .len(8)
        .map_raw(&file); 

    if let Err(e) = mmap {
        println!("ERROR: Failed to mmap OCM @ 0x{:08X}. Reason: {}", OCM_BASEADDR_CTRL, e.to_string());
        return
    }

    let mut mmap = mmap.unwrap(); // Unwrap is OK here due to test for Err above

    // Write to shared memory
    if let Some(val) = args.compare {
        write_compare(&mut mmap, val);
    }
    
    if let Some(val) = args.phaseinc {
        write_phaseinc(&mut mmap, val);
    }

    if let Some(val) = args.clkdiv {
        write_clkdiv(&mut mmap, val);
    }

    let _ = mmap.flush();

    // mmap AXI registers
    let mmap = MmapOptions::new()
        .offset(AXI_BASEADDR_REGS)
        .len(0x110050)
        .map_raw(&file);

    if let Err(e) = mmap {
        println!("ERROR: Failed to mmap AXI regs @ 0x{:08X}. Reason: {}", AXI_BASEADDR_REGS, e.to_string());
        return
    }

    let mut mmap = mmap.unwrap(); // Unwrap is OK here due to test for Err above

    // manually trigger PS-PL DMA
    trigger_dma(&mut mmap);

    if args.reset {
        // reset trip module
        trip_reset(&mut mmap);
    }

    let _ = mmap.flush();
    
}

fn write_compare(mmap: &mut MmapRaw, val: u16) {
    unsafe {
        mmap.as_mut_ptr().add(OFFSET_CMPA).cast::<u16>().write_volatile(val);
    }
}

fn write_clkdiv(mmap: &mut MmapRaw, val: u16) {
    unsafe {
        mmap.as_mut_ptr().add(OFFSET_CLKDIV).cast::<u16>().write_volatile(val);
    }
}

fn write_phaseinc(mmap: &mut MmapRaw, val: u32) {
    unsafe {
        mmap.as_mut_ptr().add(OFFSET_PHASEINC).cast::<u32>().write_volatile(val);
    }
}

fn trigger_dma(mmap: &mut MmapRaw) {
    unsafe {
        mmap.as_mut_ptr().add(OFFSET_REG_TRIGGERS).cast::<u32>().write_volatile(1);
    }
}

fn trip_reset(mmap: &mut MmapRaw) {
    unsafe {
        mmap.as_mut_ptr().add(OFFSET_REG_TRIPMASK).cast::<u32>().write_volatile(0xFFFFFFFF); // Mask all trip sources
        mmap.as_mut_ptr().add(OFFSET_REG_TRIPCTRL).cast::<u32>().write_volatile(0x80000000); // Reset trip module
    }
}
