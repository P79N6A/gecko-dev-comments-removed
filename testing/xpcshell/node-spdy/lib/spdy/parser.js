var parser = exports;

var spdy = require('../spdy'),
    util = require('util'),
    stream = require('stream'),
    Buffer = require('buffer').Buffer;








function Parser(connection, deflate, inflate) {
  stream.Stream.call(this);

  this.drained = true;
  this.paused = false;
  this.buffer = [];
  this.buffered = 0;
  this.waiting = 8;

  this.state = { type: 'frame-head' };
  this.deflate = deflate;
  this.inflate = inflate;
  this.framer = null;

  this.connection = connection;

  this.readable = this.writable = true;
}
util.inherits(Parser, stream.Stream);








parser.create = function create(connection, deflate, inflate) {
  return new Parser(connection, deflate, inflate);
};






Parser.prototype.write = function write(data) {
  if (data !== undefined) {
    
    this.buffer.push(data);
    this.buffered += data.length;
  }

  
  if (this.paused) return false;

  
  if (this.buffered < this.waiting) return;

  
  if (data !== undefined) this.drained = false;

  var self = this,
      buffer = new Buffer(this.waiting),
      sliced = 0,
      offset = 0;

  while (this.waiting > offset && sliced < this.buffer.length) {
    var chunk = this.buffer[sliced++],
        overmatched = false;

    
    if (chunk.length > this.waiting - offset) {
      chunk.copy(buffer, offset, 0, this.waiting - offset);

      this.buffer[--sliced] = chunk.slice(this.waiting - offset);
      this.buffered += this.buffer[sliced].length;

      overmatched = true;
    } else {
      chunk.copy(buffer, offset);
    }

    
    offset += chunk.length;
    this.buffered -= chunk.length;

    if (overmatched) break;
  }

  
  this.buffer = this.buffer.slice(sliced);

  
  this.paused = true;
  this.execute(this.state, buffer, function (err, waiting) {
    
    self.paused = false;

    
    if (err) return self.emit('error', err);

    
    self.waiting = waiting;

    if (self.waiting <= self.buffered) {
      self.write();
    } else {
      process.nextTick(function() {
        if (self.drained) return;

        
        self.drained = true;
        self.emit('drain');
      });
    }
  });
};





Parser.prototype.end = function end() {
  this.emit('end');
};








Parser.prototype.execute = function execute(state, data, callback) {
  if (state.type === 'frame-head') {
    var header = state.header = spdy.protocol.generic.parseHeader(data);

    
    if (!this.framer && header.control) {
      if (spdy.protocol[header.version]) {
        this.framer = new spdy.protocol[header.version].Framer(
          spdy.utils.zwrap(this.deflate),
          spdy.utils.zwrap(this.inflate)
        );

        
        this.connection.framer = this.framer;
        this.emit('_framer', this.framer);
      }
    }

    state.type = 'frame-body';
    callback(null, header.length);
  } else if (state.type === 'frame-body') {
    var self = this;

    
    if (!state.header.control) {
      return onFrame(null, {
        type: 'DATA',
        id: state.header.id,
        fin: (state.header.flags & 0x01) === 0x01,
        compressed: (state.header.flags & 0x02) === 0x02,
        data: data
      });
    } else {
    
      this.framer.execute(state.header, data, onFrame);
    }

    function onFrame(err, frame) {
      if (err) return callback(err);

      self.emit('frame', frame);

      state.type = 'frame-head';
      callback(null, 8);
    };
  }
};
