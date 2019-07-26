





const file = require("sdk/io/file");
const { pathFor } = require("sdk/system");
const { Loader } = require("sdk/test/loader");

const STREAM_CLOSED_ERROR = "The stream is closed and cannot be used.";


const BUFFER_BYTE_LEN = 0x8000;

exports.testWriteRead = function (test) {
  let fname = dataFileFilename();

  
  let str = "exports.testWriteRead data!";
  let stream = file.open(fname, "w");
  test.assert(!stream.closed, "stream.closed after open should be false");
  stream.write(str);
  stream.close();
  test.assert(stream.closed, "stream.closed after close should be true");
  test.assertRaises(function () stream.close(),
                    STREAM_CLOSED_ERROR,
                    "stream.close after already closed should raise error");
  test.assertRaises(function () stream.write("This shouldn't be written!"),
                    STREAM_CLOSED_ERROR,
                    "stream.write after close should raise error");

  
  stream = file.open(fname);
  test.assert(!stream.closed, "stream.closed after open should be false");
  test.assertEqual(stream.read(), str,
                   "stream.read should return string written");
  test.assertEqual(stream.read(), "",
                   "stream.read at EOS should return empty string");
  stream.close();
  test.assert(stream.closed, "stream.closed after close should be true");
  test.assertRaises(function () stream.close(),
                    STREAM_CLOSED_ERROR,
                    "stream.close after already closed should raise error");
  test.assertRaises(function () stream.read(),
                    STREAM_CLOSED_ERROR,
                    "stream.read after close should raise error");

  
  
  
  str = "";
  let bufLen = BUFFER_BYTE_LEN;
  let fileSize = bufLen * 10;
  for (let i = 0; i < fileSize; i++)
    str += i % 10;
  stream = file.open(fname, "w");
  stream.write(str);
  stream.close();
  stream = file.open(fname);
  test.assertEqual(stream.read(), str,
                   "stream.read should return string written");
  stream.close();

  
  stream = file.open(fname, "w");
  let i = 0;
  while (i < str.length) {
    
    let chunk = str.substr(i, bufLen + 1);
    stream.write(chunk);
    i += bufLen + 1;
  }
  stream.close();
  stream = file.open(fname);
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

  
  stream = file.open(fname);
  test.assertEqual(stream.read(fileSize * 100), str,
                   "stream.read with big byte length should return string " +
                   "written");
  stream.close();

  stream = file.open(fname);
  test.assertEqual(stream.read(0), "",
                   "string.read with zero byte length should return empty " +
                   "string");
  stream.close();

  stream = file.open(fname);
  test.assertEqual(stream.read(-1), "",
                   "string.read with negative byte length should return " +
                   "empty string");
  stream.close();

  file.remove(fname);
};

exports.testWriteAsync = function (test) {
  test.waitUntilDone();

  let fname = dataFileFilename();
  let str = "exports.testWriteAsync data!";
  let stream = file.open(fname, "w");
  test.assert(!stream.closed, "stream.closed after open should be false");

  
  stream.writeAsync(str, function (err) {
    test.assertEqual(this, stream, "|this| should be the stream object");
    test.assertEqual(err, undefined,
                     "stream.writeAsync should not cause error");
    test.assert(stream.closed, "stream.closed after write should be true");
    test.assertRaises(function () stream.close(),
                      STREAM_CLOSED_ERROR,
                      "stream.close after already closed should raise error");
    test.assertRaises(function () stream.writeAsync("This shouldn't work!"),
                      STREAM_CLOSED_ERROR,
                      "stream.writeAsync after close should raise error");

    
    stream = file.open(fname, "r");
    test.assert(!stream.closed, "stream.closed after open should be false");
    let readStr = stream.read();
    test.assertEqual(readStr, str,
                     "string.read should yield string written");
    stream.close();
    file.remove(fname);
    test.done();
  });
};

exports.testUnload = function (test) {
  let loader = Loader(module);
  let file = loader.require("sdk/io/file");

  let filename = dataFileFilename("temp");
  let stream = file.open(filename, "w");

  loader.unload();
  test.assert(stream.closed, "stream should be closed after module unload");
};


function dataFileFilename() {
  return file.join(pathFor("ProfD"), "test-text-streams-data");
}
