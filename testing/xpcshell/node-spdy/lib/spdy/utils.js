var spdy = require('../spdy'),
    utils = exports;

var zlib = require('zlib'),
    Buffer = require('buffer').Buffer;





utils.createDeflate = function createDeflate(version) {
  var deflate = zlib.createDeflate({
    dictionary: spdy.protocol[version].dictionary,
    windowBits: 11
  });

  
  deflate.locked = false;
  deflate.lockBuffer = [];

  return deflate;
};





utils.createInflate = function createInflate(version) {
  var inflate = zlib.createInflate({
    dictionary: spdy.protocol[version].dictionary,
    windowBits: 15
  });

  
  inflate.locked = false;
  inflate.lockBuffer = [];

  return inflate;
};






utils.resetZlibStream = function resetZlibStream(stream, callback) {
  if (stream.locked) {
    stream.lockBuffer.push(function() {
      resetZlibStream(stream, callback);
    });
    return;
  }

  stream.reset();
  stream.lockBuffer = [];

  callback(null);
};

var delta = 0;







utils.zstream = function zstream(stream, buffer, callback) {
  var flush = stream._flush,
      chunks = [],
      total = 0;

  if (stream.locked) {
    stream.lockBuffer.push(function() {
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
  stream.write(buffer);

  stream.once('error', function(err) {
    stream.removeAllListeners('data');
    callback(err);
  });

  stream.flush(function() {
    stream.removeAllListeners('data');
    stream.removeAllListeners('error');
    stream._flush = flush;

    callback(null, chunks, total);

    stream.locked = false;
    var deferred = stream.lockBuffer.shift();
    if (deferred) deferred();
  });
};






utils.zwrap = function zwrap(stream) {
  return function(data, callback) {
    utils.zstream(stream, data, callback);
  };
};
