use buffered_reader;
use byteorder::{NativeEndian, ReadBytesExt, WriteBytesExt};
use console::Term;
use std::env;
use std::fs::File;
use std::io::{Write, BufWriter};
use std::sync::{mpsc, Arc, Mutex};
use std::thread;
use std::time::Instant;

// some const
const THREAD_NUM: usize = 4;
// const PREFIX_LEN: usize = 1;
// const PREFIX_LIST: [usize; PREFIX_LEN] = [134];
const PREFIX_LEN: usize = 42;
// the sequnce of the prefixes is optimized by putting the most frequent ones to
// the front inside each 4 group
const PREFIX_LIST: [usize; PREFIX_LEN] = [
    134, 135, 136, 137, 138, 139, 147, 150, 151, 152, 157, 158, 159, 178, 182, 183, 184, 187, 188,
    198, 133, 149, 153, 173, 177, 180, 181, 189, 199, 130, 131, 132, 145, 155, 156, 166, 175, 176,
    185, 186, 170, 171,
];
const MOBILE_SPAN_LEN: usize = 100000000;
const TOTAL_NUM: usize = PREFIX_LEN * MOBILE_SPAN_LEN;
const SLICE_LEN: usize = u16::max_value() as usize + 1;
const SLICE_NUM: usize = TOTAL_NUM / SLICE_LEN + 1;

// the data structure of the formula
struct Pair {
    md5: u16,          // the first 16 digits of the md5 digest
    mobile_index: u16, // the index in each slice
}

fn main() {
    // alloc memory
    let v_mutex = Arc::new(Mutex::new(alloc_big_vec(TOTAL_NUM)));

    // parse commands
    let args: Vec<String> = env::args().collect();
    if args.len() == 1 {
        print_usage();
    } else {
        match args[1].as_str() {
            "-b" => {
                build_formula(&v_mutex);
                write_formula_to_file(&v_mutex);
            }
            "-t" => {
                build_formula(&v_mutex);
                test(&v_mutex);
            }
            "-tf" => {
                read_formula_from_file(&v_mutex);
                test(&v_mutex);
            }
            "-d" => {
                if args.len() < 3 {
                    println!("please add a filename!")
                } else {
                    // build_formula(&v_mutex);
                    decode_file(&v_mutex, &args[2]);
                }
            }
            "-df" => {
                if args.len() < 3 {
                    println!("please add a filename!")
                } else {
                    read_formula_from_file(&v_mutex);
                    decode_file(&v_mutex, &args[2]);
                }
            }
            _ => print_usage(),
        }
    }
}

fn print_usage() {
    println!("md5mobile 0.1.0");
    println!("Herman Wu");
    println!("");
    println!("md5mobile builds a formula in memory to decode md5 hash value back to the original mobile number. It supports Chinese mobile number in the format of 13812345678.");
    println!("");
    println!("Usage:");
    println!("  md5mobile [Flags] [Filename]");
    println!("Flags:");
    println!("  -d    decode the hashes in given file.");
    println!("  -t    generate random mobiles and test");
    println!("  -b    build formula and write to file \"formula.dat\"");
    println!("  -f    load from \"formula.dat\" at first. usage: -df, -tf");
    println!("Filename:");
    println!("  csv file which contains hashes to be decoded.");
}

fn build_thread_slice_array() -> [(usize, usize); THREAD_NUM] {
    let mut thread_slice_array = [(0, 0); THREAD_NUM]; // each thread's slice (start, length)
    for threadid in 0..THREAD_NUM {
        // assign slices to threads
        let mut thread_slice_num = SLICE_NUM / THREAD_NUM;
        if threadid < SLICE_NUM % THREAD_NUM {
            thread_slice_num += 1;
        }
        let mut slice_start = 0;
        if threadid > 0 {
            slice_start = thread_slice_array[threadid - 1].0 + thread_slice_array[threadid - 1].1;
        }
        thread_slice_array[threadid] = (slice_start, thread_slice_num);
    }
    thread_slice_array
}

fn build_formula(v_mutex: &Arc<Mutex<Vec<Pair>>>) {
    let start = Instant::now();
    println!("Start to build formula ...");
    let (tx, rx) = mpsc::channel();

    let thread_slice_array = build_thread_slice_array();
    for threadid in 0..THREAD_NUM {
        println!("Thread {} starting ...", threadid);
        //create threads
        let v_clone = v_mutex.clone();
        let tx_clone = mpsc::Sender::clone(&tx);
        thread::spawn(move || {
            // alloc local thread memory
            let mut v_thread = alloc_big_vec(SLICE_LEN);
            // work on each slice
            for thread_slice in 0..thread_slice_array[threadid].1 {
                let slice = thread_slice_array[threadid].0 + thread_slice;
                // cumpute md5 hash
                for mobile_index in 0..SLICE_LEN {
                    let address = slice * SLICE_LEN + mobile_index;
                    if address < TOTAL_NUM {
                        let mobile = address_to_mobile(address);
                        let m_md5 = mobile_to_md5(mobile);
                        v_thread[mobile_index].md5 = m_md5;
                        v_thread[mobile_index].mobile_index = mobile_index as u16;
                    }
                }
                // sort each slice
                v_thread.sort_by(|a, b| a.md5.cmp(&b.md5));

                // write to main memory
                let mut v = v_clone.lock().unwrap();
                for mobile_index in 0..SLICE_LEN {
                    let address = slice * SLICE_LEN + mobile_index;
                    if address < TOTAL_NUM {
                        v[address].md5 = v_thread[mobile_index].md5;
                        v[address].mobile_index = v_thread[mobile_index].mobile_index;
                    }
                }

                // send progress to main thread
                tx_clone.send((threadid, thread_slice)).unwrap();
            }
        });
    }
    drop(tx);

    // recieve progress from threads
    let term = Term::stdout();
    for (threadid, thread_slice) in rx {
        // print progress
        term.move_cursor_up(THREAD_NUM - threadid).unwrap();
        term.clear_line().unwrap();
        term.write_str(&format!(
            "Thread {} slice {}/{} completed. @{:?}",
            threadid,
            thread_slice + 1,
            thread_slice_array[threadid].1,
            start.elapsed()
        ))
        .unwrap();
        term.move_cursor_down(THREAD_NUM - threadid).unwrap();
        term.clear_line().unwrap();
    }

    println!(
        "# Main thread formula completed after {:?}",
        start.elapsed()
    );
}

fn alloc_big_vec(len: usize) -> Vec<Pair> {
    let mut v: Vec<Pair> = Vec::with_capacity(len);
    unsafe {
        v.set_len(len);
    }
    v
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

fn restore_mobile(mobile_index: u16, slice: usize) -> u64 {
    let address = slice * SLICE_LEN + mobile_index as usize;
    address_to_mobile(address)
}

fn look_up(v_mutex: &Arc<Mutex<Vec<Pair>>>, digest_str: &str) -> u64 {
    let mut digest = md5::Digest::from(md5::Context::new());
    let str_decoded = hex::decode(digest_str).unwrap();
    for i in 0..digest.len() {
        digest[i] = str_decoded[i];
    }
    look_up_digest(&v_mutex, digest)
}

fn look_up_digest(v_mutex: &Arc<Mutex<Vec<Pair>>>, digest: md5::Digest) -> u64 {
    // let start = Instant::now();
    
    let m_md5 = digest_to_md5(digest);
    let found = Arc::new(Mutex::new(false));

    let (tx, rx) = mpsc::channel();
    let thread_slice_array = build_thread_slice_array();
    for threadid in 0..THREAD_NUM {
        //create threads
        let v_clone = v_mutex.clone();
        let tx_clone = mpsc::Sender::clone(&tx);
        let found = found.clone();
        thread::spawn(move || {
            // work on each slice
            for thread_slice in 0..thread_slice_array[threadid].1 {
                let slice = thread_slice_array[threadid].0 + thread_slice;
                let slice_start = slice * SLICE_LEN;
                let mut slice_end = slice_start + SLICE_LEN;
                if slice_end > TOTAL_NUM {
                    slice_end = TOTAL_NUM;
                }
                let v = v_clone.lock().unwrap();
                let v_slice = &v[slice_start..slice_end];
                let result = v_slice.binary_search_by(|probe| probe.md5.cmp(&m_md5));
                match result {
                    Ok(index) => {
                        let mut i = index;
                        while i > 0 && v_slice[i].md5 == m_md5 {
                            let mobile_index = v_slice[i].mobile_index;
                            let mobile = restore_mobile(mobile_index, slice);
                            // println!("{},{},{}", threadid, slice, mobile);
                            
                            if md5::compute(mobile.to_string()) == digest {
                                let mut found = found.lock().unwrap();
                                *found = true;
                                // println!("## {}", mobile);
                                // send result to main thread
                                tx_clone.send((true, mobile)).unwrap();
                                break;
                            }
                            i -= 1;
                        }
                    }
                    Err(_) => {
                        // println!("{} not found", slice);
                    }
                }
                // check if found by other threads
                let found = found.lock().unwrap();
                if *found {
                    break;
                }
            }
        });
    }
    drop(tx);

    // recieve results from threads
    for (found, mobile) in rx {
        if found {
            // println!("found {}", mobile);
            // println!("Look up costed {:?}", start.elapsed());
            return mobile;
        }
    }

    // println!("Look up costed {:?}", start.elapsed());
    return 0;
}

fn write_formula_to_file(v_mutex: &Arc<Mutex<Vec<Pair>>>) {
    let start = Instant::now();
    let v_clone = v_mutex.clone();
    let v = v_clone.lock().unwrap();
    let term = Term::stdout();
    let mut percent = 0;
    term.write_str(&format!("Writing formula to file ...{}%", percent))
        .unwrap();

    // write md5 then mobile brings better compress ratio
    // however it increases memory access costs
    let mut file = BufWriter::new(File::create("formula.dat").unwrap());
    for i in 0..TOTAL_NUM {
        file.write_u16::<NativeEndian>(v[i].md5).unwrap();
        file.write_u16::<NativeEndian>(v[i].mobile_index).unwrap();
        let done = (i + 1) * 100 / TOTAL_NUM;
        if done > percent {
            percent = done;
            term.clear_line().unwrap();
            term.write_str(&format!("Writing formula to file ...{}%", percent))
                .unwrap();
        }
    }
    println!("\n# Write formula costed {:?}", start.elapsed());
}

fn read_formula_from_file(v_mutex: &Arc<Mutex<Vec<Pair>>>) {
    let start = Instant::now();
    let v_clone = v_mutex.clone();
    let mut v = v_clone.lock().unwrap();
    let term = Term::stdout();
    let mut percent = 0;
    term.write_str(&format!("Reading formula from file ...{}%", percent))
        .unwrap();

    let mut file = buffered_reader::File::open("formula.dat").unwrap();
    for i in 0..TOTAL_NUM {
        v[i].md5 = file.read_u16::<NativeEndian>().unwrap();
        v[i].mobile_index = file.read_u16::<NativeEndian>().unwrap();
        let done = (i + 1) * 100 / TOTAL_NUM;
        if done > percent {
            percent = done;
            term.clear_line().unwrap();
            term.write_str(&format!("Reading formula from file ...{}%", percent))
                .unwrap();
        }
    }
    println!("\n# Read formula costed {:?}", start.elapsed());
}

fn decode_file(v_mutex: &Arc<Mutex<Vec<Pair>>>, filename: &String) {
    match std::fs::read_to_string(filename) {
        Err(_) => {println!("couldn't open {}", filename); return;},
        Ok(buf) => {
            let start = Instant::now();

            let v: Vec<&str> = buf.split(',').collect();
            let mut v_hash: Vec<&str> = vec![];
            for s in v {
                let s = s.trim();
                if s.len() > 0 { v_hash.push(s); }
            }

            let mut file = File::create(format!("{}.out",filename)).unwrap();
            let term = Term::stdout();
            let len = v_hash.len();
            for i in 0..len {
                let mobile = look_up(v_mutex, v_hash[i]);
                file.write(format!("{},{}\n", v_hash[i], mobile).as_ref()).unwrap();
                term.clear_line().unwrap();
                term.write_str(&format!("Decoding ...{}/{}", i+1, len)).unwrap();
            }

            println!("Decode completed. Please find the results in {}.out", filename);
            println!("\n# Decode file costed {:?}", start.elapsed());
        },
    }
}

fn test(v_mutex: &Arc<Mutex<Vec<Pair>>>) {
    println!("run test ...");
    let n = 10;
    use rand::{thread_rng, Rng};
    let mut rng = thread_rng();
    let start = Instant::now();

    for _ in 0..n {
        let start = Instant::now();
        let prefix = rng.gen_range(0, PREFIX_LEN);
        let mobile = PREFIX_LIST[prefix] * MOBILE_SPAN_LEN + rng.gen_range(0, MOBILE_SPAN_LEN);
        let digest = format!("{:?}", md5::compute(mobile.to_string()));
        let result = look_up(&v_mutex, &digest);
        println!("{},{},{},{:?}", mobile, digest, result, start.elapsed());
    }

    let costs = start.elapsed().as_millis() / n;
    let costs = costs as f32 / 1000.;
    println!("average costs {}s", costs);
}
