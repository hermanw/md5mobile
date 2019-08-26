use std::sync::mpsc;
use std::thread;
use std::time::Instant;

const THREAD_NUM: usize = 4;
const PREFIX_LIST: [usize; 1] = [138];
const MOBILE_SPAN_LEN: usize = 100000000;

struct Pair {
    md5: u16,
    mobile: u16,
}

fn main() {
    //
    // build the formula
    //
    let start = Instant::now();

    let (tx, rx) = mpsc::channel();

    let total_num = PREFIX_LIST.len() * MOBILE_SPAN_LEN;
    let slice_len = u16::max_value() as usize + 1;
    let slice_num = total_num / slice_len + 1;

    let mut thread_slice_array = [(0, 0); THREAD_NUM]; // each thread's slice (start, length)
    for threadid in 0..THREAD_NUM {
        // assign slices to threads
        let mut thread_slice_num = slice_num / THREAD_NUM;
        if threadid < slice_num % THREAD_NUM {
            thread_slice_num += 1;
        }
        let mut slice_start = 0;
        if threadid > 0 {
            slice_start = thread_slice_array[threadid - 1].0 + thread_slice_array[threadid - 1].1;
        }
        thread_slice_array[threadid] = (slice_start, thread_slice_num);
        // println!("{:?}", thread_slice_array[threadid]);

        //create threads
        let c_tx = mpsc::Sender::clone(&tx);
        thread::spawn(move || {
            println!("Thread {} start...", threadid);
            for thread_slice in 0..thread_slice_array[threadid].1 {
                let slice = thread_slice_array[threadid].0 + thread_slice;
                for index in 0..slice_len {
                    let address = slice * slice_len + index;
                    if address < total_num {
                        let mobile = address_to_mobile(address);
                        let m_md5 = mobile_to_md5(mobile);
                        // println!("{},{},{},{}", threadid, address, mobile, m_md5);
                        c_tx.send((address, index as u16, m_md5)).unwrap();
                    }
                }
                // send total_num as address to indicate the end of a slice
                c_tx.send((total_num, slice as u16, 0)).unwrap();
                // println!("Thread {} slice {} completed.", threadid, slice);
            }
            println!(
                "# Thread {} completed after {:?}.",
                threadid,
                start.elapsed()
            );
        });
    }
    drop(tx);

    // alloc memory
    let mut v = alloc_big_vec(total_num);

    // recieve results from threads
    let mut count = 0;
    for (address, index, m_md5) in rx {
        if address < total_num {
            count += 1;
            v[address].md5 = m_md5;
            v[address].mobile = index;
        } else {
            // println!("slice {} completed.", index);
            // sort each slice
            let slice_start = index as usize * slice_len;
            let mut slice_end = slice_start + slice_len;
            if slice_end > total_num {
                slice_end = total_num;
            }
            let v_slice = &mut v[slice_start..slice_end];
            v_slice.sort_by(|a, b| a.md5.cmp(&b.md5));
        }
    }
    println!("{}", count);

    println!("# Main thread formula completed after {:?}", start.elapsed());

    //
    // look up
    //
    let start = Instant::now();

    let digest = md5::compute("13856004456".to_string());
    let m_md5 = digest_to_md5(digest);
    for slice in 0..slice_num {
        // look up in each slice
        let slice_start = slice * slice_len;
        let mut slice_end = slice_start + slice_len;
        if slice_end > total_num {
            slice_end = total_num;
        }
        let v_slice = &mut v[slice_start..slice_end];

        // for i in 0..v_slice.len() {
        //     if v_slice[i].md5 == m_md5 {
        //         println!("{},{},{}", slice,v_slice[i].mobile,restore_mobile(v_slice[i].mobile,slice,slice_len));
        //     }
        // }
        let result = v_slice.binary_search_by(|probe| probe.md5.cmp(&m_md5));
        match result {
            Ok(index) => {
                let mut i = index;
                while i > 0 && v_slice[i].md5 == m_md5 {
                    let mobile_index = v_slice[i].mobile;
                    let mobile = restore_mobile(mobile_index,slice,slice_len);
                    println!("{},{},{}", slice, mobile_index, mobile);
                    if md5::compute(mobile.to_string()) == digest {
                        println!("## found {}", mobile);
                    }
                    i -= 1;
                }
                },
            Err(_) => println!("{} not found", slice)
        }
    }

    println!("Look up costs {:?}", start.elapsed());
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

fn restore_mobile(mobile: u16, slice: usize, slice_len: usize) -> u64 {
    let address = slice * slice_len + mobile as usize;
    address_to_mobile(address)
}