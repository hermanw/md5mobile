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
const PREFIX_LIST: [usize; 46] = [
186, 134, 135, 159,
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
const MOBILE_SPAN_LEN: usize = 100000000;
const HS_BITS: usize = 5;
const MB_BITS: usize = 4;

// the data structure of the formula
struct Pair {
    hash: [u8; HS_BITS],
    mobile_index: [u8; MB_BITS],
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
    let mut v_hash: Vec<(String, usize)> = vec![];
    match std::fs::read_to_string(&filename) {
        Err(_) => {
            println!("couldn't open {}", filename);
        }
        Ok(buf) => {
            let v: Vec<&str> = buf.split(',').collect();
            for s in v {
                let s = s.trim();
                if s.len() > 0 {
                    v_hash.push((s.to_string(),0));
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

    // alloc memory
    let slice_len = MOBILE_SPAN_LEN / thread_num;
    let v_hash_mutex = Arc::new(Mutex::new(v_hash));
    let finished: usize = 0;
    let finished_mutex = Arc::new(Mutex::new(finished));

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
            let mut v = alloc_big_vec(slice_len);
            let mut is_finished = false;
            for prefix in 0..PREFIX_LIST.len() {
                // comput md5
                let mut percent = 0;
                for i in 0..slice_len {
                    let mobile = PREFIX_LIST[prefix] * MOBILE_SPAN_LEN + threadid*slice_len + i;
                    let hash = md5::compute(mobile.to_string());
                    for j in 0..HS_BITS {v[i].hash[j] = hash[j]}
                    for j in 0..MB_BITS {v[i].mobile_index[j] = ((i >> (j*8)) % 256) as u8}
                    let done = (i + 1) * 100 / slice_len;
                    if done > percent {
                        percent = done;
                        tx_clone.send((threadid, prefix, percent, 0)).unwrap();
                        is_finished = *finished_clone.lock().unwrap() == v_hash_len;
                        if is_finished {break;}
                    }
                }
                if is_finished {break;}
                // sort
                v.sort_unstable_by(|a, b| a.hash.cmp(&b.hash));
                // look up
                for i in 0..v_hash_len {
                    let mut hash: [u8; HS_BITS] = [0; HS_BITS];
                    {
                        // check if already decoded
                        let v_hash = v_hash_clone.lock().unwrap();
                        let finished = finished_clone.lock().unwrap();
                        if *finished == v_hash_len {break;}
                        if v_hash[i].1 != 0 { continue;}

                        // still some hash not decoded
                        let hash_bytes = hex::decode(&v_hash[i].0).unwrap();
                        for j in 0..HS_BITS {
                            hash[j] = hash_bytes[j];
                        }
                    }

                    let result = v.binary_search_by(|probe| probe.hash.cmp(&hash));
                    match result {
                        Ok(index) => {
                            // verify and find the corect match
                            let mut i_result = index;
                            while v[i_result].hash == hash {
                                let mut mobile = PREFIX_LIST[prefix] * MOBILE_SPAN_LEN + threadid*slice_len;
                                for k in 0..MB_BITS {
                                    mobile += (v[i_result].mobile_index[k] as usize) << k*8;
                                }
                                
                                let hash_i = md5::compute(mobile.to_string());
                                let mut v_hash = v_hash_clone.lock().unwrap();
                                if format!("{:?}", hash_i) == v_hash[i].0 {
                                    let mut finished = finished_clone.lock().unwrap();
                                    v_hash[i].1 = mobile;
                                    *finished += 1;
                                    tx_clone.send((threadid, prefix, 101, *finished)).unwrap();
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
                if *finished_clone.lock().unwrap() == v_hash_len {break;} // don't have to look into other prefixes
            }            
        });
    }
    drop(tx);

    // recieve progress from threads
    let term = Term::stdout();
    let mut hash_finished = 0;
    for (threadid, prefix, percent, finished) in rx {
        // print progress
        let mut thread_str = format!(
            "{}: Thread {} building formula... {}% completed. @{:?}",
            PREFIX_LIST[prefix],
            threadid,
            percent,
            start.elapsed()
        );
        if percent >= 100 {
            thread_str = format!("{}: Thread {} looking up...", PREFIX_LIST[prefix], threadid);
            if percent > 100 { hash_finished = finished; }
        }
        term.move_cursor_up(thread_num - threadid).unwrap();
        term.clear_line().unwrap();
        term.write_str(&thread_str).unwrap();
        term.move_cursor_down(thread_num - threadid).unwrap();
        term.clear_line().unwrap();
        term.write_str(&format!("{}/{} decoded", hash_finished, v_hash_len)).unwrap();
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
