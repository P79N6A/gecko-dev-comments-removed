



const Ci = Components.interfaces;
const Cr = Components.results;
const CC = Components.Constructor;
const Cc = Components.classes;


const data = "0123456789";

const count = 10;

function test_multiplex_streams() {
  try {
    var MultiplexStream = CC("@mozilla.org/io/multiplex-input-stream;1",
                             "nsIMultiplexInputStream");
                             do_check_eq(1, 1);

    var BinaryInputStream = Components.Constructor("@mozilla.org/binaryinputstream;1",
                                                   "nsIBinaryInputStream");
    var BinaryOutputStream = Components.Constructor("@mozilla.org/binaryoutputstream;1",
                                                    "nsIBinaryOutputStream",
                                                    "setOutputStream");
    var multiplex = new MultiplexStream();
    for (var i = 0; i < count; ++i) {
      let s = Cc["@mozilla.org/io/string-input-stream;1"]
                .createInstance(Ci.nsIStringInputStream);
      s.setData(data, data.length);

      multiplex.appendStream(s);
    }
    var seekable = multiplex.QueryInterface(Ci.nsISeekableStream);
    var sis = Cc["@mozilla.org/scriptableinputstream;1"]
                .createInstance(Ci.nsIScriptableInputStream);
    sis.init(seekable);
    
    var readData = sis.read(20);
    do_check_eq(readData, data + data);
    
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, 35);
    do_check_eq(seekable.tell(), 35);
    do_check_eq(seekable.available(), 65);
    readData = sis.read(5);
    do_check_eq(readData, data.slice(5));
    do_check_eq(seekable.available(), 60);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, 40);
    do_check_eq(seekable.tell(), 40);
    do_check_eq(seekable.available(), 60);
    readData = sis.read(10);
    do_check_eq(readData, data);
    do_check_eq(seekable.tell(), 50);
    do_check_eq(seekable.available(), 50);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, 39);
    do_check_eq(seekable.tell(), 39);
    do_check_eq(seekable.available(), 61);
    readData = sis.read(11);
    do_check_eq(readData, data.slice(9) + data);
    do_check_eq(seekable.tell(), 50);
    do_check_eq(seekable.available(), 50);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
    do_check_eq(seekable.tell(), 0);
    do_check_eq(seekable.available(), 100);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, 50);
    
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_CUR, 15);
    do_check_eq(seekable.tell(), 65);
    do_check_eq(seekable.available(), 35);
    readData = sis.read(10);
    do_check_eq(readData, data.slice(5) + data.slice(0, 5));
    do_check_eq(seekable.tell(), 75);
    do_check_eq(seekable.available(), 25);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_CUR, -15);
    do_check_eq(seekable.tell(), 60);
    do_check_eq(seekable.available(), 40);
    readData = sis.read(10);
    do_check_eq(readData, data);
    do_check_eq(seekable.tell(), 70);
    do_check_eq(seekable.available(), 30);

    
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_END, -5);
    do_check_eq(seekable.tell(), data.length * count - 5);
    readData = sis.read(5);
    do_check_eq(readData, data.slice(5));
    do_check_eq(seekable.tell(), data.length * count);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_END, -15);
    do_check_eq(seekable.tell(), data.length * count - 15);
    readData = sis.read(15);
    do_check_eq(readData, data.slice(5) + data);
    do_check_eq(seekable.tell(), data.length * count);

    
    
    var caught = false;
    try {
      seekable.seek(Ci.nsISeekableStream.NS_SEEK_END, 15);
    } catch(e) {
      caught = true;
    }
    do_check_eq(caught, true);
    do_check_eq(seekable.tell(), data.length * count);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
    do_check_eq(seekable.tell(), 0);
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_CUR, -15);
    do_check_eq(seekable.tell(), 0);
    
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
    do_check_eq(seekable.tell(), 0);
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_CUR, 3 * data.length * count);
    do_check_eq(seekable.tell(), 100);
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_SET, data.length * count);
    do_check_eq(seekable.tell(), 100);
    seekable.seek(Ci.nsISeekableStream.NS_SEEK_CUR, -2 * data.length * count);
    do_check_eq(seekable.tell(), 0);
  } catch(e) {
    dump(e + "\n");
  }
}

function run_test() {
  test_multiplex_streams();
}

