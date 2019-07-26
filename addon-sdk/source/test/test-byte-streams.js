





const byteStreams = require("sdk/io/byte-streams");
const file = require("sdk/io/file");
const { pathFor } = require("sdk/system");
const { Loader } = require("sdk/test/loader");

const STREAM_CLOSED_ERROR = "The stream is closed and cannot be used.";


const BUFFER_BYTE_LEN = 0x8000;

exports.testWriteRead = function (test) {
  let fname = dataFileFilename();

  
  let str = "exports.testWriteRead data!";
  let stream = open(test, fname, true);
  test.assert(!stream.closed, "stream.closed after open should be false");
  stream.write(str);
  stream.close();
  test.assert(stream.closed, "Stream should be closed after stream.close");
  test.assertRaises(function () stream.write("This shouldn't be written!"),
                    STREAM_CLOSED_ERROR,
                    "stream.write after close should raise error");

  
  stream = open(test, fname);
  test.assertEqual(stream.read(), str,
                   "stream.read should return string written");
  test.assertEqual(stream.read(), "",
                   "stream.read at EOS should return empty string");
  stream.close();
  test.assert(stream.closed, "Stream should be closed after stream.close");
  test.assertRaises(function () stream.read(),
                    STREAM_CLOSED_ERROR,
                    "stream.read after close should raise error");

  file.remove(fname);
};


exports.testWriteReadBig = function (test) {
  let str = "";
  let bufLen = BUFFER_BYTE_LEN;
  let fileSize = bufLen * 10;
  for (let i = 0; i < fileSize; i++)
    str += i % 10;
  let fname = dataFileFilename();
  let stream = open(test, fname, true);
  stream.write(str);
  stream.close();
  stream = open(test, fname);
  test.assertEqual(stream.read(), str,
                   "stream.read should return string written");
  stream.close();
  file.remove(fname);
};


exports.testWriteReadChunks = function (test) {
  let str = "";
  let bufLen = BUFFER_BYTE_LEN;
  let fileSize = bufLen * 10;
  for (let i = 0; i < fileSize; i++)
    str += i % 10;
  let fname = dataFileFilename();
  let stream = open(test, fname, true);
  let i = 0;
  while (i < str.length) {
    
    let chunk = str.substr(i, bufLen + 1);
    stream.write(chunk);
    i += bufLen + 1;
  }
  stream.close();
  stream = open(test, fname);
  let readStr = "";
  bufLen = BUFFER_BYTE_LEN;
  let readLen = bufLen + 1;
  do {
    var frag = stream.read(readLen);
    readStr += frag;
  } while (frag);
  stream.close();
  test.assertEqual(readStr, str,
                   "stream.write and read in chunks should work as expected");
  file.remove(fname);
};

exports.testReadLengths = function (test) {
  let fname = dataFileFilename();
  let str = "exports.testReadLengths data!";
  let stream = open(test, fname, true);
  stream.write(str);
  stream.close();

  stream = open(test, fname);
  test.assertEqual(stream.read(str.length * 1000), str,
                   "stream.read with big byte length should return string " +
                   "written");
  stream.close();

  stream = open(test, fname);
  test.assertEqual(stream.read(0), "",
                   "string.read with zero byte length should return empty " +
                   "string");
  stream.close();

  stream = open(test, fname);
  test.assertEqual(stream.read(-1), "",
                   "string.read with negative byte length should return " +
                   "empty string");
  stream.close();

  file.remove(fname);
};

exports.testTruncate = function (test) {
  let fname = dataFileFilename();
  let str = "exports.testReadLengths data!";
  let stream = open(test, fname, true);
  stream.write(str);
  stream.close();

  stream = open(test, fname);
  test.assertEqual(stream.read(), str,
                   "stream.read should return string written");
  stream.close();

  stream = open(test, fname, true);
  stream.close();

  stream = open(test, fname);
  test.assertEqual(stream.read(), "",
                   "stream.read after truncate should be empty");
  stream.close();

  file.remove(fname);
};

exports.testUnload = function (test) {
  let loader = Loader(module);
  let file = loader.require("sdk/io/file");

  let filename = dataFileFilename("temp-b");
  let stream = file.open(filename, "wb");

  loader.unload();
  test.assert(stream.closed, "Stream should be closed after module unload");
};


function dataFileFilename() {
  return file.join(pathFor("ProfD"), "test-byte-streams-data");
}


function open(test, filename, forWriting) {
  let stream = file.open(filename, forWriting ? "wb" : "b");
  let klass = forWriting ? "ByteWriter" : "ByteReader";
  test.assert(stream instanceof byteStreams[klass],
              "Opened stream should be a " + klass);
  return stream;
}
