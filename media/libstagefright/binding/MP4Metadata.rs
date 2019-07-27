






pub struct BoxHeader {
    
pub name: u32,
    
pub size: u64,
    
pub offset: u64,
}


pub struct FileTypeBox {
    name: u32,
    size: u64,
    major_brand: u32,
    minor_version: u32,
    compatible_brands: Vec<u32>,
}


pub struct MovieHeaderBox {
    pub name: u32,
    pub size: u64,
    pub timescale: u32,
    pub duration: u64,
    
}

pub struct TrackHeaderBox {
    pub name: u32,
    pub size: u64,
    pub track_id: u32,
    pub enabled: bool,
    pub duration: u64,
    pub width: u32,
    pub height: u32,
}

mod byteorder;
use byteorder::{BigEndian, ReadBytesExt};
use std::io::{Read, Seek, SeekFrom, Take};
use std::io::Cursor;


pub fn read_box_header<T: ReadBytesExt>(src: &mut T) -> byteorder::Result<BoxHeader> {
    let tmp_size = try!(src.read_u32::<BigEndian>());
    let name = try!(src.read_u32::<BigEndian>());
    let size = match tmp_size {
        1 => try!(src.read_u64::<BigEndian>()),
        _ => tmp_size as u64,
    };
    assert!(size >= 8);
    if tmp_size == 1 {
        assert!(size >= 16);
    }
    let offset = match tmp_size {
        1 => 4 + 4 + 8,
        _ => 4 + 4,
    };
    assert!(offset <= size);
    Ok(BoxHeader{
      name: name,
      size: size,
      offset: offset,
    })
}


fn read_fullbox_extra<T: ReadBytesExt>(src: &mut T) -> (u8, u32) {
    let version = src.read_u8().unwrap();
    let flags_a = src.read_u8().unwrap();
    let flags_b = src.read_u8().unwrap();
    let flags_c = src.read_u8().unwrap();
    (version, (flags_a as u32) << 16 |
              (flags_b as u32) <<  8 |
              (flags_c as u32))
}


pub fn skip_box_content<T: ReadBytesExt + Seek>
  (src: &mut T, header: &BoxHeader)
  -> std::io::Result<u64>
{
    src.seek(SeekFrom::Current((header.size - header.offset) as i64))
}


fn limit<'a, T: Read>(f: &'a mut T, h: &BoxHeader) -> Take<&'a mut T> {
    f.take(h.size - h.offset)
}


fn recurse<T: Read>(f: &mut T, h: &BoxHeader) -> byteorder::Result<()> {
    use std::error::Error;
    println!("{} -- recursing", h);
    
    
    
    
    
    let buf: Vec<u8> = limit(f, &h)
        .bytes()
        .map(|u| u.unwrap())
        .collect();
    let mut content = Cursor::new(buf);
    loop {
        match read_box(&mut content) {
            Ok(_) => {},
            Err(byteorder::Error::UnexpectedEOF) => {
                
                
                
                println!("Caught byteorder::Error::UnexpectedEOF");
                break;
            },
            Err(byteorder::Error::Io(e)) => {
                println!("I/O Error '{:?}' reading box: {}",
                         e.kind(), e.description());
                return Err(byteorder::Error::Io(e));
            },
        }
    }
    println!("{} -- end", h);
    Ok(())
}




pub fn read_box<T: Read + Seek>(f: &mut T) -> byteorder::Result<()> {
    read_box_header(f).and_then(|h| {
        match &(fourcc_to_string(h.name))[..] {
            "ftyp" => {
                let mut content = limit(f, &h);
                let ftyp = try!(read_ftyp(&mut content, &h));
                println!("{}", ftyp);
            },
            "moov" => try!(recurse(f, &h)),
            "mvhd" => {
                let mut content = limit(f, &h);
                let mvhd = try!(read_mvhd(&mut content, &h));
                println!("  {}", mvhd);
            },
            "trak" => try!(recurse(f, &h)),
            "tkhd" => {
                let mut content = limit(f, &h);
                let tkhd = try!(read_tkhd(&mut content, &h));
                println!("  {}", tkhd);
            },
            _ => {
                
                println!("{} (skipped)", h);
                try!(skip_box_content(f, &h).and(Ok(())));
            },
        };
        Ok(()) 
    })
}



#[no_mangle]
pub unsafe extern fn read_box_from_buffer(buffer: *const u8, size: usize)
  -> bool {
    use std::slice;
    use std::thread;

    
    if buffer.is_null() || size < 8 {
        return false;
    }

    
    let b = slice::from_raw_parts(buffer, size);
    let mut c = Cursor::new(b);

    
    let task = thread::spawn(move || {
        read_box(&mut c).or_else(|e| { match e {
            
            byteorder::Error::UnexpectedEOF => { Ok(()) },
            e => { Err(e) },
        }}).unwrap();
    });
    
    task.join().is_ok()
}



pub fn read_ftyp<T: ReadBytesExt>(src: &mut T, head: &BoxHeader)
  -> byteorder::Result<FileTypeBox> {
    let major = try!(src.read_u32::<BigEndian>());
    let minor = try!(src.read_u32::<BigEndian>());
    let brand_count = (head.size - 8 - 8) /4;
    let mut brands = Vec::new();
    for _ in 0..brand_count {
        brands.push( try!(src.read_u32::<BigEndian>()) );
    }
    Ok(FileTypeBox{
        name: head.name,
        size: head.size,
        major_brand: major,
        minor_version: minor,
        compatible_brands: brands,
    })
}


pub fn read_mvhd<T: ReadBytesExt>(src: &mut T, head: &BoxHeader)
  -> byteorder::Result<MovieHeaderBox> {
    let (version, _) = read_fullbox_extra(src);
    match version {
        1 => {
            
            let mut skip: Vec<u8> = vec![0; 16];
            let r = try!(src.read(&mut skip));
            assert!(r == skip.len());
        },
        0 => {
            
            
            let mut skip: Vec<u8> = vec![0; 8];
            let r = try!(src.read(&mut skip));
            assert!(r == skip.len());
        },
        _ => panic!("invalid mhdr version"),
    }
    let timescale = src.read_u32::<BigEndian>().unwrap();
    let duration = match version {
        1 => try!(src.read_u64::<BigEndian>()),
        0 => try!(src.read_u32::<BigEndian>()) as u64,
        _ => panic!("invalid mhdr version"),
    };
    
    let mut skip: Vec<u8> = vec![0; 80];
    let r = try!(src.read(&mut skip));
    assert!(r == skip.len());
    Ok(MovieHeaderBox {
        name: head.name,
        size: head.size,
        timescale: timescale,
        duration: duration,
    })
}


pub fn read_tkhd<T: ReadBytesExt>(src: &mut T, head: &BoxHeader)
  -> byteorder::Result<TrackHeaderBox> {
    let (version, flags) = read_fullbox_extra(src);
    let disabled = flags & 0x1u32 == 0 || flags & 0x2u32 == 0;
    match version {
        1 => {
            
            let mut skip: Vec<u8> = vec![0; 16];
            let r = try!(src.read(&mut skip));
            assert!(r == skip.len());
        },
        0 => {
            
            
            let mut skip: Vec<u8> = vec![0; 8];
            let r = try!(src.read(&mut skip));
            assert!(r == skip.len());
        },
        _ => panic!("invalid tkhd version"),
    }
    let track_id = try!(src.read_u32::<BigEndian>());
    let _reserved = try!(src.read_u32::<BigEndian>());
    assert!(_reserved == 0);
    let duration = match version {
        1 => {
            try!(src.read_u64::<BigEndian>())
        },
        0 => try!(src.read_u32::<BigEndian>()) as u64,
        _ => panic!("invalid tkhd version"),
    };
    let _reserved = try!(src.read_u32::<BigEndian>());
    let _reserved = try!(src.read_u32::<BigEndian>());
    
    let mut skip: Vec<u8> = vec![0; 44];
    let r = try!(src.read(&mut skip));
    assert!(r == skip.len());
    let width = try!(src.read_u32::<BigEndian>());
    let height = try!(src.read_u32::<BigEndian>());
    Ok(TrackHeaderBox {
        name: head.name,
        size: head.size,
        track_id: track_id,
        enabled: !disabled,
        duration: duration,
        width: width,
        height: height,
    })
}


fn fourcc_to_string(name: u32) -> String {
    let u32_to_vec = |u| {
        vec!((u >> 24 & 0xffu32) as u8,
             (u >> 16 & 0xffu32) as u8,
             (u >>  8 & 0xffu32) as u8,
             (u & 0xffu32) as u8)
    };
    let name_bytes = u32_to_vec(name);
    String::from_utf8_lossy(&name_bytes).into_owned()
}

use std::fmt;
impl fmt::Display for BoxHeader {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "'{}' {} bytes", fourcc_to_string(self.name), self.size)
    }
}

impl fmt::Display for FileTypeBox {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let name = fourcc_to_string(self.name);
        let brand = fourcc_to_string(self.major_brand);
        write!(f, "'{}' {} bytes '{}' v{}", name, self.size,
            brand, self.minor_version)
    }
}

impl fmt::Display for MovieHeaderBox {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let name = fourcc_to_string(self.name);
        write!(f, "'{}' {} bytes duration {}s", name, self.size,
            (self.duration as f64)/(self.timescale as f64))
    }
}

use std::u16;
impl fmt::Display for TrackHeaderBox {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let name = fourcc_to_string(self.name);
        
        let base = u16::MAX as f64 + 1.0;
        let width = (self.width as f64) / base;
        let height = (self.height as f64) / base;
        let disabled = if self.enabled { "" } else { " (disabled)" };
        write!(f, "'{}' {} bytes duration {} id {} {}x{}{}",
            name, self.size, self.duration, self.track_id,
            width, height, disabled)
    }
}

#[test]
fn test_read_box_header() {
    use std::io::Cursor;
    use std::io::Write;
    let mut test: Vec<u8> = vec![0, 0, 0, 8];  
    write!(&mut test, "test").unwrap(); 
    let mut stream = Cursor::new(test);
    let parsed = read_box_header(&mut stream).unwrap();
    assert_eq!(parsed.name, 1952805748);
    assert_eq!(parsed.size, 8);
    println!("box {}", parsed);
}


#[test]
fn test_read_box_header_long() {
    use std::io::Cursor;
    let mut test: Vec<u8> = vec![0, 0, 0, 1]; 
    test.extend("long".to_string().into_bytes()); 
    test.extend(vec![0, 0, 0, 0, 0, 0, 16, 0]); 
    
    let mut stream = Cursor::new(test);
    let parsed = read_box_header(&mut stream).unwrap();
    assert_eq!(parsed.name, 1819242087);
    assert_eq!(parsed.size, 4096);
    println!("box {}", parsed);
}

#[test]
fn test_read_ftyp() {
    use std::io::Cursor;
    use std::io::Write;
    let mut test: Vec<u8> = vec![0, 0, 0, 24]; 
    write!(&mut test, "ftyp").unwrap(); 
    write!(&mut test, "mp42").unwrap(); 
    test.extend(vec![0, 0, 0, 0]);      
    write!(&mut test, "isom").unwrap(); 
    write!(&mut test, "mp42").unwrap();
    assert_eq!(test.len(), 24);

    let mut stream = Cursor::new(test);
    let header = read_box_header(&mut stream).unwrap();
    let parsed = read_ftyp(&mut stream, &header).unwrap();
    assert_eq!(parsed.name, 1718909296);
    assert_eq!(parsed.size, 24);
    assert_eq!(parsed.major_brand, 1836069938);
    assert_eq!(parsed.minor_version, 0);
    assert_eq!(parsed.compatible_brands.len(), 2);
    println!("box {}", parsed);
}
