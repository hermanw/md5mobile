use console::Term;
use std::env;
use std::fs::File;
use std::io::Write;
use std::thread;
use std::time::Instant;
use std::sync::{mpsc, Arc, Mutex};
use md5::{Md5, Digest};

mod mca;

// the data structure of the formula
struct Pair {
    hash: u32,
    id: u64
}

const DAYS: [usize; 12] = [31,29,31,30,31,30,31,31,30,31,30,31];
const SEQ_SIZE: usize = 1000;
const AREA_BATCH_SIZE : usize = 20;
const SLICE_SIZE: usize = 366 * SEQ_SIZE * AREA_BATCH_SIZE;
const COE: [usize; 17] =[7,9,10,5,8,4,2,1,6,3,7,9,10,5,8,4,2];
const C_SUM: [&str; 11] = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "X"];

fn main() {
    // parse args
    let mut from_year = 1970;
    let mut to_year = 2000;
    let mut v_hash: Vec<(String, u64, u32)> = vec![]; // (hash,id,hash_bytes_prefix)
    if !parse_args(&mut v_hash, &mut from_year, &mut to_year) {return;}
    let v_hash_len = v_hash.len();

    // start
    println!("Start to decode... [from year {} to {}]", from_year, to_year);
    let start = Instant::now();
    // set up mutex
    let v_hash_mutex = Arc::new(Mutex::new(v_hash));
    let finished: usize = 0;
    let finished_mutex = Arc::new(Mutex::new(finished));

    //create threads
    let thread_num = num_cpus::get();
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
            // generate helper data structures
            let v_years = generate_v_years(&from_year, &to_year);
            let v_strings = generate_v_strings(&from_year, &to_year);
            let v_sums = generate_v_sums();
            // alloc memory
            let mut v: Vec<Pair> = Vec::with_capacity(SLICE_SIZE);
            unsafe {
                v.set_len(SLICE_SIZE);
            }
            // each year
            for year in v_years {
                // each assigned area
                let mut i_area = threadid * AREA_BATCH_SIZE;
                while i_area < mca::MCA_SIZE {
                    thread_work(threadid,&mut v,&v_strings,&v_sums,year,i_area,&from_year,&v_hash_clone, v_hash_len, &finished_clone, &tx_clone);
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
    } // end create threads
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

fn parse_args(v_hash: &mut Vec<(String, u64, u32)>, from_year: &mut usize, to_year: &mut usize) -> bool {
    // parse file name
    let filename: String;
    let args: Vec<String> = env::args().collect();
    if args.len() == 1 {
        println!("usage: mi hash_filename [from_year] [to_year]");
        return false;
    } else {
        filename = args[1].to_string();
        if args.len() > 3 {
            *from_year = args[2].parse::<usize>().unwrap();
            *to_year = args[3].parse::<usize>().unwrap();
        }
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

fn generate_ids(v:&mut Vec<Pair>, v_strings:&Vec<Vec<String>>, v_sums:&Vec<Vec<usize>>, year:usize, i_area:usize,from_year:&usize) {
    let mut i: usize = 0;
    let mut id_string = String::from("123456789012345678");
    let mut md5 = Md5::new();
    for offset in 0..AREA_BATCH_SIZE {
        let current_area = i_area + offset;
        if current_area >= mca::MCA_SIZE {
            break;
        }
        let area = mca::MCA[current_area] as u64;
        let area_year = area * 100000000000 + year as u64 * 10000000;
        let mut sum_area_year = 0;
        let mut t = mca::MCA[current_area];
        for i in 0..6 {
            sum_area_year += (t % 10) * COE[5-i];
            t = t / 10;
        }
        let mut t = year;
        for i in 0..4 {
            sum_area_year += (t % 10) * COE[9-i];
            t = t / 10;
        }
        id_string.replace_range(0..6, &v_strings[0][current_area]);
        id_string.replace_range(6..10, &v_strings[1][year-from_year]);
        let mut area_year_month = area_year;
        for month in 0..DAYS.len() {
            area_year_month += 100000;
            let sum_area_year_month = sum_area_year + v_sums[0][month];
            id_string.replace_range(10..12, &v_strings[2][month+1]);
            let mut area_year_month_day = area_year_month;
            for day in 0..DAYS[month] {
                area_year_month_day += 1000;
                let sum_area_year_month_day = sum_area_year_month + v_sums[1][day];
                id_string.replace_range(12..14, &v_strings[2][day+1]);
                for seq in 0..SEQ_SIZE { 
                    let id = area_year_month_day + seq as u64;
                    id_string.replace_range(14..17, &v_strings[3][seq]);
                    let sum = sum_area_year_month_day + v_sums[2][seq];
                    let mut r = sum % 11;
                    r = 12 - r;
                    if r > 10 {
                        r = r - 11;
                    }
                    id_string.replace_range(17..18, C_SUM[r]);
                    // if i<100 { println!("{},{}", id,id_string);}
                    md5.input(&id_string);
                    let hash = md5.result_reset();
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

fn generate_v_years(from_year: &usize, to_year: &usize) -> Vec<usize> {
    let from_year = *from_year;
    let to_year = *to_year;
    let mut v_years = Vec::with_capacity(to_year-from_year+1);
    let mut first = 1990;
    if first > to_year {first = to_year;}
    if first < from_year {first = from_year;}
    v_years.push(first);
    let mut delta = 1;
    while first-delta >= from_year || first+delta <= to_year {
        if first-delta >= from_year {
            v_years.push(first-delta);
        }
        if first+delta <= to_year {
            v_years.push(first+delta);
        }
        delta+=1;
    }
    v_years
}

fn generate_v_strings(from_year: &usize, to_year: &usize) -> Vec<Vec<String>> {
    let mut v_area = Vec::with_capacity(mca::MCA_SIZE);
    for area in mca::MCA.iter() {
        v_area.push(area.to_string());
    }
    let mut v_year = Vec::with_capacity(*to_year-*from_year+1);
    for year in *from_year..*to_year+1 {
        v_year.push(year.to_string());
    }
    let mut v_100 = Vec::with_capacity(100);
    for i in 0..100 {
        v_100.push(format!("{:02}",i));
    }
    let mut v_1000 = Vec::with_capacity(1000);
    for i in 0..1000 {
        v_1000.push(format!("{:03}",i));
    }
    let mut v = Vec::with_capacity(4);
    v.push(v_area);
    v.push(v_year);
    v.push(v_100);
    v.push(v_1000);
    v
}

fn generate_v_sums() -> Vec<Vec<usize>> {
    let mut v_month = Vec::with_capacity(12);
    for i in 0..12 {
        let mut sum = ((i+1) / 10) * COE[10];
        sum += ((i+1) % 10) * COE[11];
        v_month.push(sum);
    }
    let mut v_day = Vec::with_capacity(31);
    for i in 0..31 {
        let mut sum = ((i+1) / 10) * COE[12];
        sum += ((i+1) % 10) * COE[13];
        v_day.push(sum);
    }
    let mut v_seq = Vec::with_capacity(1000);
    for i in 0..1000 {
        let mut sum = (i / 100) * COE[14];
        sum += ((i%100) / 10) * COE[15];
        sum += (i%10) * COE[16];
        v_seq.push(sum);
    }
    let mut v = Vec::with_capacity(3);
    v.push(v_month);
    v.push(v_day);
    v.push(v_seq);
    v
}

fn generate_id_string(id: u64) -> String {
    let id_string = id.to_string();
    let mut sum = 0;
    let bytes = id_string.as_bytes();
    for i in 0..bytes.len() {
        let digit = (bytes[i] - '0' as u8) as usize;
        sum += digit*COE[i];
    }
    let mut r = sum % 11;
    r = 12 - r;
    if r > 10 {
        r = r - 11;
    }
    id_string + C_SUM[r]
}

fn thread_work(threadid:usize,
    v:&mut Vec<Pair>,
    v_strings:&Vec<Vec<String>>,
    v_sums:&Vec<Vec<usize>>,
    year:usize,
    i_area:usize,
    from_year:&usize,
    v_hash_clone:&Arc<Mutex<Vec<(String, u64, u32)>>>, 
    v_hash_len:usize, 
    finished_clone:&Arc<Mutex<usize>>, 
    tx_clone:&mpsc::Sender<(usize,usize,usize,usize)>) {

    // step 1: generate id list & compute md5
    generate_ids(v, &v_strings, &v_sums, year, i_area, from_year);
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
                    let mut md5 = Md5::new();
                    md5.input(&id_string);
                    let hash_i = md5.result();
                    let mut v_hash = v_hash_clone.lock().unwrap();
                    if hex::encode(hash_i) == v_hash[i].0 {
                        let mut finished = finished_clone.lock().unwrap();
                        v_hash[i].1 = id;
                        *finished += 1;
                        tx_clone
                            .send((threadid, year, i_area*100/mca::MCA_SIZE, *finished))
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
    tx_clone.send((threadid, year, i_area*100/mca::MCA_SIZE, 0)).unwrap();
}