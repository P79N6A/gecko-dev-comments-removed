var assert = require('assert');

var Serializer   = require('./framer').Serializer;
var Deserializer = require('./framer').Deserializer;
var Compressor   = require('./compressor').Compressor;
var Decompressor = require('./compressor').Decompressor;
var Connection   = require('./connection').Connection;
var Duplex       = require('stream').Duplex;
var Transform    = require('stream').Transform;

exports.Endpoint = Endpoint;






































function Endpoint(log, role, settings, filters) {
  Duplex.call(this);

  
  this._log = log.child({ component: 'endpoint', e: this });

  
  
  assert((role === 'CLIENT') || role === 'SERVER');
  if (role === 'CLIENT') {
    this._writePrelude();
  } else {
    this._readPrelude();
  }

  
  
  
  this._initializeDataFlow(role, settings, filters || {});

  
  this._initializeManagement();

  
  this._initializeErrorHandling();
}
Endpoint.prototype = Object.create(Duplex.prototype, { constructor: { value: Endpoint } });




var CLIENT_PRELUDE = new Buffer('PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n');


Endpoint.prototype._writePrelude = function _writePrelude() {
  this._log.debug('Sending the client connection header prelude.');
  this.push(CLIENT_PRELUDE);
};


Endpoint.prototype._readPrelude = function _readPrelude() {
  
  var cursor = 0;

  
  this._write = function _temporalWrite(chunk, encoding, done) {
    
    
    var offset = cursor;
    while(cursor < CLIENT_PRELUDE.length && (cursor - offset) < chunk.length) {
      if (CLIENT_PRELUDE[cursor] !== chunk[cursor - offset]) {
        this._log.fatal({ cursor: cursor, offset: offset, chunk: chunk },
                        'Client connection header prelude does not match.');
        this._error('handshake', 'PROTOCOL_ERROR');
        return;
      }
      cursor += 1;
    }

    
    
    if (cursor === CLIENT_PRELUDE.length) {
      this._log.debug('Successfully received the client connection header prelude.');
      delete this._write;
      chunk = chunk.slice(cursor - offset);
      this._write(chunk, encoding, done);
    }
  };
};


































function createTransformStream(filter) {
  var transform = new Transform({ objectMode: true });
  var push = transform.push.bind(transform);
  transform._transform = function(frame, encoding, done) {
    filter(frame, push, done);
  };
  return transform;
}

function pipeAndFilter(stream1, stream2, filter) {
  if (filter) {
    stream1.pipe(createTransformStream(filter)).pipe(stream2);
  } else {
    stream1.pipe(stream2);
  }
}

Endpoint.prototype._initializeDataFlow = function _initializeDataFlow(role, settings, filters) {
  var firstStreamId, compressorRole, decompressorRole;
  if (role === 'CLIENT') {
    firstStreamId = 1;
    compressorRole = 'REQUEST';
    decompressorRole = 'RESPONSE';
  } else {
    firstStreamId = 2;
    compressorRole = 'RESPONSE';
    decompressorRole = 'REQUEST';
  }

  this._serializer   = new Serializer(this._log);
  this._deserializer = new Deserializer(this._log);
  this._compressor   = new Compressor(this._log, compressorRole);
  this._decompressor = new Decompressor(this._log, decompressorRole);
  this._connection   = new Connection(this._log, firstStreamId, settings);

  pipeAndFilter(this._connection, this._compressor, filters.beforeCompression);
  pipeAndFilter(this._compressor, this._serializer, filters.beforeSerialization);
  pipeAndFilter(this._deserializer, this._decompressor, filters.afterDeserialization);
  pipeAndFilter(this._decompressor, this._connection, filters.afterDecompression);

  this._connection.on('ACKNOWLEDGED_SETTINGS_HEADER_TABLE_SIZE',
                      this._decompressor.setTableSizeLimit.bind(this._decompressor));
  this._connection.on('RECEIVING_SETTINGS_HEADER_TABLE_SIZE',
                      this._compressor.setTableSizeLimit.bind(this._compressor));
};

var noread = {};
Endpoint.prototype._read = function _read() {
  this._readableState.sync = true;
  var moreNeeded = noread, chunk;
  while (moreNeeded && (chunk = this._serializer.read())) {
    moreNeeded = this.push(chunk);
  }
  if (moreNeeded === noread) {
    this._serializer.once('readable', this._read.bind(this));
  }
  this._readableState.sync = false;
};

Endpoint.prototype._write = function _write(chunk, encoding, done) {
  this._deserializer.write(chunk, encoding, done);
};




Endpoint.prototype._initializeManagement = function _initializeManagement() {
  this._connection.on('stream', this.emit.bind(this, 'stream'));
};

Endpoint.prototype.createStream = function createStream() {
  return this._connection.createStream();
};




Endpoint.prototype._initializeErrorHandling = function _initializeErrorHandling() {
  this._serializer.on('error', this._error.bind(this, 'serializer'));
  this._deserializer.on('error', this._error.bind(this, 'deserializer'));
  this._compressor.on('error', this._error.bind(this, 'compressor'));
  this._decompressor.on('error', this._error.bind(this, 'decompressor'));
  this._connection.on('error', this._error.bind(this, 'connection'));

  this._connection.on('peerError', this.emit.bind(this, 'peerError'));
};

Endpoint.prototype._error = function _error(component, error) {
  this._log.fatal({ source: component, message: error }, 'Fatal error, closing connection');
  this.close(error);
  setImmediate(this.emit.bind(this, 'error', error));
};

Endpoint.prototype.close = function close(error) {
  this._connection.close(error);
};




exports.serializers = {};

var nextId = 0;
exports.serializers.e = function(endpoint) {
  if (!('id' in endpoint)) {
    endpoint.id = nextId;
    nextId += 1;
  }
  return endpoint.id;
};
