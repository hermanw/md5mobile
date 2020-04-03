use console::Term;
use std::env;
use std::fs::File;
use std::io::Write;
use std::thread;
use std::time::Instant;
use std::sync::{mpsc, Arc, Mutex};

// the sequnce of the prefixes is optimized by putting the most frequent ones to the front
// [('186', 716495), ('158', 710534), ('135', 695733), ('159', 695146),
//  ('136', 679350), ('150', 664271), ('137', 660530), ('138', 645891),
//  ('187', 635777), ('151', 628992), ('182', 617683), ('152', 617580),
//  ('139', 616160), ('183', 548983), ('188', 534265), ('134', 410070),
//  ('185', 326902), ('189', 304128), ('180', 294937), ('157', 284990),
//  ('155', 277126), ('156', 265458), ('131', 261437), ('132', 259597),
//  ('133', 255571), ('130', 250038), ('181', 249092), ('176', 242827),
//  ('177', 228022), ('153', 215892), ('184', 78492), ('178', 76291),
//  ('173', 72096), ('147', 48942), ('175', 38738), ('199', 16200),
//  ('166', 14720), ('170', 4409), ('198', 4110), ('171', 1438),
//  ('191', 145), ('145', 40), ('165', 2), ('172', 1),
//  ('154', 1), ('146', 1)]
const PREFIX_LIST: [u64; 46] = [
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
191, 145, 165, 172, 
154, 146
];
const MOBILE_SPAN_LEN: u64 = 100000000;
const SLICE_NUM: usize = 10;    // each thread divides its span into slices

// the data structure of the formula
struct Pair {
    hash: u32,
    mobile_index: u32,
}

fn main() {
    // parse file name
    let filename: String;
    let args: Vec<String> = env::args().collect();
    if args.len() == 1 {
        println!("missing file name");
        return;
    } else {
        filename = args[1].to_string();
    }

    // process the file and get a md5 hash list
    print!("Loading file {}... ", filename);
    let mut v_hash: Vec<(String, u64, u32)> = vec![];   // (hash,mobile,hash_bytes_prefix)
    match std::fs::read_to_string(&filename) {
        Err(_) => {
            println!("couldn't open {}", filename);
        }
        Ok(buf) => {
            let v: Vec<&str> = buf.split(',').collect();
            for s in v {
                let s = s.trim();
                if s.len() > 0 {
                    let hash = s.to_string();
                    let hash_bytes = hex::decode(s.to_string()).unwrap();
                    unsafe {
                        let hash_bytes_prefix = *(hash_bytes.as_ptr() as *const u32);
                        v_hash.push((hash,0,hash_bytes_prefix));
                    }
                }
            }
        }
    }    
    let v_hash_len = v_hash.len();
    if v_hash_len == 0 {
        println!("no valid hash");
        return; 
    }
    println!("OK");
    println!("Total {} hashes.", v_hash_len);

    println!("Start to decode...");
    let start = Instant::now();
    let thread_num = num_cpus::get();

    // set up mutex
    let v_hash_mutex = Arc::new(Mutex::new(v_hash));
    let finished: usize = 0;
    let finished_mutex = Arc::new(Mutex::new(finished));
    let slice_len = MOBILE_SPAN_LEN as usize / thread_num / SLICE_NUM;

    //create threads
    let (tx, rx) = mpsc::channel();
    for threadid in 0..thread_num {
        println!("Thread {} starting...", threadid);
        let tx_clone = mpsc::Sender::clone(&tx);
        let v_hash_clone = v_hash_mutex.clone();
        let finished_clone = finished_mutex.clone();
        thread::spawn(move || {
            if threadid > 0 {
                // allow some interval to avoid conflict in accessing mutex
                thread::sleep(std::time::Duration::from_millis(200*threadid as u64));
            }
            // alloc memory
            let mut v = alloc_big_vec(slice_len);
            // start to work on each prefix
            for prefix in 0..PREFIX_LIST.len() {
                let prefix_num = PREFIX_LIST[prefix] * MOBILE_SPAN_LEN;
                // work on each slice
                for slice in 0..SLICE_NUM {
                    // step 1: compute md5
                    for i in 0..slice_len {
                        let mobile = (threadid * SLICE_NUM + slice) * slice_len + i;
                        let mobile = prefix_num + mobile as u64;
                        let hash = md5::compute(mobile.to_string());
                        unsafe {
                            v[i].hash = *(hash.as_ptr() as *const u32);
                        }
                        v[i].mobile_index = i as u32;
                    }
                    // step 2: sort
                    v.sort_unstable_by(|a, b| a.hash.cmp(&b.hash));
                    // step 3: look up all hashes
                    for i in 0..v_hash_len {
                        #[allow(unused_assignments)]
                        let mut hash: u32 = 0;
                        { // important trick for thread locks
                            // check if already decoded
                            let v_hash = v_hash_clone.lock().unwrap();
                            let finished = finished_clone.lock().unwrap();
                            if *finished == v_hash_len {break;}
                            if v_hash[i].1 != 0 { continue;}

                            // still some hash not decoded
                            hash = v_hash[i].2;
                        }

                        let result = v.binary_search_by(|probe| probe.hash.cmp(&hash));
                        match result {
                            Ok(index) => {
                                // verify and find the corect match
                                let mut i_result = index;
                                while v[i_result].hash == hash {
                                    let mobile = (threadid * SLICE_NUM + slice) * slice_len
                                        + v[i_result].mobile_index as usize;
                                    let mobile = prefix_num + mobile as u64;
                                
                                    let hash_i = md5::compute(mobile.to_string());
                                    let mut v_hash = v_hash_clone.lock().unwrap();
                                    if format!("{:?}", hash_i) == v_hash[i].0 {
                                        let mut finished = finished_clone.lock().unwrap();
                                        v_hash[i].1 = mobile;
                                        *finished += 1;
                                        tx_clone.send((threadid, prefix, slice, *finished)).unwrap();
                                        break;
                                    }
                                    if i_result > 0 {
                                        i_result -= 1;
                                    } else {
                                        break;
                                    }                                
                                }
                            }
                            Err(_) => {
                                // println!("{},{}",hash,0);
                            }
                        }
                    }
                    // send progress                    
                    tx_clone.send((threadid, prefix, slice, 0)).unwrap();
                    if *finished_clone.lock().unwrap() == v_hash_len {break;} // check if work done
                } 
                if *finished_clone.lock().unwrap() == v_hash_len {break;} // check if work done
            }
        });
    }
    drop(tx);

    // recieve progress from threads
    let term = Term::stdout();
    let mut hash_finished = 0;
    for (threadid, prefix, slice, finished) in rx {
        // print progress
        let thread_str = format!(
            "{}: Thread {} working... {}%",
            PREFIX_LIST[prefix],
            threadid,
            (slice+1)*100/SLICE_NUM
        );
        if finished > 0 {hash_finished = finished;}
        term.move_cursor_up(thread_num - threadid).unwrap();
        term.clear_line().unwrap();
        term.write_str(&thread_str).unwrap();
        term.move_cursor_down(thread_num - threadid).unwrap();
        term.clear_line().unwrap();
        term.write_str(&format!("{}/{} decoded @{:?}", hash_finished, v_hash_len, start.elapsed())).unwrap();
    }
    for i in 0..thread_num {
        term.move_cursor_up(thread_num - i).unwrap();
        term.clear_line().unwrap();
        term.write_str(&format!("Thread {} stoped", i)).unwrap();
        term.move_cursor_down(thread_num - i).unwrap();
    }
    println!("\nCompleted @{:?}", start.elapsed());

    // write result to file
    let mut file = File::create(format!("{}.out",filename)).unwrap();
    let v_hash_clone = v_hash_mutex.clone();
    let v_hash = v_hash_clone.lock().unwrap();
    for i in 0..v_hash_len {
        file.write(format!("{},{}\n", v_hash[i].0, v_hash[i].1).as_ref()).unwrap();
    }

    println!("Decoded results are in {}.out", filename);
}

fn alloc_big_vec(len: usize) -> Vec<Pair> {
    let mut v: Vec<Pair> = Vec::with_capacity(len);
    unsafe {
        v.set_len(len);
    }
    v
}
