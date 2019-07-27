var spdy = require('../spdy'),
    utils = exports;

var stream = require('stream'),
    zlib = require('zlib'),
    Buffer = require('buffer').Buffer;


utils.isLegacy = !stream.Duplex;
if (utils.isLegacy)
  utils.DuplexStream = stream;
else
  utils.DuplexStream = stream.Duplex;







utils.createDeflate = function createDeflate(version, compression) {
  var deflate = zlib.createDeflate({
    dictionary: spdy.protocol.dictionary[version],
    flush: zlib.Z_SYNC_FLUSH,
    windowBits: 11,
    level: compression ? zlib.Z_DEFAULT_COMPRESSION : zlib.Z_NO_COMPRESSION
  });

  
  deflate.locked = false;
  deflate.lockQueue = [];
  if (spdy.utils.isLegacy)
    deflate._flush = zlib.Z_SYNC_FLUSH;

  return deflate;
};






utils.createInflate = function createInflate(version) {
  var inflate = zlib.createInflate({
    dictionary: spdy.protocol.dictionary[version],
    flush: zlib.Z_SYNC_FLUSH,
    windowBits: 15
  });

  
  inflate.locked = false;
  inflate.lockQueue = [];
  if (spdy.utils.isLegacy)
    inflate._flush = zlib.Z_SYNC_FLUSH;

  return inflate;
};






utils.resetZlibStream = function resetZlibStream(stream, callback) {
  if (stream.locked) {
    stream.lockQueue.push(function() {
      resetZlibStream(stream, callback);
    });
    return;
  }

  stream.reset();
  stream.lockQueue = [];

  callback(null);
};

var delta = 0;







utils.zstream = function zstream(stream, buffer, callback) {
  var chunks = [],
      total = 0;

  if (stream.locked) {
    stream.lockQueue.push(function() {
      zstream(stream, buffer, callback);
    });
    return;
  }
  stream.locked = true;

  function collect(chunk) {
    chunks.push(chunk);
    total += chunk.length;
  }
  stream.on('data', collect);

  stream.write(buffer, done);

  function done() {
    stream.removeAllListeners('data');
    stream.removeAllListeners('error');

    if (callback)
      callback(null, chunks, total);

    stream.locked = false;
    var deferred = stream.lockQueue.shift();
    if (deferred)
      deferred();
  };

  stream.once('error', function(err) {
    stream.removeAllListeners('data');
    callback(err);
    callback = null;
  });
};






utils.zwrap = function zwrap(stream) {
  return function(data, callback) {
    utils.zstream(stream, data, callback);
  };
};

if (typeof setImmediate === 'undefined')
  utils.nextTick = process.nextTick.bind(process);
else
  utils.nextTick = setImmediate;
