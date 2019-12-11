use buffered_reader;
use byteorder::{NativeEndian, ReadBytesExt, WriteBytesExt};
use std::fs::File;
use std::path::Path;
use std::io::BufWriter;
use std::sync::mpsc;
use std::thread;
use std::time::Instant;

// some const
const PREFIX_LEN: usize = 44;
// the sequnce of the prefixes is optimized by putting the most frequent ones to the front
const PREFIX_LIST: [usize; PREFIX_LEN] = [
    186, 158, 135, 159,
    136, 150, 137, 138,
    187, 151, 182, 152,
    139, 183, 188, 134,
    185, 189, 180, 157,
    155, 156, 131, 132,
    133, 130, 181, 176,
    177, 153, 184, 178,
    173, 147, 175, 199,
    166, 170, 198, 171,
    191, 145, 165, 172
];
const MOBILE_SPAN_LEN: usize = 100000000;
const TOTAL_NUM: usize = PREFIX_LEN * MOBILE_SPAN_LEN;
const SLICE_LEN: usize = u16::max_value() as usize + 1;
const SLICE_NUM: usize = TOTAL_NUM / SLICE_LEN + 1;

// the data structure of the formula
pub struct Pair {
    md5: u16,          // the first 16 digits of the md5 digest
    mobile_index: u16, // the index in each slice
}

pub struct Decoder {
    seq :u32,
    q_sender: Vec<mpsc::Sender<(u32, String)>>,
    a_receiver: mpsc::Receiver<(u32, u64)>
}

impl Decoder {    
    pub fn look_up(&mut self, digest_str: &String) -> u64{
        // validate digest_str
        match hex::decode(digest_str) {
            Ok(digest) => if digest.len() != 16 {return 0},
            Err(_) => return 0,
        }

        let start = Instant::now();
        // send request
        for sender in &self.q_sender {
            let q = (self.seq, digest_str.to_string());
            sender.send(q).unwrap();
        }
        // wait for response
        let mut result = 0;
        let mut resp_count = 0;
        loop {
            let a = self.a_receiver.recv().unwrap();
            if a.0 == self.seq {
                if a.1 > 0 {
                    result = a.1;
                    break;
                } else {
                    resp_count += 1;
                }
                if resp_count >= self.q_sender.len() {
                    break;
                }
            }
        }
        // send complete notification
        for sender in &self.q_sender {
            let q = (self.seq, digest_str.to_string());
            sender.send(q).unwrap();
        }
        println!("{},{},{} costed {:?}", self.seq, digest_str, result, start.elapsed());
        // increase seq
        self.seq += 1;
        result
    }

    // pub fn test(&mut self) {
    //     println!("run test ...");
    //     use rand::{thread_rng, Rng};
    //     let mut rng = thread_rng();
    //     let start = Instant::now();
    
    //     for prefix in 0..PREFIX_LEN {
    //         let start = Instant::now();
    //         let mobile = PREFIX_LIST[prefix] * MOBILE_SPAN_LEN + rng.gen_range(0, MOBILE_SPAN_LEN);
    //         let digest = format!("{:?}", md5::compute(mobile.to_string()));
    //         let result = self.look_up(&digest);
    //         println!("{},{},{},{:?}", mobile, digest, result, start.elapsed());
    //     }
    
    //     let costs = start.elapsed().as_millis() as usize / PREFIX_LEN;
    //     let costs = costs as f32 / 1000.;
    //     println!("average costs {}s", costs);
    // }
}

pub fn init() -> Decoder {
    println!("Initializing decoder ...");
    let thread_num = num_cpus::get();
    let (a_sender, a_receiver) = mpsc::channel();
    let mut decoder = Decoder {
        seq: 0,
        q_sender: Vec::new(),
        a_receiver: a_receiver
    };
    // start threads
    for threadid in 0..thread_num {
        println!("Thread {} started", threadid);
        // prepare message channels
        let (q_sender, q_receiver) = mpsc::channel();
        decoder.q_sender.push(q_sender);
        let a_sender_clone = mpsc::Sender::clone(&a_sender);
        // create threads
        thread::spawn(move || {
            let mut thread_slice_num = SLICE_NUM / thread_num;
            if threadid < SLICE_NUM % thread_num {
                thread_slice_num += 1;
            }        
            // alloc memory
            let mut v = alloc_formula_vec(thread_slice_num);
            // load fomula
            load_formula(&mut v, threadid, thread_num, thread_slice_num);
            // notify init completed
            a_sender_clone.send((0,0)).unwrap();
            // start to listen ...
            loop {
                let (seq, digest_str) = q_receiver.recv().unwrap();
                // println!("{},{},{}", threadid, seq, digest_str);

                let mut digest = md5::Digest::from(md5::Context::new());
                let str_decoded = hex::decode(digest_str).unwrap();
                for i in 0..digest.len() {
                    digest[i] = str_decoded[i];
                }
                let m_md5 = digest_to_md5(digest);
                let mut found = false;
                let mut skip = false;
                for slice in 0..thread_slice_num {
                    let mobile = look_up_in_slice(&v, threadid, thread_num, slice, digest, m_md5);
                    if mobile > 0 {
                        found = true;
                        // println!("{},{},{} found", threadid, seq, mobile);
                        a_sender_clone.send((seq,mobile)).unwrap();
                        break;
                    }
                    // check if completed by other thread
                    if q_receiver.try_recv().is_ok() {
                        skip = true;
                        break;
                    }
                }
                // wait for complete notification
                if !skip {
                    if !found {
                        // println!("{},{} not found", threadid, seq);
                        a_sender_clone.send((seq,0)).unwrap();
                    }
                    while q_receiver.try_recv().is_err() {}
                }
            }
        });
    }

    // wait for init completed
    for _ in 0..thread_num {
        decoder.a_receiver.recv().unwrap();
    }
    println!("Decoder init completed");
    decoder
}

fn alloc_formula_vec(thread_slice_num: usize) -> Vec<Pair> {
    let len = thread_slice_num * SLICE_LEN;
    let mut v: Vec<Pair> = Vec::with_capacity(len);
    unsafe {
        v.set_len(len);
    }
    v
}

fn load_formula(v: &mut Vec<Pair>, threadid: usize, thread_num: usize, thread_slice_num: usize) {
    let formula_file = format!("formula{}.dat", threadid);
    if Path::new(&formula_file).exists() {
        read_formula_from_file(v, &formula_file);
    } else {
        build_formula(v, threadid, thread_num, thread_slice_num);
        write_formula_to_file(v, &formula_file);
    }
}

fn build_formula(v: &mut Vec<Pair>, threadid: usize, thread_num: usize, thread_slice_num: usize) {
    let start = Instant::now();
    for slice in 0..thread_slice_num {
        // if slice % 100 == 0 {println!("{},{}",threadid,slice);}
        // cumpute md5 hash in each slice
        for mobile_index in 0..SLICE_LEN {
            let address = (slice * thread_num + threadid) * SLICE_LEN + mobile_index;
            if address >= TOTAL_NUM {
                break;
            }
            let mobile = address_to_mobile(address);
            let m_md5 = mobile_to_md5(mobile);
            v[slice * SLICE_LEN + mobile_index].md5 = m_md5;
            v[slice * SLICE_LEN + mobile_index].mobile_index = mobile_index as u16;
        }
        // sort each slice
        let v_slice = & mut v[slice * SLICE_LEN..(slice+1) * SLICE_LEN];
        v_slice.sort_by(|a, b| a.md5.cmp(&b.md5));
    }
    println!("Build formula{} costed {:?}", threadid, start.elapsed());
}

fn write_formula_to_file(v: &Vec<Pair>, formula_file: &String) {
    let start = Instant::now();
    let mut file = BufWriter::new(File::create(formula_file).unwrap());
    for i in 0..v.len() {
        file.write_u16::<NativeEndian>(v[i].md5).unwrap();
        file.write_u16::<NativeEndian>(v[i].mobile_index).unwrap();
    }
    println!("Write file {} costed {:?}", formula_file, start.elapsed());
}

fn read_formula_from_file(v: &mut Vec<Pair>, formula_file: &String) {
    let start = Instant::now();
    let mut file = buffered_reader::File::open(formula_file).unwrap();
    for i in 0..v.len() {
        v[i].md5 = file.read_u16::<NativeEndian>().unwrap();
        v[i].mobile_index = file.read_u16::<NativeEndian>().unwrap();
    }
    println!("Read file {} costed {:?}", formula_file, start.elapsed());
}

fn address_to_mobile(address: usize) -> u64 {
    (PREFIX_LIST[address / MOBILE_SPAN_LEN] * MOBILE_SPAN_LEN + address % MOBILE_SPAN_LEN) as u64
}

fn mobile_to_md5(mobile: u64) -> u16 {
    let digest = md5::compute(mobile.to_string());
    digest_to_md5(digest)
}

fn digest_to_md5(digest: md5::Digest) -> u16 {
    let d0 = digest[0] as u16;
    let d1 = digest[1] as u16;
    (d0 << 8) + d1
}

fn look_up_in_slice(v: &Vec<Pair>, threadid: usize, thread_num: usize, slice: usize, digest: md5::Digest, m_md5: u16) -> u64 {
    let mut mobile = 0;
    let v_slice = &v[slice * SLICE_LEN..(slice+1) * SLICE_LEN];
    let result = v_slice.binary_search_by(|probe| probe.md5.cmp(&m_md5));
    match result {
        Ok(index) => {
            let mut i = index;
            while i > 0 && v_slice[i].md5 == m_md5 {
                let mobile_index = v_slice[i].mobile_index;
                let address = (slice * thread_num + threadid) * SLICE_LEN + mobile_index as usize;
                let mobile_try = address_to_mobile(address);
                
                if md5::compute(mobile_try.to_string()) == digest {
                    mobile = mobile_try;
                    break;
                }
                i -= 1;
            }
        }
        Err(_) => {
            // println!("{} not found", slice);
        }
    }
    mobile
}

