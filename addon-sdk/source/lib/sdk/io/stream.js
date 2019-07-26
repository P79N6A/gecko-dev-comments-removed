



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { EventTarget } = require("../event/target");
const { emit } = require("../event/core");
const { Buffer } = require("./buffer");
const { Class } = require("../core/heritage");
const { setTimeout } = require("../timers");
const { ns } = require("../core/namespace");

function isFunction(value) typeof value === "function"

function accessor() {
  let map = new WeakMap();
  return function(fd, value) {
    if (value === null) map.delete(fd);
    if (value !== undefined) map.set(fd, value);
    return map.get(fd);
  }
}

let nsIInputStreamPump = accessor();
let nsIAsyncOutputStream = accessor();
let nsIInputStream = accessor();
let nsIOutputStream = accessor();





function isClosed(stream) {
  
  let isClosed = false;
  stream.asyncWait({
    
    
    
    onOutputStreamReady: function onClose() isClosed = true

  
  
  
  }, stream.WAIT_CLOSURE_ONLY, 0, null);
  return isClosed;
}










function onStateChange(stream, target) {
  let isAsync = false;
  stream.asyncWait({
    onOutputStreamReady: function onOutputStreamReady() {
      
      
      
      if (!isAsync)
        return setTimeout(onOutputStreamReady, 0);

      
      
      
      emit(target, isClosed(stream) ? "close" : "drain");
    }
  }, 0, 0, null);
  isAsync = true;
}

function pump(stream) {
  let input = nsIInputStream(stream);
  nsIInputStreamPump(stream).asyncRead({
    onStartRequest: function onStartRequest() {
      emit(stream, "start");
    },
    onDataAvailable: function onDataAvailable(req, c, is, offset, count) {
      try {
        let bytes = input.readByteArray(count);
        emit(stream, "data", new Buffer(bytes, stream.encoding));
      } catch (error) {
        emit(stream, "error", error);
        stream.readable = false;
      }
    },
    onStopRequest: function onStopRequest() {
      stream.readable = false;
      emit(stream, "end");
    }
  }, null);
}

const Stream = Class({
  extends: EventTarget,
  initialize: function() {
    this.readable = false;
    this.writable = false;
    this.encoding = null;
  },
  setEncoding: function setEncoding(encoding) {
    this.encoding = String(encoding).toUpperCase();
  },
  pipe: function pipe(target, options) {
    let source = this;
    function onData(chunk) {
      if (target.writable) {
        if (false === target.write(chunk))
          source.pause();
      }
    }
    function onDrain() {
      if (source.readable) source.resume();
    }
    function onEnd() {
      target.end();
    }
    function onPause() {
      source.pause();
    }
    function onResume() {
      if (source.readable)
        source.resume();
    }

    function cleanup() {
      source.removeListener("data", onData);
      target.removeListener("drain", onDrain);
      source.removeListener("end", onEnd);

      target.removeListener("pause", onPause);
      target.removeListener("resume", onResume);

      source.removeListener("end", cleanup);
      source.removeListener("close", cleanup);

      target.removeListener("end", cleanup);
      target.removeListener("close", cleanup);
    }

    if (!options || options.end !== false)
      target.on("end", onEnd);

    source.on("data", onData);
    target.on("drain", onDrain);
    target.on("resume", onResume);
    target.on("pause", onPause);

    source.on("end", cleanup);
    source.on("close", cleanup);

    target.on("end", cleanup);
    target.on("close", cleanup);

    emit(target, "pipe", source);
  },
  pause: function pause() {
    emit(this, "pause");
  },
  resume: function resume() {
    emit(this, "resume");
  },
  destroySoon: function destroySoon() {
    this.destroy();
  }
});
exports.Stream = Stream;

const InputStream = Class({
  extends: Stream,
  initialize: function initialize(options) {
    let { input, pump } = options;

    this.readable = true;
    this.paused = false;
    nsIInputStream(this, input);
    nsIInputStreamPump(this, pump);
  },
  get status() nsIInputStreamPump(this).status,
  read: function() pump(this),
  pause: function pause() {
    this.paused = true;
    nsIInputStreamPump(this).suspend();
    emit(this, "paused");
  },
  resume: function resume() {
    this.paused = false;
    nsIInputStreamPump(this).resume();
    emit(this, "resume");
  },
  destroy: function destroy() {
    this.readable = false;
    try {
      emit(this, "close", null);
      nsIInputStreamPump(this).cancel(null);
      nsIInputStreamPump(this, null);

      nsIInputStream(this).close();
      nsIInputStream(this, null);
    } catch (error) {
      emit(this, "error", error);
    }
  }
});
exports.InputStream = InputStream;

const OutputStream = Class({
  extends: Stream,
  initialize: function initialize(options) {
    let { output, asyncOutputStream } = options;

    this.writable = true;
    nsIOutputStream(this, output);
    nsIAsyncOutputStream(this, asyncOutputStream);
  },
  write: function write(content, encoding, callback) {
    let output = nsIOutputStream(this);
    let asyncOutputStream = nsIAsyncOutputStream(this);

    if (isFunction(encoding)) {
      callback = encoding;
      encoding = callback;
    }

    
    
    let isWritten = false;
    
    if (!this.writable)
      throw Error("stream not writable");

    try {
      
      if (!Buffer.isBuffer(content))
        content = new Buffer(content, encoding);

      
      
      output.writeByteArray(content.valueOf(), content.length);
      output.flush();

      if (callback) this.once("drain", callback);
      onStateChange(asyncOutputStream, this);
      return true;
    } catch (error) {
      
      emit(this, "error", error);
    }
  },
  flush: function flush() {
    nsIOutputStream(this).flush();
  },
  end: function end(content, encoding, callback) {
    if (isFunction(content)) {
      callback = content
      content = callback
    }
    if (isFunction(encoding)) {
      callback = encoding
      encoding = callback
    }

    
    if (isFunction(callback))
      this.once("close", callback);

    
    if (content)
      this.write(content, encoding, end.bind(this));
    
    else
      nsIOutputStream(this).close();
  },
  destroy: function destroy(callback) {
    try {
      this.end(callback);
      nsIOutputStream(this, null);
      nsIAsyncOutputStream(this, null);
    } catch (error) {
      emit(this, "error", error);
    }
  }
});
exports.OutputStream = OutputStream;

const DuplexStream = Class({
  extends: Stream,
  initialize: function initialize(options) {
    let { input, output, pump } = options;

    this.writable = true;
    this.readable = true;
    this.encoding = null;

    nsIInputStream(this, input);
    nsIOutputStream(this, output);
    nsIInputStreamPump(this, pump);
  },
  read: InputStream.prototype.read,
  pause: InputStream.prototype.pause,
  resume: InputStream.prototype.resume,

  write: OutputStream.prototype.write,
  flush: OutputStream.prototype.flush,
  end: OutputStream.prototype.end,

  destroy: function destroy(error) {
    if (error)
      emit(this, "error", error);
    InputStream.prototype.destroy.call(this);
    OutputStream.prototype.destroy.call(this);
  }
});
exports.DuplexStream = DuplexStream;
