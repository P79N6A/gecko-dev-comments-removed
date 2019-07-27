






























const { Buffer, TextEncoder, TextDecoder } = require('sdk/io/buffer');
const { safeMerge } = require('sdk/util/object');

const ENCODINGS = ['utf-8', 'utf-16le', 'utf-16be'];

exports.testBufferMain = function (assert) {
  let b = Buffer('abcdef');

  
  new Buffer('');
  new Buffer(0);
  
  
  
  ENCODINGS.forEach(enc => {
    new Buffer('', enc);
    assert.pass('Creating a buffer with ' + enc + ' does not throw');
  });

  ENCODINGS.forEach(function(encoding) {
    
    if (encoding === 'utf-8') return;
    var b = new Buffer(10);
    b.write('あいうえお', encoding);
    assert.equal(b.toString(encoding), 'あいうえお',
      'encode and decodes buffer with ' + encoding);
  });

  
  assert.throws(() => {
    b.toString('invalid');
  }, RangeError, 'invalid encoding for Buffer.toString');

  
  
  assert.equal(new Buffer('abc').toString('utf8', 0, 0), '',
    'toString 0-length buffer, valid range');
  assert.equal(new Buffer('abc').toString('utf8', -100, -100), '',
    'toString 0-length buffer, invalid range');
  assert.equal(new Buffer('abc').toString('utf8', 100, 100), '',
    'toString 0-length buffer, invalid range');

  
  assert.equal(new Buffer('abc').toString({toString: function() {
    return 'utf8';
  }}), 'abc', 'toString with object as an encoding');

  
  var buf = new Buffer([0, 0, 0, 0, 0]); 
  var sub = buf.slice(0, 4); 
  var written = sub.write('12345', 'utf8');
  assert.equal(written, 4, 'correct bytes written in slice');
  assert.equal(buf[4], 0, 'correct origin buffer value');

  
  
  Buffer(3.3).toString(); 
  assert.equal(Buffer(-1).length, 0);
  assert.equal(Buffer(NaN).length, 0);
  assert.equal(Buffer(3.3).length, 3);
  assert.equal(Buffer({length: 3.3}).length, 3);
  assert.equal(Buffer({length: 'BAM'}).length, 0);

  
  assert.equal(Buffer('99').length, 2);
  assert.equal(Buffer('13.37').length, 5);
};

exports.testIsEncoding = function (assert) {
  ENCODINGS.map(encoding => {
    assert.ok(Buffer.isEncoding(encoding),
      'Buffer.isEncoding ' + encoding + ' truthy');
  });
  ['not-encoding', undefined, null, 100, {}].map(encoding => {
    assert.ok(!Buffer.isEncoding(encoding),
      'Buffer.isEncoding ' + encoding + ' falsy');
  });
};

exports.testBufferCopy = function (assert) {
  
  var cntr = 0;
  var b = Buffer(1024); 

  assert.strictEqual(1024, b.length);
  b[0] = -1;
  assert.strictEqual(b[0], 255);

  var shimArray = [];
  for (var i = 0; i < 1024; i++) {
    b[i] = i % 256;
    shimArray[i] = i % 256;
  }

  compareBuffers(assert, b, shimArray, 'array notation');

  var c = new Buffer(512);
  assert.strictEqual(512, c.length);
  
  b.fill(++cntr);
  c.fill(++cntr);
  var copied = b.copy(c, 0, 0, 512);
  assert.strictEqual(512, copied,
    'copied ' + copied + ' bytes from b into c');

  compareBuffers(assert, b, c, 'copied to other buffer');

  
  b.fill(++cntr);
  c.fill(++cntr);
  var copied = c.copy(b, 0, 0);
  assert.strictEqual(c.length, copied,
    'copied ' + copied + ' bytes from c into b w/o sourceEnd');
  compareBuffers(assert, b, c,
    'copied to other buffer without specifying sourceEnd');

  
  b.fill(++cntr);
  c.fill(++cntr);
  var copied = c.copy(b, 0);
  assert.strictEqual(c.length, copied,
    'copied ' + copied + ' bytes from c into b w/o sourceStart');
  compareBuffers(assert, b, c,
    'copied to other buffer without specifying sourceStart');

  
  b.fill(++cntr);
  c.fill(++cntr);

  var copied = b.copy(c);
  assert.strictEqual(c.length, copied,
    'copied ' + copied + ' bytes from b into c w/o targetStart');
  compareBuffers(assert, b, c,
    'copy long buffer to shorter buffer without targetStart');

  
  b.fill(++cntr);
  c.fill(++cntr);
  var copied = b.copy(c, 0, b.length - Math.floor(c.length / 2));
  assert.strictEqual(Math.floor(c.length / 2), copied,
    'copied ' + copied + ' bytes from end of b into beg. of c');

  let successStatus = true;
  for (var i = 0; i < Math.floor(c.length / 2); i++) {
    if (b[b.length - Math.floor(c.length / 2) + i] !== c[i])
      successStatus = false;
  }

  for (var i = Math.floor(c.length /2) + 1; i < c.length; i++) {
    if (c[c.length-1] !== c[i])
      successStatus = false;
  }
  assert.ok(successStatus,
    'Copied bytes from end of large buffer into beginning of small buffer');

  
  b.fill(++cntr);
  c.fill(++cntr);
  var copied = b.copy(c, 0, 0, 513);
  assert.strictEqual(c.length, copied,
    'copied ' + copied + ' bytes from b trying to overrun c');
  compareBuffers(assert, b, c,
    'copying to buffer that would overflow');

  
  b.fill(++cntr);
  b.fill(++cntr, 256);
  var copied = b.copy(b, 0, 256, 1024);
  assert.strictEqual(768, copied,
    'copied ' + copied + ' bytes from b into b');

  compareBuffers(assert, b, shimArray.map(()=>cntr),
    'copy partial buffer to itself');

  
  var bb = new Buffer(10);
  bb.fill('hello crazy world');

  
  assert.throws(function() {
    Buffer(5).copy(Buffer(5), 0, -1);
  }, RangeError, 'buffer copy throws at negative sourceStart');

  
  b.fill(++cntr);
  c.fill(++cntr);
  var copied = b.copy(c, 0, 0, 1025);
  assert.strictEqual(copied, c.length,
    'copied ' + copied + ' bytes from b into c');
  compareBuffers(assert, b, c, 'copying should reset sourceEnd if targetEnd if sourceEnd > targetEnd');

  
  assert.throws(function() {
    b.copy(c, 0, 0, -1);
  }, RangeError, 'buffer copy throws at negative sourceEnd');

  
  assert.equal(b.copy(c, 0, 100, 10), 0);

  
  assert.equal(b.copy(c, 512, 0, 10), 0);

  
  b.copy(new Buffer(0), 0, 0, 0);

  
  b.copy(new Buffer(0), 1, 1, 1);
  b.copy(new Buffer(1), 1, 1, 1);

  
  b.copy(new Buffer(1), 0, 2048, 2048);
};

exports.testBufferWrite = function (assert) {
  let b = Buffer(1024);
  b.fill(0);

  assert.throws(() => {
    b.write('test string', 0, 5, 'invalid');
  }, RangeError, 'invalid encoding with buffer write throws');
  
  assert.throws(function() {
    b.write('', 2048);
  }, RangeError, 'writing a 0-length string beyond buffer throws');
  
  assert.throws(function() {
    b.write('a', -1);
  }, RangeError, 'writing negative offset on buffer throws');

  
  assert.throws(function() {
    b.write('a', 2048);
  }, RangeError, 'writing past buffer bounds from pool throws');

  

  
  
  
  
  var writeTest = new Buffer('abcdes');
  writeTest.write('n', 'utf8');

  writeTest.write('d', '2', 'utf8');
  writeTest.write('e', 3, 'utf8');

  assert.equal(writeTest.toString(), 'nbdees',
    'buffer write API alternative syntax works');
};

exports.testBufferWriteEncoding = function (assert) {

  
  var buf = new Buffer('\0');
  assert.equal(buf.length, 1);
  buf = new Buffer('\0\0');
  assert.equal(buf.length, 2);

  buf = new Buffer(2);
  var written = buf.write(''); 
  assert.equal(written, 0);
  written = buf.write('\0'); 
  assert.equal(written, 1);
  written = buf.write('a\0'); 
  assert.equal(written, 2);
  






  written = buf.write('\0\0あ'); 
  buf = new Buffer(10);
  written = buf.write('あいう'); 
  assert.equal(written, 9);
  written = buf.write('あいう\0'); 
  assert.equal(written, 10);
};

exports.testBufferWriteWithMaxLength = function (assert) {
  
  var buf = new Buffer(4);
  buf.fill(0xFF);
  var written = buf.write('abcd', 1, 2, 'utf8');
  assert.equal(written, 2);
  assert.equal(buf[0], 0xFF);
  assert.equal(buf[1], 0x61);
  assert.equal(buf[2], 0x62);
  assert.equal(buf[3], 0xFF);

  buf.fill(0xFF);
  written = buf.write('abcd', 1, 4);
  assert.equal(written, 3);
  assert.equal(buf[0], 0xFF);
  assert.equal(buf[1], 0x61);
  assert.equal(buf[2], 0x62);
  assert.equal(buf[3], 0x63);

  buf.fill(0xFF);
  
  








};

exports.testBufferSlice = function (assert) {
  var asciiString = 'hello world';
  var offset = 100;
  var b = Buffer(1024);
  b.fill(0);

  for (var i = 0; i < asciiString.length; i++) {
    b[i] = asciiString.charCodeAt(i);
  }
  var asciiSlice = b.toString('utf8', 0, asciiString.length);
  assert.equal(asciiString, asciiSlice);

  var written = b.write(asciiString, offset, 'utf8');
  assert.equal(asciiString.length, written);
  asciiSlice = b.toString('utf8', offset, offset + asciiString.length);
  assert.equal(asciiString, asciiSlice);

  var sliceA = b.slice(offset, offset + asciiString.length);
  var sliceB = b.slice(offset, offset + asciiString.length);
  compareBuffers(assert, sliceA, sliceB,
    'slicing is idempotent');

  let sliceTest = true;
  for (var j = 0; j < 100; j++) {
    var slice = b.slice(100, 150);
    if (50 !== slice.length)
      sliceTest = false;
    for (var i = 0; i < 50; i++) {
      if (b[100 + i] !== slice[i])
        sliceTest = false;
    }
  }
  assert.ok(sliceTest, 'massive slice runs do not affect buffer');

  
  let testBuf = new Buffer('abcde');
  assert.equal('bcde', testBuf.slice(1).toString(), 'single argument slice');

  
  assert.equal(0, Buffer('hello').slice(0, 0).length, 'slice(0,0) === 0');

  var buf = new Buffer('0123456789');
  assert.equal(buf.slice(-10, 10), '0123456789', 'buffer slice range correct');
  assert.equal(buf.slice(-20, 10), '0123456789', 'buffer slice range correct');
  assert.equal(buf.slice(-20, -10), '', 'buffer slice range correct');
  assert.equal(buf.slice(0, -1), '012345678', 'buffer slice range correct');
  assert.equal(buf.slice(2, -2), '234567', 'buffer slice range correct');
  assert.equal(buf.slice(0, 65536), '0123456789', 'buffer slice range correct');
  assert.equal(buf.slice(65536, 0), '', 'buffer slice range correct');

  sliceTest = true;
  for (var i = 0, s = buf.toString(); i < buf.length; ++i) {
    if (buf.slice(-i) != s.slice(-i)) sliceTest = false;
    if (buf.slice(0, -i) != s.slice(0, -i)) sliceTest = false;
  }
  assert.ok(sliceTest, 'buffer.slice should be consistent');

  
  b.fill(0);
  let sliced = b.slice(0, 10);
  let babyslice = sliced.slice(0, 5);

  for (let i = 0; i < sliced.length; i++)
    sliced[i] = 'jetpack'.charAt(i);

  compareBuffers(assert, b, sliced,
    'modifying sliced buffer affects original');

  compareBuffers(assert, b, babyslice,
    'modifying sliced buffer affects child-sliced buffer');

  for (let i = 0; i < sliced.length; i++)
    b[i] = 'odinmonkey'.charAt(i);

  compareBuffers(assert, b, sliced,
    'modifying original buffer affects sliced');

  compareBuffers(assert, b, babyslice,
    'modifying original buffer affects grandchild sliced buffer');
};

exports.testSlicingParents = function (assert) {
  let root = Buffer(5);
  let child = root.slice(0, 4);
  let grandchild = child.slice(0, 3);

  assert.equal(root.parent, undefined, 'a new buffer should not have a parent');

  
  assert.equal(root.slice(0,0).parent, undefined,
    '0-length slice should not have a parent');

  assert.equal(child.parent, root,
    'a valid slice\'s parent should be the original buffer (child)');

  assert.equal(grandchild.parent, root,
    'a valid slice\'s parent should be the original buffer (grandchild)');
};

exports.testIsBuffer = function (assert) {
  let buffer = new Buffer('content', 'utf8');
  assert.ok(Buffer.isBuffer(buffer), 'isBuffer truthy on buffers');
  assert.ok(!Buffer.isBuffer({}), 'isBuffer falsy on objects');
  assert.ok(!Buffer.isBuffer(new Uint8Array()),
    'isBuffer falsy on Uint8Array');
  assert.ok(Buffer.isBuffer(buffer.slice(0)), 'Buffer#slice should be a new buffer');
};

exports.testBufferConcat = function (assert) {
  let zero = [];
  let one = [ new Buffer('asdf') ];
  let long = [];
  for (let i = 0; i < 10; i++) long.push(new Buffer('asdf'));

  let flatZero = Buffer.concat(zero);
  let flatOne = Buffer.concat(one);
  let flatLong = Buffer.concat(long);
  let flatLongLen = Buffer.concat(long, 40);

  assert.equal(flatZero.length, 0);
  assert.equal(flatOne.toString(), 'asdf');
  assert.equal(flatOne, one[0]);
  assert.equal(flatLong.toString(), (new Array(10+1).join('asdf')));
  assert.equal(flatLongLen.toString(), (new Array(10+1).join('asdf')));
};

exports.testBufferByteLength = function (assert) {
  let str = '\u00bd + \u00bc = \u00be';
  assert.equal(Buffer.byteLength(str), 12,
    'correct byteLength of string');

  assert.equal(14, Buffer.byteLength('Il était tué'));
  assert.equal(14, Buffer.byteLength('Il était tué', 'utf8'));
  
  






};

exports.testTextEncoderDecoder = function (assert) {
  assert.ok(TextEncoder, 'TextEncoder exists');
  assert.ok(TextDecoder, 'TextDecoder exists');
};

exports.testOverflowedBuffers = function (assert) {
  assert.throws(function() {
    new Buffer(0xFFFFFFFF);
  }, RangeError, 'correctly throws buffer overflow');

  assert.throws(function() {
    new Buffer(0xFFFFFFFFF);
  }, RangeError, 'correctly throws buffer overflow');

  assert.throws(function() {
    var buf = new Buffer(8);
    buf.readFloatLE(0xffffffff);
  }, RangeError, 'correctly throws buffer overflow with readFloatLE');

  assert.throws(function() {
    var buf = new Buffer(8);
    buf.writeFloatLE(0.0, 0xffffffff);
  }, RangeError, 'correctly throws buffer overflow with writeFloatLE');

  
  assert.throws(function() {
    var buf = new Buffer(8);
    buf.readFloatLE(-1);
  }, RangeError, 'correctly throws with readFloatLE negative values');

  assert.throws(function() {
    var buf = new Buffer(8);
    buf.writeFloatLE(0.0, -1);
  }, RangeError, 'correctly throws with writeFloatLE with negative values');

  assert.throws(function() {
    var buf = new Buffer(8);
    buf.readFloatLE(-1);
  }, RangeError, 'correctly throws with readFloatLE with negative values');
};

exports.testReadWriteDataTypeErrors = function (assert) {
  var buf = new Buffer(0);
  assert.throws(function() { buf.readUInt8(0); }, RangeError,
    'readUInt8(0) throws');
  assert.throws(function() { buf.readInt8(0); }, RangeError,
    'readInt8(0) throws');

  [16, 32].forEach(function(bits) {
    var buf = new Buffer(bits / 8 - 1);
    assert.throws(function() { buf['readUInt' + bits + 'BE'](0); },
      RangeError,
      'readUInt' + bits + 'BE');

    assert.throws(function() { buf['readUInt' + bits + 'LE'](0); },
      RangeError,
      'readUInt' + bits + 'LE');

    assert.throws(function() { buf['readInt' + bits + 'BE'](0); },
      RangeError,
      'readInt' + bits + 'BE()');

    assert.throws(function() { buf['readInt' + bits + 'LE'](0); },
      RangeError,
      'readInt' + bits + 'LE()');
  });
};

safeMerge(exports, require('./buffers/test-write-types'));
safeMerge(exports, require('./buffers/test-read-types'));

function compareBuffers (assert, buf1, buf2, message) {
  let status = true;
  for (let i = 0; i < Math.min(buf1.length, buf2.length); i++) {
    if (buf1[i] !== buf2[i])
      status = false;
  }
  assert.ok(status, 'buffer successfully copied: ' + message);
}
require('sdk/test').run(exports);
