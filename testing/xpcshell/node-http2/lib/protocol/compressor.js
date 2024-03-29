
















exports.HeaderTable = HeaderTable;
exports.HuffmanTable = HuffmanTable;
exports.HeaderSetCompressor = HeaderSetCompressor;
exports.HeaderSetDecompressor = HeaderSetDecompressor;
exports.Compressor = Compressor;
exports.Decompressor = Decompressor;

var TransformStream = require('stream').Transform;
var assert = require('assert');
var util = require('util');












function HeaderTable(log, limit) {
  var self = HeaderTable.staticTable.map(entryFromPair);
  self._log = log;
  self._limit = limit || DEFAULT_HEADER_TABLE_LIMIT;
  self._staticLength = self.length;
  self._size = 0;
  self._enforceLimit = HeaderTable.prototype._enforceLimit;
  self.add = HeaderTable.prototype.add;
  self.setSizeLimit = HeaderTable.prototype.setSizeLimit;
  return self;
}

function entryFromPair(pair) {
  var entry = pair.slice();
  entry._size = size(entry);
  return entry;
}









var DEFAULT_HEADER_TABLE_LIMIT = 4096;

function size(entry) {
  return (new Buffer(entry[0] + entry[1], 'utf8')).length + 32;
}



















HeaderTable.prototype._enforceLimit = function _enforceLimit(limit) {
  var droppedEntries = [];
  while ((this._size > 0) && (this._size > limit)) {
    var dropped = this.pop();
    this._size -= dropped._size;
    droppedEntries.unshift(dropped);
  }
  return droppedEntries;
};

HeaderTable.prototype.add = function(entry) {
  var limit = this._limit - entry._size;
  var droppedEntries = this._enforceLimit(limit);

  if (this._size <= limit) {
    this.splice(this._staticLength, 0, entry);
    this._size += entry._size;
  }

  return droppedEntries;
};


HeaderTable.prototype.setSizeLimit = function setSizeLimit(limit) {
  this._limit = limit;
  this._enforceLimit(this._limit);
};









HeaderTable.staticTable  = [
  [ ':authority'                  , ''            ],
  [ ':method'                     , 'GET'         ],
  [ ':method'                     , 'POST'        ],
  [ ':path'                       , '/'           ],
  [ ':path'                       , '/index.html' ],
  [ ':scheme'                     , 'http'        ],
  [ ':scheme'                     , 'https'       ],
  [ ':status'                     , '200'         ],
  [ ':status'                     , '204'         ],
  [ ':status'                     , '206'         ],
  [ ':status'                     , '304'         ],
  [ ':status'                     , '400'         ],
  [ ':status'                     , '404'         ],
  [ ':status'                     , '500'         ],
  [ 'accept-charset'              , ''            ],
  [ 'accept-encoding'             , 'gzip, deflate'],
  [ 'accept-language'             , ''            ],
  [ 'accept-ranges'               , ''            ],
  [ 'accept'                      , ''            ],
  [ 'access-control-allow-origin' , ''            ],
  [ 'age'                         , ''            ],
  [ 'allow'                       , ''            ],
  [ 'authorization'               , ''            ],
  [ 'cache-control'               , ''            ],
  [ 'content-disposition'         , ''            ],
  [ 'content-encoding'            , ''            ],
  [ 'content-language'            , ''            ],
  [ 'content-length'              , ''            ],
  [ 'content-location'            , ''            ],
  [ 'content-range'               , ''            ],
  [ 'content-type'                , ''            ],
  [ 'cookie'                      , ''            ],
  [ 'date'                        , ''            ],
  [ 'etag'                        , ''            ],
  [ 'expect'                      , ''            ],
  [ 'expires'                     , ''            ],
  [ 'from'                        , ''            ],
  [ 'host'                        , ''            ],
  [ 'if-match'                    , ''            ],
  [ 'if-modified-since'           , ''            ],
  [ 'if-none-match'               , ''            ],
  [ 'if-range'                    , ''            ],
  [ 'if-unmodified-since'         , ''            ],
  [ 'last-modified'               , ''            ],
  [ 'link'                        , ''            ],
  [ 'location'                    , ''            ],
  [ 'max-forwards'                , ''            ],
  [ 'proxy-authenticate'          , ''            ],
  [ 'proxy-authorization'         , ''            ],
  [ 'range'                       , ''            ],
  [ 'referer'                     , ''            ],
  [ 'refresh'                     , ''            ],
  [ 'retry-after'                 , ''            ],
  [ 'server'                      , ''            ],
  [ 'set-cookie'                  , ''            ],
  [ 'strict-transport-security'   , ''            ],
  [ 'transfer-encoding'           , ''            ],
  [ 'user-agent'                  , ''            ],
  [ 'vary'                        , ''            ],
  [ 'via'                         , ''            ],
  [ 'www-authenticate'            , ''            ]
];











util.inherits(HeaderSetDecompressor, TransformStream);
function HeaderSetDecompressor(log, table) {
  TransformStream.call(this, { objectMode: true });

  this._log = log.child({ component: 'compressor' });
  this._table = table;
  this._chunks = [];
}




HeaderSetDecompressor.prototype._transform = function _transform(chunk, encoding, callback) {
  this._chunks.push(chunk);
  callback();
};






















HeaderSetDecompressor.prototype._execute = function _execute(rep) {
  this._log.trace({ key: rep.name, value: rep.value, index: rep.index },
                  'Executing header representation');

  var entry, pair;

  if (rep.contextUpdate) {
    this.setTableSizeLimit(rep.newMaxSize);
  }

  
  
  else if (typeof rep.value === 'number') {
    var index = rep.value;
    entry = this._table[index];

    pair = entry.slice();
    this.push(pair);
  }

  
  
  
  
  
  
  
  else {
    if (typeof rep.name === 'number') {
      pair = [this._table[rep.name][0], rep.value];
    } else {
      pair = [rep.name, rep.value];
    }

    if (rep.index) {
      entry = entryFromPair(pair);
      this._table.add(entry);
    }

    this.push(pair);
  }
};





HeaderSetDecompressor.prototype._flush = function _flush(callback) {
  var buffer = concat(this._chunks);

  
  buffer.cursor = 0;
  while (buffer.cursor < buffer.length) {
    this._execute(HeaderSetDecompressor.header(buffer));
  }

  callback();
};













util.inherits(HeaderSetCompressor, TransformStream);
function HeaderSetCompressor(log, table) {
  TransformStream.call(this, { objectMode: true });

  this._log = log.child({ component: 'compressor' });
  this._table = table;
  this.push = TransformStream.prototype.push.bind(this);
}

HeaderSetCompressor.prototype.send = function send(rep) {
  this._log.trace({ key: rep.name, value: rep.value, index: rep.index },
                  'Emitting header representation');

  if (!rep.chunks) {
    rep.chunks = HeaderSetCompressor.header(rep);
  }
  rep.chunks.forEach(this.push);
};




HeaderSetCompressor.prototype._transform = function _transform(pair, encoding, callback) {
  var name = pair[0].toLowerCase();
  var value = pair[1];
  var entry, rep;

  
  var nameMatch = -1, fullMatch = -1;
  for (var droppedIndex = 0; droppedIndex < this._table.length; droppedIndex++) {
    entry = this._table[droppedIndex];
    if (entry[0] === name) {
      if (entry[1] === value) {
        fullMatch = droppedIndex;
        break;
      } else if (nameMatch === -1) {
        nameMatch = droppedIndex;
      }
    }
  }

  var mustNeverIndex = ((name === 'cookie' && value.length < 20) ||
                        (name === 'set-cookie' && value.length < 20) ||
                        name === 'authorization');

  if (fullMatch !== -1 && !mustNeverIndex) {
    this.send({ name: fullMatch, value: fullMatch, index: false });
  }

  
  else {
    entry = entryFromPair(pair);

    var indexing = (entry._size < this._table._limit / 2) && !mustNeverIndex;

    if (indexing) {
      this._table.add(entry);
    }

    this.send({ name: (nameMatch !== -1) ? nameMatch : name, value: value, index: indexing, mustNeverIndex: mustNeverIndex, contextUpdate: false });
  }

  callback();
};




HeaderSetCompressor.prototype._flush = function _flush(callback) {
  callback();
};

















HeaderSetCompressor.integer = function writeInteger(I, N) {
  var limit = Math.pow(2,N) - 1;
  if (I < limit) {
    return [new Buffer([I])];
  }

  var bytes = [];
  if (N !== 0) {
    bytes.push(limit);
  }
  I -= limit;

  var Q = 1, R;
  while (Q > 0) {
    Q = Math.floor(I / 128);
    R = I % 128;

    if (Q > 0) {
      R += 128;
    }
    bytes.push(R);

    I = Q;
  }

  return [new Buffer(bytes)];
};














HeaderSetDecompressor.integer = function readInteger(buffer, N) {
  var limit = Math.pow(2,N) - 1;

  var I = buffer[buffer.cursor] & limit;
  if (N !== 0) {
    buffer.cursor += 1;
  }

  if (I === limit) {
    var M = 0;
    do {
      I += (buffer[buffer.cursor] & 127) << M;
      M += 7;
      buffer.cursor += 1;
    } while (buffer[buffer.cursor - 1] & 128);
  }

  return I;
};



function HuffmanTable(table) {
  function createTree(codes, position) {
    if (codes.length === 1) {
      return [table.indexOf(codes[0])];
    }

    else {
      position = position || 0;
      var zero = [];
      var one = [];
      for (var i = 0; i < codes.length; i++) {
        var string = codes[i];
        if (string[position] === '0') {
          zero.push(string);
        } else {
          one.push(string);
        }
      }
      return [createTree(zero, position + 1), createTree(one, position + 1)];
    }
  }

  this.tree = createTree(table);

  this.codes = table.map(function(bits) {
    return parseInt(bits, 2);
  });
  this.lengths = table.map(function(bits) {
    return bits.length;
  });
}

HuffmanTable.prototype.encode = function encode(buffer) {
  var result = [];
  var space = 8;

  function add(data) {
    if (space === 8) {
      result.push(data);
    } else {
      result[result.length - 1] |= data;
    }
  }

  for (var i = 0; i < buffer.length; i++) {
    var byte = buffer[i];
    var code = this.codes[byte];
    var length = this.lengths[byte];

    while (length !== 0) {
      if (space >= length) {
        add(code << (space - length));
        code = 0;
        space -= length;
        length = 0;
      } else {
        var shift = length - space;
        var msb = code >> shift;
        add(msb);
        code -= msb << shift;
        length -= space;
        space = 0;
      }

      if (space === 0) {
        space = 8;
      }
    }
  }

  if (space !== 8) {
    add(this.codes[256] >> (this.lengths[256] - space));
  }

  return new Buffer(result);
};

HuffmanTable.prototype.decode = function decode(buffer) {
  var result = [];
  var subtree = this.tree;

  for (var i = 0; i < buffer.length; i++) {
    var byte = buffer[i];

    for (var j = 0; j < 8; j++) {
      var bit = (byte & 128) ? 1 : 0;
      byte = byte << 1;

      subtree = subtree[bit];
      if (subtree.length === 1) {
        result.push(subtree[0]);
        subtree = this.tree;
      }
    }
  }

  return new Buffer(result);
};






HuffmanTable.huffmanTable = new HuffmanTable([
  '1111111111000',
  '11111111111111111011000',
  '1111111111111111111111100010',
  '1111111111111111111111100011',
  '1111111111111111111111100100',
  '1111111111111111111111100101',
  '1111111111111111111111100110',
  '1111111111111111111111100111',
  '1111111111111111111111101000',
  '111111111111111111101010',
  '111111111111111111111111111100',
  '1111111111111111111111101001',
  '1111111111111111111111101010',
  '111111111111111111111111111101',
  '1111111111111111111111101011',
  '1111111111111111111111101100',
  '1111111111111111111111101101',
  '1111111111111111111111101110',
  '1111111111111111111111101111',
  '1111111111111111111111110000',
  '1111111111111111111111110001',
  '1111111111111111111111110010',
  '111111111111111111111111111110',
  '1111111111111111111111110011',
  '1111111111111111111111110100',
  '1111111111111111111111110101',
  '1111111111111111111111110110',
  '1111111111111111111111110111',
  '1111111111111111111111111000',
  '1111111111111111111111111001',
  '1111111111111111111111111010',
  '1111111111111111111111111011',
  '010100',
  '1111111000',
  '1111111001',
  '111111111010',
  '1111111111001',
  '010101',
  '11111000',
  '11111111010',
  '1111111010',
  '1111111011',
  '11111001',
  '11111111011',
  '11111010',
  '010110',
  '010111',
  '011000',
  '00000',
  '00001',
  '00010',
  '011001',
  '011010',
  '011011',
  '011100',
  '011101',
  '011110',
  '011111',
  '1011100',
  '11111011',
  '111111111111100',
  '100000',
  '111111111011',
  '1111111100',
  '1111111111010',
  '100001',
  '1011101',
  '1011110',
  '1011111',
  '1100000',
  '1100001',
  '1100010',
  '1100011',
  '1100100',
  '1100101',
  '1100110',
  '1100111',
  '1101000',
  '1101001',
  '1101010',
  '1101011',
  '1101100',
  '1101101',
  '1101110',
  '1101111',
  '1110000',
  '1110001',
  '1110010',
  '11111100',
  '1110011',
  '11111101',
  '1111111111011',
  '1111111111111110000',
  '1111111111100',
  '11111111111100',
  '100010',
  '111111111111101',
  '00011',
  '100011',
  '00100',
  '100100',
  '00101',
  '100101',
  '100110',
  '100111',
  '00110',
  '1110100',
  '1110101',
  '101000',
  '101001',
  '101010',
  '00111',
  '101011',
  '1110110',
  '101100',
  '01000',
  '01001',
  '101101',
  '1110111',
  '1111000',
  '1111001',
  '1111010',
  '1111011',
  '111111111111110',
  '11111111100',
  '11111111111101',
  '1111111111101',
  '1111111111111111111111111100',
  '11111111111111100110',
  '1111111111111111010010',
  '11111111111111100111',
  '11111111111111101000',
  '1111111111111111010011',
  '1111111111111111010100',
  '1111111111111111010101',
  '11111111111111111011001',
  '1111111111111111010110',
  '11111111111111111011010',
  '11111111111111111011011',
  '11111111111111111011100',
  '11111111111111111011101',
  '11111111111111111011110',
  '111111111111111111101011',
  '11111111111111111011111',
  '111111111111111111101100',
  '111111111111111111101101',
  '1111111111111111010111',
  '11111111111111111100000',
  '111111111111111111101110',
  '11111111111111111100001',
  '11111111111111111100010',
  '11111111111111111100011',
  '11111111111111111100100',
  '111111111111111011100',
  '1111111111111111011000',
  '11111111111111111100101',
  '1111111111111111011001',
  '11111111111111111100110',
  '11111111111111111100111',
  '111111111111111111101111',
  '1111111111111111011010',
  '111111111111111011101',
  '11111111111111101001',
  '1111111111111111011011',
  '1111111111111111011100',
  '11111111111111111101000',
  '11111111111111111101001',
  '111111111111111011110',
  '11111111111111111101010',
  '1111111111111111011101',
  '1111111111111111011110',
  '111111111111111111110000',
  '111111111111111011111',
  '1111111111111111011111',
  '11111111111111111101011',
  '11111111111111111101100',
  '111111111111111100000',
  '111111111111111100001',
  '1111111111111111100000',
  '111111111111111100010',
  '11111111111111111101101',
  '1111111111111111100001',
  '11111111111111111101110',
  '11111111111111111101111',
  '11111111111111101010',
  '1111111111111111100010',
  '1111111111111111100011',
  '1111111111111111100100',
  '11111111111111111110000',
  '1111111111111111100101',
  '1111111111111111100110',
  '11111111111111111110001',
  '11111111111111111111100000',
  '11111111111111111111100001',
  '11111111111111101011',
  '1111111111111110001',
  '1111111111111111100111',
  '11111111111111111110010',
  '1111111111111111101000',
  '1111111111111111111101100',
  '11111111111111111111100010',
  '11111111111111111111100011',
  '11111111111111111111100100',
  '111111111111111111111011110',
  '111111111111111111111011111',
  '11111111111111111111100101',
  '111111111111111111110001',
  '1111111111111111111101101',
  '1111111111111110010',
  '111111111111111100011',
  '11111111111111111111100110',
  '111111111111111111111100000',
  '111111111111111111111100001',
  '11111111111111111111100111',
  '111111111111111111111100010',
  '111111111111111111110010',
  '111111111111111100100',
  '111111111111111100101',
  '11111111111111111111101000',
  '11111111111111111111101001',
  '1111111111111111111111111101',
  '111111111111111111111100011',
  '111111111111111111111100100',
  '111111111111111111111100101',
  '11111111111111101100',
  '111111111111111111110011',
  '11111111111111101101',
  '111111111111111100110',
  '1111111111111111101001',
  '111111111111111100111',
  '111111111111111101000',
  '11111111111111111110011',
  '1111111111111111101010',
  '1111111111111111101011',
  '1111111111111111111101110',
  '1111111111111111111101111',
  '111111111111111111110100',
  '111111111111111111110101',
  '11111111111111111111101010',
  '11111111111111111110100',
  '11111111111111111111101011',
  '111111111111111111111100110',
  '11111111111111111111101100',
  '11111111111111111111101101',
  '111111111111111111111100111',
  '111111111111111111111101000',
  '111111111111111111111101001',
  '111111111111111111111101010',
  '111111111111111111111101011',
  '1111111111111111111111111110',
  '111111111111111111111101100',
  '111111111111111111111101101',
  '111111111111111111111101110',
  '111111111111111111111101111',
  '111111111111111111111110000',
  '11111111111111111111101110',
  '111111111111111111111111111111'
]);
































HeaderSetCompressor.string = function writeString(str) {
  str = new Buffer(str, 'utf8');

  var huffman = HuffmanTable.huffmanTable.encode(str);
  if (huffman.length < str.length) {
    var length = HeaderSetCompressor.integer(huffman.length, 7);
    length[0][0] |= 128;
    return length.concat(huffman);
  }

  else {
    length = HeaderSetCompressor.integer(str.length, 7);
    return length.concat(str);
  }
};

HeaderSetDecompressor.string = function readString(buffer) {
  var huffman = buffer[buffer.cursor] & 128;
  var length = HeaderSetDecompressor.integer(buffer, 7);
  var encoded = buffer.slice(buffer.cursor, buffer.cursor + length);
  buffer.cursor += length;
  return (huffman ? HuffmanTable.huffmanTable.decode(encoded) : encoded).toString('utf8');
};


























































































var representations = {
  indexed             : { prefix: 7, pattern: 0x80 },
  literalIncremental  : { prefix: 6, pattern: 0x40 },
  contextUpdate       : { prefix: 0, pattern: 0x20 },
  literalNeverIndexed : { prefix: 4, pattern: 0x10 },
  literal             : { prefix: 4, pattern: 0x00 }
};

HeaderSetCompressor.header = function writeHeader(header) {
  var representation, buffers = [];

  if (header.contextUpdate) {
    representation = representations.contextUpdate;
  } else if (typeof header.value === 'number') {
    representation = representations.indexed;
  } else if (header.index) {
    representation = representations.literalIncremental;
  } else if (header.mustNeverIndex) {
    representation = representations.literalNeverIndexed;
  } else {
    representation = representations.literal;
  }

  if (representation === representations.contextUpdate) {
    buffers.push(HeaderSetCompressor.integer(header.newMaxSize, 5));
  }

  else if (representation === representations.indexed) {
    buffers.push(HeaderSetCompressor.integer(header.value + 1, representation.prefix));
  }

  else {
    if (typeof header.name === 'number') {
      buffers.push(HeaderSetCompressor.integer(header.name + 1, representation.prefix));
    } else {
      buffers.push(HeaderSetCompressor.integer(0, representation.prefix));
      buffers.push(HeaderSetCompressor.string(header.name));
    }
    buffers.push(HeaderSetCompressor.string(header.value));
  }

  buffers[0][0][0] |= representation.pattern;

  return Array.prototype.concat.apply([], buffers); 
};

HeaderSetDecompressor.header = function readHeader(buffer) {
  var representation, header = {};

  var firstByte = buffer[buffer.cursor];
  if (firstByte & 0x80) {
    representation = representations.indexed;
  } else if (firstByte & 0x40) {
    representation = representations.literalIncremental;
  } else if (firstByte & 0x20) {
    representation = representations.contextUpdate;
  } else if (firstByte & 0x10) {
    representation = representations.literalNeverIndexed;
  } else {
    representation = representations.literal;
  }

  header.value = header.name = -1;
  header.index = false;
  header.contextUpdate = false;
  header.newMaxSize = 0;
  header.mustNeverIndex = false;

  if (representation === representations.contextUpdate) {
    header.contextUpdate = true;
    header.newMaxSize = HeaderSetDecompressor.integer(buffer, 5);
  }

  else if (representation === representations.indexed) {
    header.value = header.name = HeaderSetDecompressor.integer(buffer, representation.prefix) - 1;
  }

  else {
    header.name = HeaderSetDecompressor.integer(buffer, representation.prefix) - 1;
    if (header.name === -1) {
      header.name = HeaderSetDecompressor.string(buffer);
    }
    header.value = HeaderSetDecompressor.string(buffer);
    header.index = (representation === representations.literalIncremental);
    header.mustNeverIndex = (representation === representations.literalNeverIndexed);
  }

  return header;
};























var MAX_HTTP_PAYLOAD_SIZE = 16384;





util.inherits(Compressor, TransformStream);
function Compressor(log, type) {
  TransformStream.call(this, { objectMode: true });

  this._log = log.child({ component: 'compressor' });

  assert((type === 'REQUEST') || (type === 'RESPONSE'));
  this._table = new HeaderTable(this._log);
}


Compressor.prototype.setTableSizeLimit = function setTableSizeLimit(size) {
  this._table.setSizeLimit(size);
};




Compressor.prototype.compress = function compress(headers) {
  var compressor = new HeaderSetCompressor(this._log, this._table);
  var colonHeaders = [];
  var nonColonHeaders = [];

  
  for (var name in headers) {
    if (name.trim()[0] === ':') {
      colonHeaders.push(name);
    } else {
      nonColonHeaders.push(name);
    }
  }

  function compressHeader(name) {
    var value = headers[name];
    name = String(name).toLowerCase();

    
    
    if (name == 'cookie') {
      if (!(value instanceof Array)) {
        value = [value];
      }
      value = Array.prototype.concat.apply([], value.map(function(cookie) {
        return String(cookie).split(';').map(trim);
      }));
    }

    if (value instanceof Array) {
      for (var i = 0; i < value.length; i++) {
        compressor.write([name, String(value[i])]);
      }
    } else {
      compressor.write([name, String(value)]);
    }
  }

  colonHeaders.forEach(compressHeader);
  nonColonHeaders.forEach(compressHeader);

  compressor.end();

  var chunk, chunks = [];
  while (chunk = compressor.read()) {
    chunks.push(chunk);
  }
  return concat(chunks);
};


Compressor.prototype._transform = function _transform(frame, encoding, done) {
  
  
  
  
  
  
  
  if (frame.type === 'HEADERS' || frame.type === 'PUSH_PROMISE') {
    var buffer = this.compress(frame.headers);

    
    
    
    var adjustment = frame.type === 'PUSH_PROMISE' ? 4 : 0;
    var chunks = cut(buffer, MAX_HTTP_PAYLOAD_SIZE - adjustment);

    for (var i = 0; i < chunks.length; i++) {
      var chunkFrame;
      var first = (i === 0);
      var last = (i === chunks.length - 1);

      if (first) {
        chunkFrame = util._extend({}, frame);
        chunkFrame.flags = util._extend({}, frame.flags);
        chunkFrame.flags['END_' + frame.type] = last;
      } else {
        chunkFrame = {
          type: 'CONTINUATION',
          flags: { END_HEADERS: last },
          stream: frame.stream
        };
      }
      chunkFrame.data = chunks[i];

      this.push(chunkFrame);
    }
  }

  
  else {
    this.push(frame);
  }

  done();
};










util.inherits(Decompressor, TransformStream);
function Decompressor(log, type) {
  TransformStream.call(this, { objectMode: true });

  this._log = log.child({ component: 'compressor' });

  assert((type === 'REQUEST') || (type === 'RESPONSE'));
  this._table = new HeaderTable(this._log);

  this._inProgress = false;
  this._base = undefined;
}


Decompressor.prototype.setTableSizeLimit = function setTableSizeLimit(size) {
  this._table.setSizeLimit(size);
};




Decompressor.prototype.decompress = function decompress(block) {
  var decompressor = new HeaderSetDecompressor(this._log, this._table);
  decompressor.end(block);

  var seenNonColonHeader = false;
  var headers = {};
  var pair;
  while (pair = decompressor.read()) {
    var name = pair[0];
    var value = pair[1];
    var isColonHeader = (name.trim()[0] === ':');
    if (seenNonColonHeader && isColonHeader) {
        this.emit('error', 'PROTOCOL_ERROR');
        return headers;
    }
    seenNonColonHeader = !isColonHeader;
    if (name in headers) {
      if (headers[name] instanceof Array) {
        headers[name].push(value);
      } else {
        headers[name] = [headers[name], value];
      }
    } else {
      headers[name] = value;
    }
  }

  
  
  
  if (('cookie' in headers) && (headers['cookie'] instanceof Array)) {
    headers['cookie'] = headers['cookie'].join('; ');
  }

  return headers;
};


Decompressor.prototype._transform = function _transform(frame, encoding, done) {
  
  
  if (this._inProgress) {
    if ((frame.type !== 'CONTINUATION') || (frame.stream !== this._base.stream)) {
      this._log.error('A series of HEADER frames were not continuous');
      this.emit('error', 'PROTOCOL_ERROR');
      return;
    }
    this._frames.push(frame);
  }

  
  
  else if ((frame.type === 'HEADERS') || (frame.type === 'PUSH_PROMISE')) {
    this._inProgress = true;
    this._base = util._extend({}, frame);
    this._frames = [frame];
  }

  
  else {
    this.push(frame);
  }

  
  
  
  if (this._inProgress && (frame.flags.END_HEADERS || frame.flags.END_PUSH_PROMISE)) {
    var buffer = concat(this._frames.map(function(frame) {
      return frame.data;
    }));
    try {
      var headers = this.decompress(buffer);
    } catch(error) {
      this._log.error({ err: error }, 'Header decompression error');
      this.emit('error', 'COMPRESSION_ERROR');
      return;
    }
    this.push(util._extend(this._base, { headers: headers }));
    this._inProgress = false;
  }

  done();
};





function concat(buffers) {
  var size = 0;
  for (var i = 0; i < buffers.length; i++) {
    size += buffers[i].length;
  }

  var concatenated = new Buffer(size);
  for (var cursor = 0, j = 0; j < buffers.length; cursor += buffers[j].length, j++) {
    buffers[j].copy(concatenated, cursor);
  }

  return concatenated;
}


function cut(buffer, size) {
  var chunks = [];
  var cursor = 0;
  do {
    var chunkSize = Math.min(size, buffer.length - cursor);
    chunks.push(buffer.slice(cursor, cursor + chunkSize));
    cursor += chunkSize;
  } while(cursor < buffer.length);
  return chunks;
}

function trim(string) {
  return string.trim();
}
