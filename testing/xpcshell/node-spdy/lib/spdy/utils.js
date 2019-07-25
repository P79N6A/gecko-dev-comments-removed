var utils = exports;

var zlib = require('zlib'),
    Buffer = require('buffer').Buffer;


var dictionary = new Buffer([
  'optionsgetheadpostputdeletetraceacceptaccept-charsetaccept-encodingaccept-',
  'languageauthorizationexpectfromhostif-modified-sinceif-matchif-none-matchi',
  'f-rangeif-unmodifiedsincemax-forwardsproxy-authorizationrangerefererteuser',
  '-agent10010120020120220320420520630030130230330430530630740040140240340440',
  '5406407408409410411412413414415416417500501502503504505accept-rangesageeta',
  'glocationproxy-authenticatepublicretry-afterservervarywarningwww-authentic',
  'ateallowcontent-basecontent-encodingcache-controlconnectiondatetrailertran',
  'sfer-encodingupgradeviawarningcontent-languagecontent-lengthcontent-locati',
  'oncontent-md5content-rangecontent-typeetagexpireslast-modifiedset-cookieMo',
  'ndayTuesdayWednesdayThursdayFridaySaturdaySundayJanFebMarAprMayJunJulAugSe',
  'pOctNovDecchunkedtext/htmlimage/pngimage/jpgimage/gifapplication/xmlapplic',
  'ation/xhtmltext/plainpublicmax-agecharset=iso-8859-1utf-8gzipdeflateHTTP/1',
  '.1statusversionurl\x00'
].join(''));





utils.createDeflate = function createDeflate() {
  var deflate = zlib.createDeflate({ dictionary: dictionary, windowBits: 11 });

  
  deflate.locked = false;
  deflate.lockBuffer = [];

  return deflate;
};





utils.createInflate = function createInflate() {
  var inflate = zlib.createInflate({ dictionary: dictionary, windowBits: 15 });

  
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

  stream.flush(function() {
    stream.removeAllListeners('data');
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

