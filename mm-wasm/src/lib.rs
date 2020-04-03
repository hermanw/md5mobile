use std::mem;
use std::ffi::{CString, CStr};
use std::os::raw::{c_char, c_void};

mod decoder;

#[no_mangle]
pub extern "C" fn alloc(size: usize) -> *mut c_void {
    let mut buf = Vec::with_capacity(size);
    let ptr = buf.as_mut_ptr();
    mem::forget(buf);
    return ptr as *mut c_void;
}

#[no_mangle]
pub extern "C" fn dealloc(ptr: *mut c_void, cap: usize) {
    unsafe  {
        let _buf = Vec::from_raw_parts(ptr, 0, cap);
    }
}

#[no_mangle]
pub extern "C" fn dealloc_str(ptr: *mut c_char) {
    unsafe {
        let _ = CString::from_raw(ptr);
    }
}

#[no_mangle]
pub extern "C" fn validate(data: *mut c_char) -> usize {
    prep(data).len()
}


fn prep(data: *mut c_char) -> Vec<(String, u64, u32)> {
    unsafe {
        let data = CStr::from_ptr(data);
        let mut v_hash: Vec<(String, u64, u32)> = vec![];
        let v: Vec<&str> = data.to_str().unwrap().split(',').collect();
        for s in v {
            let s = s.trim().to_lowercase();
            if s.len() > 0 {
                let hash = s.to_string();
                let hash_bytes = hex::decode(s.to_string()).unwrap();
                let hash_bytes_prefix = *(hash_bytes.as_ptr() as *const u32);
                v_hash.push((hash,0,hash_bytes_prefix));
            }
        }
        v_hash
    }
}

#[no_mangle]
pub extern "C" fn decode(data: *mut c_char, thread_num: usize, threadid: usize) {
    let v_hash = prep(data);
    decoder::decode(v_hash, thread_num, threadid);
}
