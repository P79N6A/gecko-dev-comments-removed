use std::error;
use std::fmt;
use std::io;
use std::result;

use byteorder::ByteOrder;


pub type Result<T> = result::Result<T, Error>;








#[derive(Debug)]
pub enum Error {
    
    
    
    
    UnexpectedEOF,
    
    Io(io::Error),
}

impl From<io::Error> for Error {
    fn from(err: io::Error) -> Error { Error::Io(err) }
}

impl From<Error> for io::Error {
    fn from(err: Error) -> io::Error {
        match err {
            Error::Io(err) => err,
            Error::UnexpectedEOF => io::Error::new(io::ErrorKind::Other,
                                                   "unexpected EOF")
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Error::UnexpectedEOF => write!(f, "Unexpected end of file."),
            Error::Io(ref err) => err.fmt(f),
        }
    }
}

impl error::Error for Error {
    fn description(&self) -> &str {
        match *self {
            Error::UnexpectedEOF => "Unexpected end of file.",
            Error::Io(ref err) => error::Error::description(err),
        }
    }

    fn cause(&self) -> Option<&error::Error> {
        match *self {
            Error::UnexpectedEOF => None,
            Error::Io(ref err) => err.cause(),
        }
    }
}



















pub trait ReadBytesExt: io::Read {
    
    
    
    
    fn read_u8(&mut self) -> Result<u8> {
        let mut buf = [0; 1];
        try!(read_full(self, &mut buf));
        Ok(buf[0])
    }

    
    
    
    
    fn read_i8(&mut self) -> Result<i8> {
        let mut buf = [0; 1];
        try!(read_full(self, &mut buf));
        Ok(buf[0] as i8)
    }

    
    fn read_u16<T: ByteOrder>(&mut self) -> Result<u16> {
        let mut buf = [0; 2];
        try!(read_full(self, &mut buf));
        Ok(T::read_u16(&buf))
    }

    
    fn read_i16<T: ByteOrder>(&mut self) -> Result<i16> {
        let mut buf = [0; 2];
        try!(read_full(self, &mut buf));
        Ok(T::read_i16(&buf))
    }

    
    fn read_u32<T: ByteOrder>(&mut self) -> Result<u32> {
        let mut buf = [0; 4];
        try!(read_full(self, &mut buf));
        Ok(T::read_u32(&buf))
    }

    
    fn read_i32<T: ByteOrder>(&mut self) -> Result<i32> {
        let mut buf = [0; 4];
        try!(read_full(self, &mut buf));
        Ok(T::read_i32(&buf))
    }

    
    fn read_u64<T: ByteOrder>(&mut self) -> Result<u64> {
        let mut buf = [0; 8];
        try!(read_full(self, &mut buf));
        Ok(T::read_u64(&buf))
    }

    
    fn read_i64<T: ByteOrder>(&mut self) -> Result<i64> {
        let mut buf = [0; 8];
        try!(read_full(self, &mut buf));
        Ok(T::read_i64(&buf))
    }

    
    fn read_uint<T: ByteOrder>(&mut self, nbytes: usize) -> Result<u64> {
        let mut buf = [0; 8];
        try!(read_full(self, &mut buf[..nbytes]));
        Ok(T::read_uint(&buf[..nbytes], nbytes))
    }

    
    fn read_int<T: ByteOrder>(&mut self, nbytes: usize) -> Result<i64> {
        let mut buf = [0; 8];
        try!(read_full(self, &mut buf[..nbytes]));
        Ok(T::read_int(&buf[..nbytes], nbytes))
    }

    
    
    fn read_f32<T: ByteOrder>(&mut self) -> Result<f32> {
        let mut buf = [0; 4];
        try!(read_full(self, &mut buf));
        Ok(T::read_f32(&buf))
    }

    
    
    fn read_f64<T: ByteOrder>(&mut self) -> Result<f64> {
        let mut buf = [0; 8];
        try!(read_full(self, &mut buf));
        Ok(T::read_f64(&buf))
    }
}



impl<R: io::Read + ?Sized> ReadBytesExt for R {}

fn read_full<R: io::Read + ?Sized>(rdr: &mut R, buf: &mut [u8]) -> Result<()> {
    let mut nread = 0usize;
    while nread < buf.len() {
        match rdr.read(&mut buf[nread..]) {
            Ok(0) => return Err(Error::UnexpectedEOF),
            Ok(n) => nread += n,
            Err(ref e) if e.kind() == io::ErrorKind::Interrupted => {},
            Err(e) => return Err(From::from(e))
        }
    }
    Ok(())
}

fn write_all<W: io::Write + ?Sized>(wtr: &mut W, buf: &[u8]) -> Result<()> {
    wtr.write_all(buf).map_err(From::from)
}



















pub trait WriteBytesExt: io::Write {
    
    
    
    
    fn write_u8(&mut self, n: u8) -> Result<()> {
        write_all(self, &[n])
    }

    
    
    
    
    fn write_i8(&mut self, n: i8) -> Result<()> {
        write_all(self, &[n as u8])
    }

    
    fn write_u16<T: ByteOrder>(&mut self, n: u16) -> Result<()> {
        let mut buf = [0; 2];
        T::write_u16(&mut buf, n);
        write_all(self, &buf)
    }

    
    fn write_i16<T: ByteOrder>(&mut self, n: i16) -> Result<()> {
        let mut buf = [0; 2];
        T::write_i16(&mut buf, n);
        write_all(self, &buf)
    }

    
    fn write_u32<T: ByteOrder>(&mut self, n: u32) -> Result<()> {
        let mut buf = [0; 4];
        T::write_u32(&mut buf, n);
        write_all(self, &buf)
    }

    
    fn write_i32<T: ByteOrder>(&mut self, n: i32) -> Result<()> {
        let mut buf = [0; 4];
        T::write_i32(&mut buf, n);
        write_all(self, &buf)
    }

    
    fn write_u64<T: ByteOrder>(&mut self, n: u64) -> Result<()> {
        let mut buf = [0; 8];
        T::write_u64(&mut buf, n);
        write_all(self, &buf)
    }

    
    fn write_i64<T: ByteOrder>(&mut self, n: i64) -> Result<()> {
        let mut buf = [0; 8];
        T::write_i64(&mut buf, n);
        write_all(self, &buf)
    }

    
    
    fn write_f32<T: ByteOrder>(&mut self, n: f32) -> Result<()> {
        let mut buf = [0; 4];
        T::write_f32(&mut buf, n);
        write_all(self, &buf)
    }

    
    
    fn write_f64<T: ByteOrder>(&mut self, n: f64) -> Result<()> {
        let mut buf = [0; 8];
        T::write_f64(&mut buf, n);
        write_all(self, &buf)
    }
}



impl<W: io::Write + ?Sized> WriteBytesExt for W {}
