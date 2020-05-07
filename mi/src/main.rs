use console::Term;
use std::env;
use std::fs::File;
use std::io::Write;
use std::thread;
use std::time::Instant;
use std::sync::{mpsc, Arc, Mutex};

mod mca;
mod year;

// the data structure of the formula
struct Pair {
    hash: u32,
    id: u64
}

const DAYS: [usize; 12] = [31,29,31,30,31,30,31,31,30,31,30,31];
const SEQ_SIZE: usize = 1000;
const AREA_BATCH_SIZE : usize = 10;
const SLICE_SIZE: usize = 366 * SEQ_SIZE * AREA_BATCH_SIZE;
const COE: [usize; 17] =[7,9,10,5,8,4,2,1,6,3,7,9,10,5,8,4,2];

fn main() {
    let mut v_hash: Vec<(String, u64, u32)> = vec![]; // (hash,id,hash_bytes_prefix)
    if !parse_file(&mut v_hash) {return;}

    println!("Start to decode...");
    let start = Instant::now();
    let thread_num = num_cpus::get();
    let v_hash_len = v_hash.len();

    // set up mutex
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
                thread::sleep(std::time::Duration::from_millis(200 * threadid as u64));
            }
            // alloc memory
            let mut v: Vec<Pair> = Vec::with_capacity(SLICE_SIZE);
            unsafe {
                v.set_len(SLICE_SIZE);
            }
            // each year
            for year in &year::YEAR {
                let areas = mca::get_mca(*year);
                // each assigned area
                let mut i_area = threadid * AREA_BATCH_SIZE;
                while i_area < areas.len() {
                    thread_work(threadid,&mut v,*year,&areas,i_area, &v_hash_clone, v_hash_len, &finished_clone, &tx_clone);
                    i_area += thread_num * AREA_BATCH_SIZE;
                    if *finished_clone.lock().unwrap() == v_hash_len {
                        break;
                    } // check if work done
                }
                if *finished_clone.lock().unwrap() == v_hash_len {
                    break;
                } // check if work done
            }
        });
    } // end creat threads
    drop(tx);

    // recieve progress from threads
    let term = Term::stdout();
    let mut hash_finished = 0;
    for (threadid, year, percent, finished) in rx {
        // print progress
        let thread_str = format!(
            "Thread {} working... {} {}%",
            threadid,year,percent
        );
        if finished > 0 {hash_finished = finished;}
        term.move_cursor_up(thread_num - threadid).unwrap();
        term.clear_line().unwrap();
        term.write_str(&thread_str).unwrap();
        term.move_cursor_down(thread_num - threadid).unwrap();
        term.clear_line().unwrap();
        term.write_str(&format!("{}/{} decoded @{}", hash_finished, v_hash_len, start.elapsed().as_secs())).unwrap();
    }
    for i in 0..thread_num {
        term.move_cursor_up(thread_num - i).unwrap();
        term.clear_line().unwrap();
        term.write_str(&format!("Thread {} stoped", i)).unwrap();
        term.move_cursor_down(thread_num - i).unwrap();
    }
    println!("\nCompleted @{}", start.elapsed().as_secs());

    // write result to file
    let args: Vec<String> = env::args().collect();
    let filename = args[1].to_string();
    let mut file = File::create(format!("{}.out",filename)).unwrap();
    let v_hash_clone = v_hash_mutex.clone();
    let v_hash = v_hash_clone.lock().unwrap();
    for i in 0..v_hash_len {
        let id_string = generate_id_string(v_hash[i].1);
        file.write(format!("{},{}\n", v_hash[i].0, id_string).as_ref()).unwrap();
    }

    println!("Decoded results are in {}.out", filename);

}

fn parse_file(v_hash: &mut Vec<(String, u64, u32)>) -> bool {
    // parse file name
    let filename: String;
    let args: Vec<String> = env::args().collect();
    if args.len() == 1 {
        println!("missing file name");
        return false;
    } else {
        filename = args[1].to_string();
    }

    // process the file and get a md5 hash list
    print!("Loading file {}... ", filename);
    match std::fs::read_to_string(&filename) {
        Err(_) => {
            println!("couldn't open {}", filename);
        }
        Ok(buf) => {
            let v: Vec<&str> = buf.split(',').collect();
            for s in v {
                let s = s.trim().to_lowercase();
                if s.len() > 0 {
                    let hash = s.to_string();
                    let hash_bytes = hex::decode(s.to_string()).unwrap();
                    unsafe {
                        let hash_bytes_prefix = *(hash_bytes.as_ptr() as *const u32);
                        v_hash.push((hash, 0, hash_bytes_prefix));
                    }
                }
            }
        }
    }
    let v_hash_len = v_hash.len();
    if v_hash_len == 0 {
        println!("no valid hash");
        return false;
    }
    println!("OK");
    println!("Total {} hashes.", v_hash_len);
    true
}

fn generate_ids(v:&mut Vec<Pair>, year:usize, areas:&Vec<usize>, i_area:usize) {
    let mut i: usize = 0;
    for offset in 0..AREA_BATCH_SIZE {
        let current_area = i_area + offset;
        if current_area >= areas.len() {
            break;
        }
        let area = areas[current_area];
        for month in 0..DAYS.len() {
            for day in 0..DAYS[month] {
                for seq in 0..SEQ_SIZE { 
                    let id = generate_id(area,year,month,day,seq);
                    let id_string = generate_id_string(id);
                    // println!("{},{},{}", i,id,id_string);
                    let hash = md5::compute(id_string);
                    unsafe {
                        v[i].hash = *(hash.as_ptr() as *const u32);
                    }
                    v[i].id = id;
                    i += 1;
                }
            }
        }
    }
}

fn generate_id(area:usize,year:usize,month:usize,day:usize,seq:usize) -> u64{
    let mut id = area as u64;
    id = id * 10000 + year as u64;
    id = id * 100 + month as u64 + 1;
    id = id * 100 + day as u64 + 1;
    id = id * 1000 + seq as u64 + 1;
    id
}

fn generate_id_string(id: u64) -> String {
    let mut id_string = id.to_string();
    let mut sum = 0;
    for i in 0..id_string.len() {
        let digit = id_string[i..i+1].parse::<usize>().unwrap();
        sum += digit*COE[i];
    }
    let mut r = sum % 11;
    r = 12 - r;
    if r > 10 {
        r = r - 11;
    }
    if r == 10 {
        id_string += "X";
    } else {
        id_string += &r.to_string();
    }
    id_string
}

fn thread_work(threadid:usize, v:&mut Vec<Pair>, year:usize, areas:&Vec<usize>, i_area:usize, 
    v_hash_clone:&Arc<Mutex<Vec<(String, u64, u32)>>>, 
    v_hash_len:usize, 
    finished_clone:&Arc<Mutex<usize>>, 
    tx_clone:&mpsc::Sender<(usize,usize,usize,usize)>) {

    // step 1: generate id list & compute md5
    generate_ids(v, year, &areas, i_area);
    // step 2: sort
    v.sort_unstable_by(|a, b| a.hash.cmp(&b.hash));
    // step 3: look up all hashes
    for i in 0..v_hash_len {
        #[allow(unused_assignments)]
        let mut hash: u32 = 0;
        {
            // important trick for thread locks
            // check if already decoded
            let v_hash = v_hash_clone.lock().unwrap();
            let finished = finished_clone.lock().unwrap();
            if *finished == v_hash_len {
                break;
            }
            if v_hash[i].1 != 0 {
                continue;
            }

            // still some hash not decoded
            hash = v_hash[i].2;
        }

        let result = v.binary_search_by(|probe| probe.hash.cmp(&hash));
        match result {
            Ok(index) => {
                // verify and find the corect match
                let mut i_result = index;
                while v[i_result].hash == hash {
                    let id = v[i_result].id;
                    let id_string = generate_id_string(id);
                    let hash_i = md5::compute(id_string);
                    let mut v_hash = v_hash_clone.lock().unwrap();
                    if format!("{:?}", hash_i) == v_hash[i].0 {
                        let mut finished = finished_clone.lock().unwrap();
                        v_hash[i].1 = id;
                        *finished += 1;
                        tx_clone
                            .send((threadid, year, i_area*100/areas.len(), *finished))
                            .unwrap();
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
    // step 4: send progress
    tx_clone.send((threadid, year, i_area*100/areas.len(), 0)).unwrap();
}