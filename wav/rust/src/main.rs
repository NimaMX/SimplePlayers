extern crate byteorder;

use std::fs::File;
use std::env;
use std::mem;
use std::slice;
use std::io::{Read, BufReader};

use byteorder::{ ReadBytesExt, NativeEndian };
use alsa::{Direction, ValueOr, pcm::PCM, pcm::HwParams, pcm::Format, pcm::Access, pcm::State };

#[repr(C, packed)]
#[derive(Debug, Copy, Clone)]
struct WaveHeader {
    riff: [u8; 4],
    chunk_size: u32,
    wave: [u8; 4],
    fmt: [u8; 4],
    sub_chunk_size: u32,
    audio_format: u16,
    channel_num: u16,
    sample_per_sec: u32,
    byte_per_sec: u32,
    block_align: u16,
    bit_per_sample: u16,
    sub_chunk_id: [u8; 4],
    sub_chunk2_size: u32,
}

fn main() {
   
    let args: Vec<String> = env::args().collect();
    if args.len() > 1 {    
        println!("No file path given, please pass the wave file");
    }
    
    println!("The given file path is : {}" , &args[1]);
                                                                                                   
    let mut file = match File::open(&args[1]) {
        Ok(f) => f,
        Err(e) => {
            println!("Failed to open the file : {} ", e);
            return;
        }
    };

    let mut wave_header: WaveHeader = unsafe { mem::zeroed()  };
    let wave_header_size = mem::size_of::<WaveHeader>();
    unsafe {
        
        let config_slice = slice::from_raw_parts_mut(&mut wave_header as *mut _ as *mut u8, wave_header_size);
        
        match file.read_exact(config_slice) {
            Ok(f) => f,
            Err(e) => {
                println!("Failed to read the header : {}", e);
                return;
            }
        };
    }                                                                                            

    let pcm_device = match PCM::new("default" , Direction::Playback, false) {
        Ok(f) => f,
        Err(e) => {
            println!("Failed to open the pcm device : {}", e);
            return;
        } 
    };

    let hw_param = HwParams::any(&pcm_device).unwrap();
    hw_param.set_channels(u32::from(wave_header.channel_num)).unwrap();
    hw_param.set_rate(wave_header.sample_per_sec, ValueOr::Nearest).unwrap();
    hw_param.set_format(Format::s16()).unwrap();
    hw_param.set_access(Access::RWInterleaved).unwrap();
    pcm_device.hw_params(&hw_param).unwrap();
    
    let pcm_handler = pcm_device.io_i16().unwrap();
    let hw_par = pcm_device.hw_params_current().unwrap();
    let sw_par = pcm_device.sw_params_current().unwrap();
    sw_par.set_start_threshold(hw_par.get_buffer_size().unwrap()).unwrap();
    pcm_device.sw_params(&sw_par).unwrap();

    println!("The header structure : {:#?} ", wave_header);
    
    // let frames: Frames = HwParams::get_period_size(&hw_param).unwrap();
    // let buffer_size: usize = usize::try_from(i64::from(wave_header.channel_num) * frames * 2).unwrap();
    // println!("The buffer_size is => {} ", buffer_size);
    
    let mut buffer = [0i16; 4096];
    let mut reader = BufReader::new(file);
    let mut state: bool = false;

    loop {

        if state { break; }

        match reader.read_i16_into::<NativeEndian>(&mut buffer[..]) {
            Ok(f) => f,
            Err(_e) => {
                state = true;
            },
        };

        pcm_handler.writei(&buffer[..]).unwrap();
    }

    pcm_device.drain().unwrap();
}


