use buffered_reader;
use byteorder::{NativeEndian, ReadBytesExt, WriteBytesExt};
use std::fs::File;
use std::sync::mpsc;
use std::thread;
use std::time::Instant;
use std::io::BufWriter;

// some const
const THREAD_NUM: usize = 4;
const PREFIX_LEN: usize = 42;
const PREFIX_LIST: [usize; PREFIX_LEN] = [134, 135, 136, 137, 138, 139, 147, 150, 151, 152, 157, 158, 159, 178, 182, 183, 184, 187, 188, 198, 133, 149, 153, 173, 177, 180, 181, 189, 199, 130, 131, 132, 145, 155, 156, 166, 175, 176, 185, 186, 170, 171];
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
    let mut v = alloc_big_vec(TOTAL_NUM);

    build_formula(&mut v);
    write_formula_to_file(&v);

    read_formula_from_file(&mut v);
    look_up(&v);
}

fn build_formula(v: &mut Vec<Pair>) {
    let start = Instant::now();

    let (tx, rx) = mpsc::channel();

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
        // println!("{:?}", thread_slice_array[threadid]);

        //create threads
        let c_tx = mpsc::Sender::clone(&tx);
        thread::spawn(move || {
            println!("Thread {} start...", threadid);
            for thread_slice in 0..thread_slice_array[threadid].1 {
                let slice = thread_slice_array[threadid].0 + thread_slice;
                for mobile_index in 0..SLICE_LEN {
                    let address = slice * SLICE_LEN + mobile_index;
                    if address < TOTAL_NUM {
                        let mobile = address_to_mobile(address);
                        let m_md5 = mobile_to_md5(mobile);
                        // println!("{},{},{},{}", threadid, address, mobile, m_md5);
                        c_tx.send((address, mobile_index as u16, m_md5)).unwrap();
                    }
                }
                // send total_num as address to indicate the end of a slice
                c_tx.send((TOTAL_NUM, slice as u16, 0)).unwrap();
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

    // recieve results from threads
    let mut count = 0;
    for (address, mobile_index, m_md5) in rx {
        if address < TOTAL_NUM {
            count += 1;
            v[address].md5 = m_md5;
            v[address].mobile_index = mobile_index;
        } else {
            // println!("slice {} completed.", index);
            // sort each slice
            let slice_start = mobile_index as usize * SLICE_LEN;
            let mut slice_end = slice_start + SLICE_LEN;
            if slice_end > TOTAL_NUM {
                slice_end = TOTAL_NUM;
            }
            let v_slice = &mut v[slice_start..slice_end];
            v_slice.sort_by(|a, b| a.md5.cmp(&b.md5));
        }
    }
    println!("{}", count);

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

fn look_up(v: &Vec<Pair>) {
    let start = Instant::now();

    let digest = md5::compute("13856124456".to_string());
    let m_md5 = digest_to_md5(digest);
    for slice in 0..SLICE_NUM {
        // look up in each slice
        let slice_start = slice * SLICE_LEN;
        let mut slice_end = slice_start + SLICE_LEN;
        if slice_end > TOTAL_NUM {
            slice_end = TOTAL_NUM;
        }
        let v_slice = &v[slice_start..slice_end];

        // for i in 0..v_slice.len() {
        //     if v_slice[i].md5 == m_md5 {
        //         println!("{},{},{}", slice,v_slice[i].mobile,restore_mobile(v_slice[i].mobile,slice));
        //     }
        // }
        let result = v_slice.binary_search_by(|probe| probe.md5.cmp(&m_md5));
        match result {
            Ok(index) => {
                let mut i = index;
                while i > 0 && v_slice[i].md5 == m_md5 {
                    let mobile_index = v_slice[i].mobile_index;
                    let mobile = restore_mobile(mobile_index, slice);
                    // println!("{},{},{}", slice, mobile_index, mobile);
                    if md5::compute(mobile.to_string()) == digest {
                        println!("## found {}", mobile);
                    }
                    i -= 1;
                }
            }
            Err(_) => {
                // println!("{} not found", slice);
            }
        }
    }

    println!("Look up costed {:?}", start.elapsed());
}

fn write_formula_to_file(v: &Vec<Pair>) {
    let start = Instant::now();

    // write md5 then mobile brings better compress ratio
    // however it increases memory access costs
    let mut file = BufWriter::new(File::create("formula.dat").unwrap());
    for pair in v {
        file.write_u16::<NativeEndian>(pair.md5).unwrap();
        file.write_u16::<NativeEndian>(pair.mobile_index).unwrap();
    }
    println!("Write formula costed {:?}", start.elapsed());
}

fn read_formula_from_file(v: &mut Vec<Pair>) {
    let start = Instant::now();

    let mut file = buffered_reader::File::open("formula.dat").unwrap();
    for i in 0..TOTAL_NUM {
        v[i].md5 = file.read_u16::<NativeEndian>().unwrap();
        v[i].mobile_index = file.read_u16::<NativeEndian>().unwrap();
    }
    println!("Read formula costed {:?}", start.elapsed());
}
