#[no_mangle]
pub extern fn test_rust() -> *const u8 {
    
    let greeting = "hello from rust.\0";
    greeting.as_ptr()
}
