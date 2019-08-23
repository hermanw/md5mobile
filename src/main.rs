use std::sync::mpsc;
use std::thread;
use std::time::Instant;

static THREAD_NUM: u64 = 4;
static SLICE_NUM: u64 = 10000000;

struct Stat {
    max: usize,
    count: Vec<u32>,
}

fn main() {
    let start = Instant::now();

    let (tx, rx) = mpsc::channel();

    for threadid in 0..THREAD_NUM {
        let c_tx = mpsc::Sender::clone(&tx);
        thread::spawn(move || {
            println!("Thread {} start...", threadid);
            let n = SLICE_NUM/THREAD_NUM;
            for index in 0..n {
                let mobile: u64 = 13800000000 + n*threadid + index;
                let m_hash = mobile_to_md5(mobile) as usize;
                c_tx.send(m_hash).unwrap();
            }
            println!("Thread {} completed.", threadid);
        });
    }
    drop(tx);

    let mut v = alloc_big_vec(u32::max_value() as usize + 1);
    let mut stat = Stat {
        max: 0,
        count: vec![0; 10],
    };

    for m_hash in rx {
        let stat_index = v[m_hash] as usize;
        v[m_hash] = v[m_hash] + 1;

        stat.count[stat_index] += 1;
        if stat_index > 0 {
            stat.count[stat_index - 1] -= 1;
        }

        if stat_index + 1 > stat.max {
            stat.max += 1;
        }
    }

    println!("{}", stat.max);
    for i in 0..stat.max {
        println!("{}", stat.count[i]);
    }

    println!("It costs: {:?}", start.elapsed());
}

fn alloc_big_vec(len: usize) -> Vec<u32> {
    // let v: Vec<u32> = vec![0; len];
    let mut v: Vec<u32> = Vec::with_capacity(len);
    unsafe {
        v.set_len(len);
    }
    // println!("{}", v.len());
    v
}

fn mobile_to_md5(mobile: u64) -> u32 {
    let digest = md5::compute(mobile.to_string());
    let d0 = digest[0] as u32;
    let d1 = digest[1] as u32;
    let d2 = digest[2] as u32;
    let d3 = digest[3] as u32;
    // println!("{},{}", mobile,format!("{:x}",digest));
    (d0 << 24) + (d1 << 16) + (d2 << 8) + d3
}
