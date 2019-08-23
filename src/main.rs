use std::time::Instant;

fn main() {
    let start = Instant::now();

    let mut v = alloc_big_vec(u32::max_value() as usize + 1);

    let mut max = 0;
    let mut stat: Vec<u32> = Vec::with_capacity(10);
    unsafe {
        stat.set_len(10);
    }

    for index in 0..100000000 {
        let mobile: u64 = index + 13800000000;
        let m_hash = mobile_to_md5(mobile) as usize;
        v[m_hash] = v[m_hash] + 1;
        let stat_index = (v[m_hash] - 1) as usize;
        stat[stat_index] = stat[stat_index] + 1;
        if stat_index > 1 {
            stat[stat_index - 1] = stat[stat_index - 1] + 1;
        }
        if stat_index > max {
            max = stat_index
        }
    }
    println!("{}", max);
    for i in 0..max {
        println!("{}", stat[i]);
    }
    println!("It costs: {:?}", start.elapsed());
}

fn alloc_big_vec(len: usize) -> Vec<u32> {
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
