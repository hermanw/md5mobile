use std::os::raw::c_char;
use std::ffi::CString;

extern {
    fn sendProgress(i:usize, hash: *const c_char, mobile: *const c_char);
    #[allow(dead_code)]
    fn printToJS(log: *const c_char);
}

pub fn send_progress(i:usize, hash:&String, mobile: u64) {
    let h = CString::new(hash.to_string()).unwrap().into_raw();
    let m = CString::new(mobile.to_string()).unwrap().into_raw();
    unsafe{
        sendProgress(i, h, m);
    }
}

#[allow(unused_variables)]
pub fn print_jslog(log: String) {
    // let s = CString::new(log).unwrap().into_raw();
    // unsafe{
    //     printToJS(s);
    // }
}

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

pub fn decode(mut v_hash: Vec<(String, u64, u32)>, thread_num: usize, threadid: usize) -> Vec<(String, u64, u32)> {
    let v_hash_len = v_hash.len();
    if v_hash_len == 0 {
        // TODO
        return v_hash; 
    }
    print_jslog(format!("{},{},{}",v_hash_len,thread_num,threadid));
    let mut finished: usize = 0;
    // alloc memory
    let slice_len = MOBILE_SPAN_LEN as usize / thread_num / SLICE_NUM;
    let mut v: Vec<Pair> = Vec::with_capacity(slice_len);
    unsafe {
        v.set_len(slice_len);
    }
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
                // print_jslog(format!("{},{},{}",mobile,v[i].hash,v[i].mobile_index));
            }
            // step 2: sort
            v.sort_unstable_by(|a, b| a.hash.cmp(&b.hash));
            // step 3: look up all hashes
            for i in 0..v_hash_len {
                #[allow(unused_assignments)]
                let mut hash: u32 = 0;
                { // important trick for thread locks
                    // check if already decoded
                    if finished == v_hash_len {break;}
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
                            if format!("{:?}", hash_i) == v_hash[i].0 {
                                v_hash[i].1 = mobile;
                                finished += 1;
                                // send progress
                                print_jslog(format!("{},{}",v_hash[i].0,mobile));
                                send_progress(i, &v_hash[i].0, mobile);
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
            if finished == v_hash_len {break;} // check if work done
        } 
        if finished == v_hash_len {break;} // check if work done
    }
    v_hash
}

