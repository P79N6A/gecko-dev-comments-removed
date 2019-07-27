



var assert = require('assert');

var Transform = require('stream').Transform;

exports.Serializer = Serializer;
exports.Deserializer = Deserializer;

var logData = Boolean(process.env.HTTP2_LOG_DATA);

var MAX_PAYLOAD_SIZE = 16384;
var WINDOW_UPDATE_PAYLOAD_SIZE = 4;












function Serializer(log) {
  this._log = log.child({ component: 'serializer' });
  Transform.call(this, { objectMode: true });
}
Serializer.prototype = Object.create(Transform.prototype, { constructor: { value: Serializer } });




Serializer.prototype._transform = function _transform(frame, encoding, done) {
  this._log.trace({ frame: frame }, 'Outgoing frame');

  assert(frame.type in Serializer, 'Unknown frame type: ' + frame.type);

  var buffers = [];
  Serializer[frame.type](frame, buffers);
  var length = Serializer.commonHeader(frame, buffers);

  assert(length <= MAX_PAYLOAD_SIZE, 'Frame too large!');

  for (var i = 0; i < buffers.length; i++) {
    if (logData) {
      this._log.trace({ data: buffers[i] }, 'Outgoing data');
    }
    this.push(buffers[i]);
  }

  done();
};












function Deserializer(log, role) {
  this._role = role;
  this._log = log.child({ component: 'deserializer' });
  Transform.call(this, { objectMode: true });
  this._next(COMMON_HEADER_SIZE);
}
Deserializer.prototype = Object.create(Transform.prototype, { constructor: { value: Deserializer } });






Deserializer.prototype._next = function(size) {
  this._cursor = 0;
  this._buffer = new Buffer(size);
  this._waitingForHeader = !this._waitingForHeader;
  if (this._waitingForHeader) {
    this._frame = {};
  }
};



Deserializer.prototype._transform = function _transform(chunk, encoding, done) {
  var cursor = 0;

  if (logData) {
    this._log.trace({ data: chunk }, 'Incoming data');
  }

  while(cursor < chunk.length) {
    
    
    var toCopy = Math.min(chunk.length - cursor, this._buffer.length - this._cursor);
    chunk.copy(this._buffer, this._cursor, cursor, cursor + toCopy);
    this._cursor += toCopy;
    cursor += toCopy;

    
    

    
    
    if ((this._cursor === this._buffer.length) && this._waitingForHeader) {
      var payloadSize = Deserializer.commonHeader(this._buffer, this._frame);
      if (payloadSize <= MAX_PAYLOAD_SIZE) {
        this._next(payloadSize);
      } else {
        this.emit('error', 'FRAME_SIZE_ERROR');
        return;
      }
    }

    
    
    
    
    
    if ((this._cursor === this._buffer.length) && !this._waitingForHeader) {
      if (this._frame.type) {
        var error = Deserializer[this._frame.type](this._buffer, this._frame, this._role);
        if (error) {
          this._log.error('Incoming frame parsing error: ' + error);
          this.emit('error', error);
        } else {
          this._log.trace({ frame: this._frame }, 'Incoming frame');
          this.push(this._frame);
        }
      } else {
        this._log.error('Unknown type incoming frame');
        
      }
      this._next(COMMON_HEADER_SIZE);
    }
  }

  done();
};
















































var COMMON_HEADER_SIZE = 9;

var frameTypes = [];

var frameFlags = {};

var genericAttributes = ['type', 'flags', 'stream'];

var typeSpecificAttributes = {};

Serializer.commonHeader = function writeCommonHeader(frame, buffers) {
  var headerBuffer = new Buffer(COMMON_HEADER_SIZE);

  var size = 0;
  for (var i = 0; i < buffers.length; i++) {
    size += buffers[i].length;
  }
  headerBuffer.writeUInt8(0, 0);
  headerBuffer.writeUInt16BE(size, 1);

  var typeId = frameTypes.indexOf(frame.type);  
  headerBuffer.writeUInt8(typeId, 3);

  var flagByte = 0;
  for (var flag in frame.flags) {
    var position = frameFlags[frame.type].indexOf(flag);
    assert(position !== -1, 'Unknown flag for frame type ' + frame.type + ': ' + flag);
    if (frame.flags[flag]) {
      flagByte |= (1 << position);
    }
  }
  headerBuffer.writeUInt8(flagByte, 4);

  assert((0 <= frame.stream) && (frame.stream < 0x7fffffff), frame.stream);
  headerBuffer.writeUInt32BE(frame.stream || 0, 5);

  buffers.unshift(headerBuffer);

  return size;
};

Deserializer.commonHeader = function readCommonHeader(buffer, frame) {
  var totallyWastedByte = buffer.readUInt8(0);
  var length = buffer.readUInt16BE(1);
  
  
  length += totallyWastedByte << 16;

  frame.type = frameTypes[buffer.readUInt8(3)];
  if (!frame.type) {
    
    return length;
  }

  frame.flags = {};
  var flagByte = buffer.readUInt8(4);
  var definedFlags = frameFlags[frame.type];
  for (var i = 0; i < definedFlags.length; i++) {
    frame.flags[definedFlags[i]] = Boolean(flagByte & (1 << i));
  }

  frame.stream = buffer.readUInt32BE(5) & 0x7fffffff;

  return length;
};

























frameTypes[0x0] = 'DATA';

frameFlags.DATA = ['END_STREAM', 'RESERVED2', 'RESERVED4', 'PADDED'];

typeSpecificAttributes.DATA = ['data'];

Serializer.DATA = function writeData(frame, buffers) {
  buffers.push(frame.data);
};

Deserializer.DATA = function readData(buffer, frame) {
  var dataOffset = 0;
  var paddingLength = 0;
  if (frame.flags.PADDED) {
    paddingLength = (buffer.readUInt8(dataOffset) & 0xff);
    dataOffset = 1;
  }

  if (paddingLength) {
    frame.data = buffer.slice(dataOffset, -1 * paddingLength);
  } else {
    frame.data = buffer.slice(dataOffset);
  }
};




















frameTypes[0x1] = 'HEADERS';

frameFlags.HEADERS = ['END_STREAM', 'RESERVED2', 'END_HEADERS', 'PADDED', 'RESERVED5', 'PRIORITY'];

typeSpecificAttributes.HEADERS = ['priorityDependency', 'priorityWeight', 'exclusiveDependency', 'headers', 'data'];

















Serializer.HEADERS = function writeHeadersPriority(frame, buffers) {
  if (frame.flags.PRIORITY) {
    var buffer = new Buffer(5);
    assert((0 <= frame.priorityDependency) && (frame.priorityDependency <= 0x7fffffff), frame.priorityDependency);
    buffer.writeUInt32BE(frame.priorityDependency, 0);
    if (frame.exclusiveDependency) {
      buffer[0] |= 0x80;
    }
    assert((0 <= frame.priorityWeight) && (frame.priorityWeight <= 0xff), frame.priorityWeight);
    buffer.writeUInt8(frame.priorityWeight, 4);
    buffers.push(buffer);
  }
  buffers.push(frame.data);
};

Deserializer.HEADERS = function readHeadersPriority(buffer, frame) {
  var dataOffset = 0;
  var paddingLength = 0;
  if (frame.flags.PADDED) {
    paddingLength = (buffer.readUInt8(dataOffset) & 0xff);
    dataOffset = 1;
  }

  if (frame.flags.PRIORITY) {
    var dependencyData = new Buffer(4);
    buffer.copy(dependencyData, 0, dataOffset, dataOffset + 4);
    dataOffset += 4;
    frame.exclusiveDependency = !!(dependencyData[0] & 0x80);
    dependencyData[0] &= 0x7f;
    frame.priorityDependency = dependencyData.readUInt32BE(0);
    frame.priorityWeight = buffer.readUInt8(dataOffset);
    dataOffset += 1;
  }

  if (paddingLength) {
    frame.data = buffer.slice(dataOffset, -1 * paddingLength);
  } else {
    frame.data = buffer.slice(dataOffset);
  }
};








frameTypes[0x2] = 'PRIORITY';

frameFlags.PRIORITY = [];

typeSpecificAttributes.PRIORITY = ['priorityDependency', 'priorityWeight', 'exclusiveDependency'];











Serializer.PRIORITY = function writePriority(frame, buffers) {
  var buffer = new Buffer(5);
  assert((0 <= frame.priorityDependency) && (frame.priorityDependency <= 0x7fffffff), frame.priorityDependency);
  buffer.writeUInt32BE(frame.priorityDependency, 0);
  if (frame.exclusiveDependency) {
    buffer[0] |= 0x80;
  }
  assert((0 <= frame.priorityWeight) && (frame.priorityWeight <= 0xff), frame.priorityWeight);
  buffer.writeUInt8(frame.priorityWeight, 4);

  buffers.push(buffer);
};

Deserializer.PRIORITY = function readPriority(buffer, frame) {
  var dependencyData = new Buffer(4);
  buffer.copy(dependencyData, 0, 0, 4);
  frame.exclusiveDependency = !!(dependencyData[0] & 0x80);
  dependencyData[0] &= 0x7f;
  frame.priorityDependency = dependencyData.readUInt32BE(0);
  frame.priorityWeight = buffer.readUInt8(4);
};








frameTypes[0x3] = 'RST_STREAM';

frameFlags.RST_STREAM = [];

typeSpecificAttributes.RST_STREAM = ['error'];










Serializer.RST_STREAM = function writeRstStream(frame, buffers) {
  var buffer = new Buffer(4);
  var code = errorCodes.indexOf(frame.error);
  assert((0 <= code) && (code <= 0xffffffff), code);
  buffer.writeUInt32BE(code, 0);
  buffers.push(buffer);
};

Deserializer.RST_STREAM = function readRstStream(buffer, frame) {
  frame.error = errorCodes[buffer.readUInt32BE(0)];
  if (!frame.error) {
    
    frame.error = 'INTERNAL_ERROR';
  }
};












frameTypes[0x4] = 'SETTINGS';

frameFlags.SETTINGS = ['ACK'];

typeSpecificAttributes.SETTINGS = ['settings'];



















Serializer.SETTINGS = function writeSettings(frame, buffers) {
  var settings = [], settingsLeft = Object.keys(frame.settings);
  definedSettings.forEach(function(setting, id) {
    if (setting.name in frame.settings) {
      settingsLeft.splice(settingsLeft.indexOf(setting.name), 1);
      var value = frame.settings[setting.name];
      settings.push({ id: id, value: setting.flag ? Boolean(value) : value });
    }
  });
  assert(settingsLeft.length === 0, 'Unknown settings: ' + settingsLeft.join(', '));

  var buffer = new Buffer(settings.length * 6);
  for (var i = 0; i < settings.length; i++) {
    buffer.writeUInt16BE(settings[i].id & 0xffff, i*6);
    buffer.writeUInt32BE(settings[i].value, i*6 + 2);
  }

  buffers.push(buffer);
};

Deserializer.SETTINGS = function readSettings(buffer, frame, role) {
  frame.settings = {};

  
  
  
  if(frame.flags.ACK && buffer.length != 0) {
    return 'FRAME_SIZE_ERROR';
  }

  if (buffer.length % 6 !== 0) {
    return 'PROTOCOL_ERROR';
  }
  for (var i = 0; i < buffer.length / 6; i++) {
    var id = buffer.readUInt16BE(i*6) & 0xffff;
    var setting = definedSettings[id];
    if (setting) {
      if (role == 'CLIENT' && setting.name == 'SETTINGS_ENABLE_PUSH') {
        return 'SETTINGS frame on client got SETTINGS_ENABLE_PUSH';
      }
      var value = buffer.readUInt32BE(i*6 + 2);
      frame.settings[setting.name] = setting.flag ? Boolean(value & 0x1) : value;
    }
  }
};


var definedSettings = [];




definedSettings[1] = { name: 'SETTINGS_HEADER_TABLE_SIZE', flag: false };





definedSettings[2] = { name: 'SETTINGS_ENABLE_PUSH', flag: true };



definedSettings[3] = { name: 'SETTINGS_MAX_CONCURRENT_STREAMS', flag: false };



definedSettings[4] = { name: 'SETTINGS_INITIAL_WINDOW_SIZE', flag: false };



definedSettings[5] = { name: 'SETTINGS_MAX_FRAME_SIZE', flag: false };













frameTypes[0x5] = 'PUSH_PROMISE';

frameFlags.PUSH_PROMISE = ['RESERVED1', 'RESERVED2', 'END_PUSH_PROMISE', 'PADDED'];

typeSpecificAttributes.PUSH_PROMISE = ['promised_stream', 'headers', 'data'];

















Serializer.PUSH_PROMISE = function writePushPromise(frame, buffers) {
  var buffer = new Buffer(4);

  var promised_stream = frame.promised_stream;
  assert((0 <= promised_stream) && (promised_stream <= 0x7fffffff), promised_stream);
  buffer.writeUInt32BE(promised_stream, 0);

  buffers.push(buffer);
  buffers.push(frame.data);
};

Deserializer.PUSH_PROMISE = function readPushPromise(buffer, frame) {
  var dataOffset = 0;
  var paddingLength = 0;
  if (frame.flags.PADDED) {
    paddingLength = (buffer.readUInt8(dataOffset) & 0xff);
    dataOffset = 1;
  }
  frame.promised_stream = buffer.readUInt32BE(dataOffset) & 0x7fffffff;
  dataOffset += 4;
  if (paddingLength) {
    frame.data = buffer.slice(dataOffset, -1 * paddingLength);
  } else {
    frame.data = buffer.slice(dataOffset);
  }
};












frameTypes[0x6] = 'PING';

frameFlags.PING = ['ACK'];

typeSpecificAttributes.PING = ['data'];



Serializer.PING = function writePing(frame, buffers) {
  buffers.push(frame.data);
};

Deserializer.PING = function readPing(buffer, frame) {
  if (buffer.length !== 8) {
    return 'FRAME_SIZE_ERROR';
  }
  frame.data = buffer;
};








frameTypes[0x7] = 'GOAWAY';

frameFlags.GOAWAY = [];

typeSpecificAttributes.GOAWAY = ['last_stream', 'error'];
















Serializer.GOAWAY = function writeGoaway(frame, buffers) {
  var buffer = new Buffer(8);

  var last_stream = frame.last_stream;
  assert((0 <= last_stream) && (last_stream <= 0x7fffffff), last_stream);
  buffer.writeUInt32BE(last_stream, 0);

  var code = errorCodes.indexOf(frame.error);
  assert((0 <= code) && (code <= 0xffffffff), code);
  buffer.writeUInt32BE(code, 4);

  buffers.push(buffer);
};

Deserializer.GOAWAY = function readGoaway(buffer, frame) {
  frame.last_stream = buffer.readUInt32BE(0) & 0x7fffffff;
  frame.error = errorCodes[buffer.readUInt32BE(4)];
  if (!frame.error) {
    
    frame.error = 'INTERNAL_ERROR';
  }
};








frameTypes[0x8] = 'WINDOW_UPDATE';

frameFlags.WINDOW_UPDATE = [];

typeSpecificAttributes.WINDOW_UPDATE = ['window_size'];






Serializer.WINDOW_UPDATE = function writeWindowUpdate(frame, buffers) {
  var buffer = new Buffer(4);

  var window_size = frame.window_size;
  assert((0 < window_size) && (window_size <= 0x7fffffff), window_size);
  buffer.writeUInt32BE(window_size, 0);

  buffers.push(buffer);
};

Deserializer.WINDOW_UPDATE = function readWindowUpdate(buffer, frame) {
  if (buffer.length !== WINDOW_UPDATE_PAYLOAD_SIZE) {
    return 'FRAME_SIZE_ERROR';
  }
  frame.window_size = buffer.readUInt32BE(0) & 0x7fffffff;
  if (frame.window_size === 0) {
    return 'PROTOCOL_ERROR';
  }
};












frameTypes[0x9] = 'CONTINUATION';

frameFlags.CONTINUATION = ['RESERVED1', 'RESERVED2', 'END_HEADERS'];

typeSpecificAttributes.CONTINUATION = ['headers', 'data'];

Serializer.CONTINUATION = function writeContinuation(frame, buffers) {
  buffers.push(frame.data);
};

Deserializer.CONTINUATION = function readContinuation(buffer, frame) {
  frame.data = buffer;
};








frameTypes[0xA] = 'ALTSVC';

frameFlags.ALTSVC = [];















































typeSpecificAttributes.ALTSVC = ['maxAge', 'port', 'protocolID', 'host',
                                 'origin'];

Serializer.ALTSVC = function writeAltSvc(frame, buffers) {
  var buffer = new Buffer(8);
  buffer.writeUInt32BE(frame.maxAge, 0);
  buffer.writeUInt16BE(frame.port, 4);
  buffer.writeUInt8(0, 6);
  buffer.writeUInt8(frame.protocolID.length, 7);
  buffers.push(buffer);

  buffers.push(new Buffer(frame.protocolID, 'ascii'));

  buffer = new Buffer(1);
  buffer.writeUInt8(frame.host.length, 0);
  buffers.push(buffer);

  buffers.push(new Buffer(frame.host, 'ascii'));

  buffers.push(new Buffer(frame.origin, 'ascii'));
};

Deserializer.ALTSVC = function readAltSvc(buffer, frame) {
  frame.maxAge = buffer.readUInt32BE(0);
  frame.port = buffer.readUInt16BE(4);
  var pidLength = buffer.readUInt8(7);
  frame.protocolID = buffer.toString('ascii', 8, 8 + pidLength);
  var hostLength = buffer.readUInt8(8 + pidLength);
  frame.host = buffer.toString('ascii', 9 + pidLength, 9 + pidLength + hostLength);
  frame.origin = buffer.toString('ascii', 9 + pidLength + hostLength);
};









frameTypes[0xB] = 'BLOCKED';

frameFlags.BLOCKED = [];

typeSpecificAttributes.BLOCKED = [];

Serializer.BLOCKED = function writeBlocked(frame, buffers) {
};

Deserializer.BLOCKED = function readBlocked(buffer, frame) {
};




var errorCodes = [
  'NO_ERROR',
  'PROTOCOL_ERROR',
  'INTERNAL_ERROR',
  'FLOW_CONTROL_ERROR',
  'SETTINGS_TIMEOUT',
  'STREAM_CLOSED',
  'FRAME_SIZE_ERROR',
  'REFUSED_STREAM',
  'CANCEL',
  'COMPRESSION_ERROR',
  'CONNECT_ERROR',
  'ENHANCE_YOUR_CALM',
  'INADEQUATE_SECURITY',
  'HTTP_1_1_REQUIRED'
];






exports.serializers = {};



var frameCounter = 0;
exports.serializers.frame = function(frame) {
  if (!frame) {
    return null;
  }

  if ('id' in frame) {
    return frame.id;
  }

  frame.id = frameCounter;
  frameCounter += 1;

  var logEntry = { id: frame.id };
  genericAttributes.concat(typeSpecificAttributes[frame.type]).forEach(function(name) {
    logEntry[name] = frame[name];
  });

  if (frame.data instanceof Buffer) {
    if (logEntry.data.length > 50) {
      logEntry.data = frame.data.slice(0, 47).toString('hex') + '...';
    } else {
      logEntry.data = frame.data.toString('hex');
    }

    if (!('length' in logEntry)) {
      logEntry.length = frame.data.length;
    }
  }

  if (frame.promised_stream instanceof Object) {
    logEntry.promised_stream = 'stream-' + frame.promised_stream.id;
  }

  logEntry.flags = Object.keys(frame.flags || {}).filter(function(name) {
    return frame.flags[name] === true;
  });

  return logEntry;
};


exports.serializers.data = function(data) {
  return data.toString('hex');
};
