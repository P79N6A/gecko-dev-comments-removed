var spdy = require('../../spdy');
var util = require('util');
var Buffer = require('buffer').Buffer;
var EventEmitter = require('events').EventEmitter;
var constants = require('./').constants;

function Framer() {
  EventEmitter.call(this);

  this.version = null;
  this.deflate = null;
  this.inflate = null;
  this.debug = false;
};
util.inherits(Framer, EventEmitter);
module.exports = Framer;

Framer.create = function create(version, deflate, inflate) {
  return new Framer(version, deflate, inflate);
};







Framer.prototype.setCompression = function setCompresion(deflate, inflate) {
  this.deflate = spdy.utils.zwrap(deflate);
  this.inflate = spdy.utils.zwrap(inflate);
};








Framer.prototype.setVersion = function setVersion(version) {
  this.version = version;
  this.emit('version');
};




Framer.prototype.headersToDict = function headersToDict(headers, preprocess) {
  function stringify(value) {
    if (value !== undefined) {
      if (Array.isArray(value)) {
        return value.join('\x00');
      } else if (typeof value === 'string') {
        return value;
      } else {
        return value.toString();
      }
    } else {
      return '';
    }
  }

  
  var loweredHeaders = {};
  Object.keys(headers || {}).map(function(key) {
    loweredHeaders[key.toLowerCase()] = headers[key];
  });

  
  if (preprocess)
    preprocess(loweredHeaders);

  
  var size = this.version === 2 ? 2 : 4,
      len = size,
      pairs = Object.keys(loweredHeaders).filter(function(key) {
        var lkey = key.toLowerCase();
        return lkey !== 'connection' && lkey !== 'keep-alive' &&
               lkey !== 'proxy-connection' && lkey !== 'transfer-encoding';
      }).map(function(key) {
        var klen = Buffer.byteLength(key),
            value = stringify(loweredHeaders[key]),
            vlen = Buffer.byteLength(value);

        len += size * 2 + klen + vlen;
        return [klen, key, vlen, value];
      }),
      result = new Buffer(len);

  if (this.version === 2)
    result.writeUInt16BE(pairs.length, 0, true);
  else
    result.writeUInt32BE(pairs.length, 0, true);

  var offset = size;
  pairs.forEach(function(pair) {
    
    if (this.version === 2)
      result.writeUInt16BE(pair[0], offset, true);
    else
      result.writeUInt32BE(pair[0], offset, true);
    
    result.write(pair[1], offset + size);

    offset += pair[0] + size;

    
    if (this.version === 2)
      result.writeUInt16BE(pair[2], offset, true);
    else
      result.writeUInt32BE(pair[2], offset, true);
    
    result.write(pair[3], offset + size);

    offset += pair[2] + size;
  }, this);

  return result;
};

Framer.prototype._synFrame = function _synFrame(type,
                                                id,
                                                assoc,
                                                priority,
                                                dict,
                                                callback) {
  var self = this;

  
  this.deflate(dict, function (err, chunks, size) {
    if (err)
      return callback(err);

    var offset = type === 'SYN_STREAM' ? 18 : self.version === 2 ? 14 : 12,
        total = offset - 8 + size,
        frame = new Buffer(offset + size);

    
    frame.writeUInt16BE(0x8000 | self.version, 0, true);
    
    frame.writeUInt16BE(type === 'SYN_STREAM' ? 1 : 2, 2, true);
    
    frame.writeUInt32BE(total & 0x00ffffff, 4, true);
    
    frame.writeUInt32BE(id & 0x7fffffff, 8, true);

    if (type === 'SYN_STREAM') {
      
      if (assoc !== 0)
        frame[4] = 2;

      
      frame.writeUInt32BE(assoc & 0x7fffffff, 12, true);

      
      var priorityValue;
      if (self.version === 2)
        priorityValue = Math.max(Math.min(priority, 3), 0) << 6;
      else
        priorityValue = Math.max(Math.min(priority, 7), 0) << 5;
      frame.writeUInt8(priorityValue, 16, true);
    }

    for (var i = 0; i < chunks.length; i++) {
      chunks[i].copy(frame, offset);
      offset += chunks[i].length;
    }

    callback(null, frame);
  });
};










Framer.prototype.replyFrame = function replyFrame(id,
                                                  code,
                                                  reason,
                                                  headers,
                                                  callback) {
  if (!this.version) {
    return this.on('version', function() {
      this.replyFrame(id, code, reason, headers, callback);
    });
  }

  var self = this;
  var dict = this.headersToDict(headers, function(headers) {
    if (self.version === 2) {
      headers.status = code + ' ' + reason;
      headers.version = 'HTTP/1.1';
    } else {
      headers[':status'] = code + ' ' + reason;
      headers[':version'] = 'HTTP/1.1';
    }
  });


  this._synFrame('SYN_REPLY', id, null, 0, dict, callback);
};











Framer.prototype.streamFrame = function streamFrame(id,
                                                    assoc,
                                                    meta,
                                                    headers,
                                                    callback) {
  if (!this.version) {
    return this.on('version', function() {
      this.streamFrame(id, assoc, meta, headers, callback);
    });
  }

  var self = this;
  var dict = this.headersToDict(headers, function(headers) {
    if (self.version === 2) {
      if (meta.status)
        headers.status = meta.status;
      headers.version = meta.version || 'HTTP/1.1';
      headers.url = meta.url;
      if (meta.method)
        headers.method = meta.method;
    } else {
      if (meta.status)
        headers[':status'] = meta.status;
      headers[':version'] = meta.version || 'HTTP/1.1';
      headers[':path'] = meta.path || meta.url;
      headers[':scheme'] = meta.scheme || 'https';
      headers[':host'] = meta.host;
      if (meta.method)
        headers[':method'] = meta.method;
    }
  });

  this._synFrame('SYN_STREAM', id, assoc, meta.priority, dict, callback);
};








Framer.prototype.headersFrame = function headersFrame(id, headers, callback) {
  if (!this.version) {
    return this.on('version', function() {
      this.headersFrame(id, headers, callback);
    });
  }

  var self = this;
  var dict = this.headersToDict(headers, function(headers) {});

  this.deflate(dict, function (err, chunks, size) {
    if (err)
      return callback(err);

    var offset = self.version === 2 ? 14 : 12,
        total = offset - 8 + size,
        frame = new Buffer(offset + size);

    
    frame.writeUInt16BE(0x8000 | self.version, 0, true);
    
    frame.writeUInt16BE(8, 2, true);
    
    frame.writeUInt32BE(total & 0x00ffffff, 4, true);
    
    frame.writeUInt32BE(id & 0x7fffffff, 8, true);

    
    for (var i = 0; i < chunks.length; i++) {
      chunks[i].copy(frame, offset);
      offset += chunks[i].length;
    }

    callback(null, frame);
  });
};









Framer.prototype.dataFrame = function dataFrame(id, fin, data, callback) {
  if (!this.version) {
    return this.on('version', function() {
      this.dataFrame(id, fin, data, callback);
    });
  }

  if (!fin && !data.length)
    return callback(null, []);

  var frame = new Buffer(8 + data.length);

  frame.writeUInt32BE(id & 0x7fffffff, 0, true);
  frame.writeUInt32BE(data.length & 0x00ffffff, 4, true);
  frame.writeUInt8(fin ? 0x01 : 0x0, 4, true);

  if (data.length)
    data.copy(frame, 8);

  return callback(null, frame);
};






Framer.prototype.pingFrame = function pingFrame(id, callback) {
  if (!this.version) {
    return this.on('version', function() {
      this.pingFrame(id, callback);
    });
  }

  var header = new Buffer(12);

  
  header.writeUInt32BE(0x80000006 | (this.version << 16), 0, true);
  
  header.writeUInt32BE(0x00000004, 4, true);
  
  header.writeUInt32BE(id, 8, true);

  return callback(null, header);
};









Framer.prototype.rstFrame = function rstFrame(id, code, extra, callback) {
  if (!this.version) {
    return this.on('version', function() {
      this.rstFrame(id, code, extra, callback);
    });
  }

  var header = new Buffer(16 +
                          (this.debug ? Buffer.byteLength(extra || '') : 0));

  
  header.writeUInt32BE(0x80000003 | (this.version << 16), 0, true);
  
  header.writeUInt32BE(0x00000008, 4, true);
  
  header.writeUInt32BE(id & 0x7fffffff, 8, true);
  
  header.writeUInt32BE(code, 12, true);

  
  if (this.debug && extra)
    header.write(extra, 16);

  return callback(null, header);
};







Framer.prototype.settingsFrame = function settingsFrame(options, callback) {
  if (!this.version) {
    return this.on('version', function() {
      this.settingsFrame(options, callback);
    });
  }

  var settings,
      key = this.version === 2 ? '2/' + options.maxStreams :
                                 '3/' + options.maxStreams + ':' +
                                     options.windowSize;

  if (!(settings = Framer.settingsCache[key])) {
    var params = [];
    if (isFinite(options.maxStreams)) {
      params.push({
        key: constants.settings.SETTINGS_MAX_CONCURRENT_STREAMS,
        value: options.maxStreams
      });
    }
    if (this.version > 2) {
      params.push({
        key: constants.settings.SETTINGS_INITIAL_WINDOW_SIZE,
        value: options.windowSize
      });
    }

    settings = new Buffer(12 + 8 * params.length);

    
    settings.writeUInt32BE(0x80000004 | (this.version << 16), 0, true);
    
    settings.writeUInt32BE((4 + 8 * params.length) & 0x00FFFFFF, 4, true);
    
    settings.writeUInt32BE(params.length, 8, true);

    var offset = 12;
    params.forEach(function(param) {
      var flag = constants.settings.FLAG_SETTINGS_PERSIST_VALUE << 24;

      if (this.version === 2)
        settings.writeUInt32LE(flag | param.key, offset, true);
      else
        settings.writeUInt32BE(flag | param.key, offset, true);
      offset += 4;
      settings.writeUInt32BE(param.value & 0x7fffffff, offset, true);
      offset += 4;
    }, this);

    Framer.settingsCache[key] = settings;
  }

  return callback(null, settings);
};
Framer.settingsCache = {};






Framer.prototype.windowUpdateFrame = function windowUpdateFrame(id, delta, cb) {
  if (!this.version) {
    return this.on('version', function() {
      this.windowUpdateFrame(id, delta, cb);
    });
  }

  var header = new Buffer(16);

  
  header.writeUInt32BE(0x80000009 | (this.version << 16), 0, true);
  
  header.writeUInt32BE(0x00000008, 4, true);
  
  header.writeUInt32BE(id & 0x7fffffff, 8, true);
  
  if (delta > 0)
    header.writeUInt32BE(delta & 0x7fffffff, 12, true);
  else
    header.writeUInt32BE(delta, 12, true);

  return cb(null, header);
};

Framer.prototype.goawayFrame = function goawayFrame(lastId, status, cb) {
  if (!this.version) {
    return this.on('version', function() {
      this.goawayFrame(lastId, status, cb);
    });
  }

  var header = new Buffer(16);

  
  header.writeUInt32BE(0x80000007 | (this.version << 16), 0, true);
  
  header.writeUInt32BE(0x00000008, 4, true);
  
  header.writeUInt32BE(lastId & 0x7fffffff, 8, true);
  
  header.writeUInt32BE(status, 12, true);

  return cb(null, header);
};
