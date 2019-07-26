

















(function (global) {
  if (global.DataView)
    return;
  if (!global.ArrayBuffer)
    fail('ArrayBuffer not supported');
  if (!Object.defineProperties)
    fail('This module requires ECMAScript 5');
  var nativele = new Int8Array(new Int32Array([
      1
    ]).buffer)[0] === 1;
  var temp = new Uint8Array(8);
  global.DataView = function DataView(buffer, offset, length) {
    if (!(buffer instanceof ArrayBuffer))
      fail('Bad ArrayBuffer');
    offset = offset || 0;
    length = length || buffer.byteLength - offset;
    if (offset < 0 || length < 0 || offset + length > buffer.byteLength)
      fail('Illegal offset and/or length');
    Object.defineProperties(this, {
      buffer: {
        value: buffer,
        enumerable: false,
        writable: false,
        configurable: false
      },
      byteOffset: {
        value: offset,
        enumerable: false,
        writable: false,
        configurable: false
      },
      byteLength: {
        value: length,
        enumerable: false,
        writable: false,
        configurable: false
      },
      _bytes: {
        value: new Uint8Array(buffer, offset, length),
        enumerable: false,
        writable: false,
        configurable: false
      }
    });
  };
  global.DataView.prototype = {
    constructor: DataView,
    getInt8: function getInt8(offset) {
      return get(this, Int8Array, 1, offset);
    },
    getUint8: function getUint8(offset) {
      return get(this, Uint8Array, 1, offset);
    },
    getInt16: function getInt16(offset, le) {
      return get(this, Int16Array, 2, offset, le);
    },
    getUint16: function getUint16(offset, le) {
      return get(this, Uint16Array, 2, offset, le);
    },
    getInt32: function getInt32(offset, le) {
      return get(this, Int32Array, 4, offset, le);
    },
    getUint32: function getUint32(offset, le) {
      return get(this, Uint32Array, 4, offset, le);
    },
    getFloat32: function getFloat32(offset, le) {
      return get(this, Float32Array, 4, offset, le);
    },
    getFloat64: function getFloat32(offset, le) {
      return get(this, Float64Array, 8, offset, le);
    },
    setInt8: function setInt8(offset, value) {
      set(this, Int8Array, 1, offset, value);
    },
    setUint8: function setUint8(offset, value) {
      set(this, Uint8Array, 1, offset, value);
    },
    setInt16: function setInt16(offset, value, le) {
      set(this, Int16Array, 2, offset, value, le);
    },
    setUint16: function setUint16(offset, value, le) {
      set(this, Uint16Array, 2, offset, value, le);
    },
    setInt32: function setInt32(offset, value, le) {
      set(this, Int32Array, 4, offset, value, le);
    },
    setUint32: function setUint32(offset, value, le) {
      set(this, Uint32Array, 4, offset, value, le);
    },
    setFloat32: function setFloat32(offset, value, le) {
      set(this, Float32Array, 4, offset, value, le);
    },
    setFloat64: function setFloat64(offset, value, le) {
      set(this, Float64Array, 8, offset, value, le);
    }
  };
  function get(view, type, size, offset, le) {
    if (offset === undefined)
      fail('Missing required offset argument');
    if (offset < 0 || offset + size > view.byteLength)
      fail('Invalid index: ' + offset);
    if (size === 1 || !(!le) === nativele) {
      if ((view.byteOffset + offset) % size === 0)
        return new type(view.buffer, view.byteOffset + offset, 1)[0];
      else {
        for (var i = 0; i < size; i++)
          temp[i] = view._bytes[offset + i];
        return new type(temp.buffer)[0];
      }
    } else {
      for (var i = 0; i < size; i++)
        temp[size - i - 1] = view._bytes[offset + i];
      return new type(temp.buffer)[0];
    }
  }
  function set(view, type, size, offset, value, le) {
    if (offset === undefined)
      fail('Missing required offset argument');
    if (value === undefined)
      fail('Missing required value argument');
    if (offset < 0 || offset + size > view.byteLength)
      fail('Invalid index: ' + offset);
    if (size === 1 || !(!le) === nativele) {
      if ((view.byteOffset + offset) % size === 0) {
        new type(view.buffer, view.byteOffset + offset, 1)[0] = value;
      } else {
        new type(temp.buffer)[0] = value;
        for (var i = 0; i < size; i++)
          view._bytes[i + offset] = temp[i];
      }
    } else {
      new type(temp.buffer)[0] = value;
      for (var i = 0; i < size; i++)
        view._bytes[offset + i] = temp[size - 1 - i];
    }
  }
  function fail(msg) {
    throw new Error(msg);
  }
}(this));
function scriptProperties(namespace, props) {
  return props.reduce(function (o, p) {
    o[p] = namespace + ' ' + p;
    return o;
  }, {});
}
function cloneObject(obj) {
  var clone = Object.create(null);
  for (var prop in obj)
    clone[prop] = obj[prop];
  return clone;
}
function throwError(name, error) {
  var message = formatErrorMessage.apply(null, slice.call(arguments, 1));
  throwErrorFromVM(AVM2.currentDomain(), name, message, error.code);
}
function sortByDepth(a, b) {
  var levelA = a._level;
  var levelB = b._level;
  if (a._parent !== b._parent && a._index > -1 && b._index > -1) {
    while (a._level > levelB) {
      a = a._parent;
    }
    while (b._level > levelA) {
      b = b._parent;
    }
    while (a._level > 1) {
      if (a._parent === b._parent) {
        break;
      }
      a = a._parent;
      b = b._parent;
    }
  }
  if (a === b) {
    return levelA - levelB;
  }
  return a._index - b._index;
}
function sortNumeric(a, b) {
  return a - b;
}
var Promise = function PromiseClosure() {
    function isPromise(obj) {
      return typeof obj === 'object' && obj !== null && typeof obj.then === 'function';
    }
    function defaultOnFulfilled(value) {
      return value;
    }
    function defaultOnRejected(reason) {
      throw reason;
    }
    function propagateFulfilled(subject, value) {
      subject.subpromisesValue = value;
      var subpromises = subject.subpromises;
      if (!subpromises) {
        return;
      }
      for (var i = 0; i < subpromises.length; i++) {
        subpromises[i].fulfill(value);
      }
      delete subject.subpromises;
    }
    function propagateRejected(subject, reason) {
      subject.subpromisesReason = reason;
      var subpromises = subject.subpromises;
      if (!subpromises) {
        return;
      }
      for (var i = 0; i < subpromises.length; i++) {
        subpromises[i].reject(reason);
      }
      delete subject.subpromises;
    }
    function performCall(callback, arg, subject) {
      try {
        var value = callback(arg);
        if (isPromise(value)) {
          value.then(function Promise_queueCall_onFulfilled(value) {
            propagateFulfilled(subject, value);
          }, function Promise_queueCall_onRejected(reason) {
            propagateRejected(subject, reason);
          });
          return;
        }
        propagateFulfilled(subject, value);
      } catch (ex) {
        propagateRejected(subject, ex);
      }
    }
    var queue = [];
    function processQueue() {
      while (queue.length > 0) {
        var task = queue[0];
        if (task.directCallback) {
          task.callback.call(task.subject, task.arg);
        } else {
          performCall(task.callback, task.arg, task.subject);
        }
        queue.shift();
      }
    }
    function queueCall(callback, arg, subject, directCallback) {
      if (queue.length === 0) {
        setTimeout(processQueue, 0);
      }
      queue.push({
        callback: callback,
        arg: arg,
        subject: subject,
        directCallback: directCallback
      });
    }
    function Promise(onFulfilled, onRejected) {
      this.state = 'pending';
      this.onFulfilled = typeof onFulfilled === 'function' ? onFulfilled : defaultOnFulfilled;
      this.onRejected = typeof onRejected === 'function' ? onRejected : defaultOnRejected;
    }
    Promise.prototype = {
      fulfill: function Promise_resolve(value) {
        if (this.state !== 'pending') {
          return;
        }
        this.state = 'fulfilled';
        this.value = value;
        queueCall(this.onFulfilled, value, this, false);
      },
      reject: function Promise_reject(reason) {
        if (this.state !== 'pending') {
          return;
        }
        this.state = 'rejected';
        this.reason = reason;
        queueCall(this.onRejected, reason, this, false);
      },
      then: function Promise_then(onFulfilled, onRejected) {
        var promise = new Promise(onFulfilled, onRejected);
        if ('subpromisesValue' in this) {
          queueCall(promise.fulfill, this.subpromisesValue, promise, true);
        } else if ('subpromisesReason' in this) {
          queueCall(promise.reject, this.subpromisesReason, promise, true);
        } else {
          var subpromises = this.subpromises || (this.subpromises = []);
          subpromises.push(promise);
        }
        return promise;
      },
      get resolved() {
        return this.state === 'fulfilled';
      },
      resolve: function (value) {
        this.fulfill(value);
      }
    };
    Promise.when = function Promise_when() {
      var promise = new Promise();
      if (arguments.length === 0) {
        promise.resolve();
        return promise;
      }
      var promises = slice.call(arguments, 0);
      var result = [];
      var i = 1;
      function fulfill(value) {
        result.push(value);
        if (i < promises.length) {
          promises[i++].then(fulfill, reject);
        } else {
          promise.resolve(result);
        }
        return value;
      }
      function reject(reason) {
        promise.reject(reason);
      }
      promises[0].then(fulfill, reject);
      return promise;
    };
    return Promise;
  }();
var QuadTree = function (x, y, width, height, level) {
  this.x = x | 0;
  this.y = y | 0;
  this.width = width | 0;
  this.height = height | 0;
  this.level = level | 0;
  this.stuckObjects = [];
  this.objects = [];
  this.nodes = [];
};
QuadTree.prototype._findIndex = function (xMin, yMin, xMax, yMax) {
  var midX = this.x + (this.width / 2 | 0);
  var midY = this.y + (this.height / 2 | 0);
  var top = yMin < midY && yMax < midY;
  var bottom = yMin > midY;
  if (xMin < midX && xMax < midX) {
    if (top) {
      return 1;
    } else if (bottom) {
      return 2;
    }
  } else if (xMin > midX) {
    if (top) {
      return 0;
    } else if (bottom) {
      return 3;
    }
  }
  return -1;
};
QuadTree.prototype.insert = function (obj) {
  var nodes = this.nodes;
  if (nodes.length) {
    var index = this._findIndex(obj.xMin, obj.yMin, obj.xMax, obj.yMax);
    if (index > -1) {
      nodes[index].insert(obj);
    } else {
      this.stuckObjects.push(obj);
      obj._qtree = this;
    }
    return;
  }
  var objects = this.objects;
  objects.push(obj);
  if (objects.length > 4 && this.level < 10) {
    this._subdivide();
    while (objects.length) {
      this.insert(objects.shift());
    }
    return;
  }
  obj._qtree = this;
};
QuadTree.prototype.delete = function (obj) {
  if (obj._qtree !== this) {
    return;
  }
  var index = this.objects.indexOf(obj);
  if (index > -1) {
    this.objects.splice(index, 1);
  } else {
    index = this.stuckObjects.indexOf(obj);
    this.stuckObjects.splice(index, 1);
  }
  obj._qtree = null;
};
QuadTree.prototype._stack = [];
QuadTree.prototype._out = [];
QuadTree.prototype.retrieve = function (xMin, yMin, xMax, yMax) {
  var stack = this._stack;
  var out = this._out;
  out.length = 0;
  var node = this;
  do {
    if (node.nodes.length) {
      var index = node._findIndex(xMin, yMin, xMax, yMax);
      if (index > -1) {
        stack.push(node.nodes[index]);
      } else {
        stack.push.apply(stack, node.nodes);
      }
    }
    out.push.apply(out, node.stuckObjects);
    out.push.apply(out, node.objects);
    node = stack.pop();
  } while (node);
  return out;
};
QuadTree.prototype._subdivide = function () {
  var halfWidth = this.width / 2 | 0;
  var halfHeight = this.height / 2 | 0;
  var midX = this.x + halfWidth;
  var midY = this.y + halfHeight;
  var level = this.level + 1;
  this.nodes[0] = new QuadTree(midX, this.y, halfWidth, halfHeight, level);
  this.nodes[1] = new QuadTree(this.x, this.y, halfWidth, halfHeight, level);
  this.nodes[2] = new QuadTree(this.x, midY, halfWidth, halfHeight, level);
  this.nodes[3] = new QuadTree(midX, midY, halfWidth, halfHeight, level);
};
var create = Object.create;
var defineProperty = Object.defineProperty;
var keys = Object.keys;
var isArray = Array.isArray;
var fromCharCode = String.fromCharCode;
var logE = Math.log;
var max = Math.max;
var min = Math.min;
var pow = Math.pow;
var push = Array.prototype.push;
var slice = Array.prototype.slice;
var splice = Array.prototype.splice;
function fail(msg, context) {
  throw new Error((context ? context + ': ' : '') + msg);
}
function assert(cond, msg, context) {
  if (!cond)
    fail(msg, context);
}
function rgbaObjToStr(color) {
  return 'rgba(' + color.red + ',' + color.green + ',' + color.blue + ',' + color.alpha / 255 + ')';
}
function rgbIntAlphaToStr(color, alpha) {
  color |= 0;
  if (alpha >= 1) {
    var colorStr = color.toString(16);
    while (colorStr.length < 6) {
      colorStr = '0' + colorStr;
    }
    return '#' + colorStr;
  }
  var red = color >> 16 & 255;
  var green = color >> 8 & 255;
  var blue = color & 255;
  return 'rgba(' + red + ',' + green + ',' + blue + ',' + alpha + ')';
}
function argbUintToStr(argb) {
  return 'rgba(' + (argb >>> 16 & 255) + ',' + (argb >>> 8 & 255) + ',' + (argb & 255) + ',' + (argb >>> 24 & 255) / 255 + ')';
}
(function functionNameSupport() {
  if (eval('function t() {} t.name === \'t\'')) {
    return;
  }
  Object.defineProperty(Function.prototype, 'name', {
    get: function () {
      if (this.__name) {
        return this.__name;
      }
      var m = /function\s([^\(]+)/.exec(this.toString());
      var name = m && m[1] !== 'anonymous' ? m[1] : null;
      this.__name = name;
      return name;
    },
    configurable: true,
    enumerable: false
  });
}());
var randomStyleCache;
function randomStyle() {
  if (!randomStyleCache) {
    randomStyleCache = [];
    for (var i = 0; i < 50; i++) {
      randomStyleCache.push('#' + ('00000' + (Math.random() * (1 << 24) | 0).toString(16)).slice(-6));
    }
  }
  return randomStyleCache[Math.random() * randomStyleCache.length | 0];
}
var SWF_TAG_CODE_CSM_TEXT_SETTINGS = 74;
var SWF_TAG_CODE_DEFINE_BINARY_DATA = 87;
var SWF_TAG_CODE_DEFINE_BITS = 6;
var SWF_TAG_CODE_DEFINE_BITS_JPEG2 = 21;
var SWF_TAG_CODE_DEFINE_BITS_JPEG3 = 35;
var SWF_TAG_CODE_DEFINE_BITS_JPEG4 = 90;
var SWF_TAG_CODE_DEFINE_BITS_LOSSLESS = 20;
var SWF_TAG_CODE_DEFINE_BITS_LOSSLESS2 = 36;
var SWF_TAG_CODE_DEFINE_BUTTON = 7;
var SWF_TAG_CODE_DEFINE_BUTTON2 = 34;
var SWF_TAG_CODE_DEFINE_BUTTON_CXFORM = 23;
var SWF_TAG_CODE_DEFINE_BUTTON_SOUND = 17;
var SWF_TAG_CODE_DEFINE_EDIT_TEXT = 37;
var SWF_TAG_CODE_DEFINE_FONT = 10;
var SWF_TAG_CODE_DEFINE_FONT2 = 48;
var SWF_TAG_CODE_DEFINE_FONT3 = 75;
var SWF_TAG_CODE_DEFINE_FONT4 = 91;
var SWF_TAG_CODE_DEFINE_FONT_ALIGN_ZONES = 73;
var SWF_TAG_CODE_DEFINE_FONT_INFO = 13;
var SWF_TAG_CODE_DEFINE_FONT_INFO2 = 62;
var SWF_TAG_CODE_DEFINE_FONT_NAME = 88;
var SWF_TAG_CODE_DEFINE_MORPH_SHAPE = 46;
var SWF_TAG_CODE_DEFINE_MORPH_SHAPE2 = 84;
var SWF_TAG_CODE_DEFINE_SCALING_GRID = 78;
var SWF_TAG_CODE_DEFINE_SCENE_AND_FRAME_LABEL_DATA = 86;
var SWF_TAG_CODE_DEFINE_SHAPE = 2;
var SWF_TAG_CODE_DEFINE_SHAPE2 = 22;
var SWF_TAG_CODE_DEFINE_SHAPE3 = 32;
var SWF_TAG_CODE_DEFINE_SHAPE4 = 83;
var SWF_TAG_CODE_DEFINE_SOUND = 14;
var SWF_TAG_CODE_DEFINE_SPRITE = 39;
var SWF_TAG_CODE_DEFINE_TEXT = 11;
var SWF_TAG_CODE_DEFINE_TEXT2 = 33;
var SWF_TAG_CODE_DEFINE_VIDEO_STREAM = 60;
var SWF_TAG_CODE_DO_ABC = 82;
var SWF_TAG_CODE_DO_ABC_ = 72;
var SWF_TAG_CODE_DO_ACTION = 12;
var SWF_TAG_CODE_DO_INIT_ACTION = 59;
var SWF_TAG_CODE_ENABLE_DEBUGGER = 58;
var SWF_TAG_CODE_ENABLE_DEBUGGER2 = 64;
var SWF_TAG_CODE_END = 0;
var SWF_TAG_CODE_EXPORT_ASSETS = 56;
var SWF_TAG_CODE_FILE_ATTRIBUTES = 69;
var SWF_TAG_CODE_FRAME_LABEL = 43;
var SWF_TAG_CODE_IMPORT_ASSETS = 57;
var SWF_TAG_CODE_IMPORT_ASSETS2 = 71;
var SWF_TAG_CODE_JPEG_TABLES = 8;
var SWF_TAG_CODE_METADATA = 77;
var SWF_TAG_CODE_PLACE_OBJECT = 4;
var SWF_TAG_CODE_PLACE_OBJECT2 = 26;
var SWF_TAG_CODE_PLACE_OBJECT3 = 70;
var SWF_TAG_CODE_PROTECT = 24;
var SWF_TAG_CODE_REMOVE_OBJECT = 5;
var SWF_TAG_CODE_REMOVE_OBJECT2 = 28;
var SWF_TAG_CODE_SCRIPT_LIMITS = 65;
var SWF_TAG_CODE_SET_BACKGROUND_COLOR = 9;
var SWF_TAG_CODE_SET_TAB_INDEX = 66;
var SWF_TAG_CODE_SHOW_FRAME = 1;
var SWF_TAG_CODE_SOUND_STREAM_BLOCK = 19;
var SWF_TAG_CODE_SOUND_STREAM_HEAD = 18;
var SWF_TAG_CODE_SOUND_STREAM_HEAD2 = 45;
var SWF_TAG_CODE_START_SOUND = 15;
var SWF_TAG_CODE_START_SOUND2 = 89;
var SWF_TAG_CODE_SYMBOL_CLASS = 76;
var SWF_TAG_CODE_VIDEO_FRAME = 61;
self.SWF = {};
var FORMAT_COLORMAPPED = 3;
var FORMAT_15BPP = 4;
var FORMAT_24BPP = 5;
var FACTOR_5BBP = 255 / 31;
var crcTable = [];
for (var i = 0; i < 256; i++) {
  var c = i;
  for (var h = 0; h < 8; h++) {
    if (c & 1)
      c = 3988292384 ^ c >> 1 & 2147483647;
    else
      c = c >> 1 & 2147483647;
  }
  crcTable[i] = c;
}
function crc32(data, start, end) {
  var crc = -1;
  for (var i = start; i < end; i++) {
    var a = (crc ^ data[i]) & 255;
    var b = crcTable[a];
    crc = crc >>> 8 ^ b;
  }
  return crc ^ -1;
}
function createPngChunk(type, data) {
  var chunk = new Uint8Array(12 + data.length);
  var p = 0;
  var len = data.length;
  chunk[p] = len >> 24 & 255;
  chunk[p + 1] = len >> 16 & 255;
  chunk[p + 2] = len >> 8 & 255;
  chunk[p + 3] = len & 255;
  chunk[p + 4] = type.charCodeAt(0) & 255;
  chunk[p + 5] = type.charCodeAt(1) & 255;
  chunk[p + 6] = type.charCodeAt(2) & 255;
  chunk[p + 7] = type.charCodeAt(3) & 255;
  if (data instanceof Uint8Array)
    chunk.set(data, 8);
  p = 8 + len;
  var crc = crc32(chunk, 4, p);
  chunk[p] = crc >> 24 & 255;
  chunk[p + 1] = crc >> 16 & 255;
  chunk[p + 2] = crc >> 8 & 255;
  chunk[p + 3] = crc & 255;
  return chunk;
}
function adler32(data, start, end) {
  var a = 1;
  var b = 0;
  for (var i = start; i < end; ++i) {
    a = (a + (data[i] & 255)) % 65521;
    b = (b + a) % 65521;
  }
  return b << 16 | a;
}
function defineBitmap(tag) {
  var width = tag.width;
  var height = tag.height;
  var hasAlpha = tag.hasAlpha;
  var plte = '';
  var trns = '';
  var literals;
  var bmpData = tag.bmpData;
  switch (tag.format) {
  case FORMAT_COLORMAPPED:
    var colorType = 3;
    var bytesPerLine = width + 3 & ~3;
    var colorTableSize = tag.colorTableSize + 1;
    var paletteSize = colorTableSize * (tag.hasAlpha ? 4 : 3);
    var datalen = paletteSize + bytesPerLine * height;
    var stream = createInflatedStream(bmpData, datalen);
    var bytes = stream.bytes;
    var pos = 0;
    stream.ensure(paletteSize);
    if (hasAlpha) {
      var palette = new Uint8Array(paletteSize / 4 * 3);
      var pp = 0;
      var alphaValues = new Uint8Array(paletteSize / 4);
      var pa = 0;
      while (pos < paletteSize) {
        palette[pp++] = bytes[pos];
        palette[pp++] = bytes[pos + 1];
        palette[pp++] = bytes[pos + 2];
        alphaValues[pa++] = bytes[pos + 3];
        pos += 4;
      }
      plte = createPngChunk('PLTE', palette);
      trns = createPngChunk('tRNS', alphaValues);
    } else {
      plte = createPngChunk('PLTE', bytes.subarray(pos, pos + paletteSize));
      pos += paletteSize;
    }
    literals = new Uint8Array(width * height + height);
    var pl = 0;
    while (pos < datalen) {
      stream.ensure(bytesPerLine);
      var begin = pos;
      var end = begin + width;
      pl++;
      literals.set(bytes.subarray(begin, end), pl);
      pl += end - begin;
      stream.pos = pos += bytesPerLine;
    }
    break;
  case FORMAT_15BPP:
    var colorType = 2;
    var bytesPerLine = width * 2 + 3 & ~3;
    var stream = createInflatedStream(bmpData, bytesPerLine * height);
    var pos = 0;
    literals = new Uint8Array(width * height * 3 + height);
    var pl = 0;
    for (var y = 0; y < height; ++y) {
      pl++;
      stream.ensure(bytesPerLine);
      for (var x = 0; x < width; ++x) {
        var word = stream.getUint16(pos);
        pos += 2;
        literals[pl++] = 0 | FACTOR_5BBP * (word >> 10 & 31);
        literals[pl++] = 0 | FACTOR_5BBP * (word >> 5 & 31);
        literals[pl++] = 0 | FACTOR_5BBP * (word & 31);
      }
      stream.pos = pos += bytesPerLine;
    }
    break;
  case FORMAT_24BPP:
    var padding;
    if (hasAlpha) {
      var colorType = 6;
      padding = 0;
      literals = new Uint8Array(width * height * 4 + height);
    } else {
      var colorType = 2;
      padding = 1;
      literals = new Uint8Array(width * height * 3 + height);
    }
    var bytesPerLine = width * 4;
    var stream = createInflatedStream(bmpData, bytesPerLine * height);
    var bytes = stream.bytes;
    var pos = 0;
    var pl = 0;
    for (var y = 0; y < height; ++y) {
      stream.ensure(bytesPerLine);
      pl++;
      for (var x = 0; x < width; ++x) {
        pos += padding;
        if (hasAlpha) {
          var alpha = bytes[pos];
          if (alpha) {
            var opacity = alpha / 255;
            literals[pl++] = 0 | bytes[pos + 1] / opacity;
            literals[pl++] = 0 | bytes[pos + 2] / opacity;
            literals[pl++] = 0 | bytes[pos + 3] / opacity;
            literals[pl++] = alpha;
          } else {
            pl += 4;
          }
        } else {
          literals[pl++] = bytes[pos];
          literals[pl++] = bytes[pos + 1];
          literals[pl++] = bytes[pos + 2];
        }
        pos += 4 - padding;
      }
      stream.pos = pos;
    }
    break;
  default:
    fail('invalid format', 'bitmap');
  }
  var ihdr = new Uint8Array([
      width >> 24 & 255,
      width >> 16 & 255,
      width >> 8 & 255,
      width & 255,
      height >> 24 & 255,
      height >> 16 & 255,
      height >> 8 & 255,
      height & 255,
      8,
      colorType,
      0,
      0,
      0
    ]);
  var len = literals.length;
  var maxBlockLength = 65535;
  var idat = new Uint8Array(2 + len + Math.ceil(len / maxBlockLength) * 5 + 4);
  var pi = 0;
  idat[pi++] = 120;
  idat[pi++] = 156;
  var pos = 0;
  while (len > maxBlockLength) {
    idat[pi++] = 0;
    idat[pi++] = 255;
    idat[pi++] = 255;
    idat[pi++] = 0;
    idat[pi++] = 0;
    idat.set(literals.subarray(pos, pos + maxBlockLength), pi);
    pi += maxBlockLength;
    pos += maxBlockLength;
    len -= maxBlockLength;
  }
  idat[pi++] = 1;
  idat[pi++] = len & 255;
  idat[pi++] = len >> 8 & 255;
  idat[pi++] = ~len & 65535 & 255;
  idat[pi++] = (~len & 65535) >> 8 & 255;
  idat.set(literals.subarray(pos), pi);
  pi += literals.length - pos;
  var adler = adler32(literals, 0, literals.length);
  idat[pi++] = adler >> 24 & 255;
  idat[pi++] = adler >> 16 & 255;
  idat[pi++] = adler >> 8 & 255;
  idat[pi++] = adler & 255;
  var chunks = [
      new Uint8Array([
        137,
        80,
        78,
        71,
        13,
        10,
        26,
        10
      ]),
      createPngChunk('IHDR', ihdr),
      plte,
      trns,
      createPngChunk('IDAT', idat),
      createPngChunk('IEND', '')
    ];
  return {
    type: 'image',
    id: tag.id,
    width: width,
    height: height,
    mimeType: 'image/png',
    data: new Blob(chunks, {
      type: 'image/png'
    })
  };
}
function defineButton(tag, dictionary) {
  var characters = tag.characters;
  var states = {
      up: {},
      over: {},
      down: {},
      hitTest: {}
    };
  var i = 0, character;
  while (character = characters[i++]) {
    if (character.eob)
      break;
    var characterItem = dictionary[character.symbolId];
    var entry = {
        symbolId: characterItem.id,
        hasMatrix: !(!character.matrix),
        matrix: character.matrix
      };
    if (character.stateUp)
      states.up[character.depth] = entry;
    if (character.stateOver)
      states.over[character.depth] = entry;
    if (character.stateDown)
      states.down[character.depth] = entry;
    if (character.stateHitTest)
      states.hitTest[character.depth] = entry;
  }
  var button = {
      type: 'button',
      id: tag.id,
      buttonActions: tag.buttonActions,
      states: states
    };
  return button;
}
var nextFontId = 1;
function maxPower2(num) {
  var maxPower = 0;
  var val = num;
  while (val >= 2) {
    val /= 2;
    ++maxPower;
  }
  return pow(2, maxPower);
}
function toString16(val) {
  return fromCharCode(val >> 8 & 255, val & 255);
}
function toString32(val) {
  return toString16(val >> 16) + toString16(val);
}
function defineFont(tag, dictionary) {
  var tables = {};
  var codes = [];
  var glyphIndex = {};
  var ranges = [];
  var glyphs = tag.glyphs;
  var glyphCount = glyphs.length;
  if (tag.codes) {
    codes = codes.concat(tag.codes);
    for (var i = 0, code; code = codes[i]; ++i)
      glyphIndex[code] = i;
    codes.sort(function (a, b) {
      return a - b;
    });
    var i = 0;
    var code;
    while (code = codes[i++]) {
      var start = code;
      var end = start;
      var indices = [
          i - 1
        ];
      while ((code = codes[i]) && end + 1 === code) {
        ++end;
        indices.push(i);
        ++i;
      }
      ranges.push([
        start,
        end,
        indices
      ]);
    }
  } else {
    var indices = [];
    var UAC_OFFSET = 57344;
    for (var i = 0; i < glyphCount; i++) {
      var code = UAC_OFFSET + i;
      codes.push(code);
      glyphIndex[code] = i;
      indices.push(i);
    }
    ranges.push([
      UAC_OFFSET,
      UAC_OFFSET + glyphCount - 1,
      indices
    ]);
  }
  var ascent = Math.ceil(tag.ascent / 20) || 1024;
  var descent = -Math.ceil(tag.descent / 20) || 0;
  var leading = Math.floor(tag.leading / 20) || 0;
  tables['OS/2'] = '\0\x01\0\0' + toString16(tag.bold ? 700 : 400) + '\0\x05' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0\0\0\0\0\0\0\0\0' + '\0\0\0\0' + '\0\0\0\0' + '\0\0\0\0' + '\0\0\0\0' + 'ALF ' + toString16((tag.italic ? 1 : 0) | (tag.bold ? 32 : 0)) + toString16(codes[0]) + toString16(codes[codes.length - 1]) + toString16(ascent) + toString16(descent) + toString16(leading) + toString16(ascent) + toString16(-descent) + '\0\0\0\0' + '\0\0\0\0';
  ;
  var startCount = '';
  var endCount = '';
  var idDelta = '';
  var idRangeOffset = '';
  var i = 0;
  var range;
  while (range = ranges[i++]) {
    var start = range[0];
    var end = range[1];
    var code = range[2][0];
    startCount += toString16(start);
    endCount += toString16(end);
    idDelta += toString16(code - start + 1 & 65535);
    idRangeOffset += toString16(0);
  }
  endCount += '\xff\xff';
  startCount += '\xff\xff';
  idDelta += '\0\x01';
  idRangeOffset += '\0\0';
  var segCount = ranges.length + 1;
  var searchRange = maxPower2(segCount) * 2;
  var rangeShift = 2 * segCount - searchRange;
  var format314 = '\0\0' + toString16(segCount * 2) + toString16(searchRange) + toString16(logE(segCount) / logE(2)) + toString16(rangeShift) + endCount + '\0\0' + startCount + idDelta + idRangeOffset;
  ;
  tables['cmap'] = '\0\0\0\x01\0\x03\0\x01\0\0\0\f\0\x04' + toString16(format314.length + 4) + format314;
  ;
  var glyf = '\0\x01\0\0\0\0\0\0\0\0\0\0\0\x001\0';
  var loca = '\0\0';
  var resolution = tag.resolution || 1;
  var offset = 16;
  var maxPoints = 0;
  var xMins = [];
  var xMaxs = [];
  var yMins = [];
  var yMaxs = [];
  var maxContours = 0;
  var i = 0;
  var code;
  while (code = codes[i++]) {
    var glyph = glyphs[glyphIndex[code]];
    var records = glyph.records;
    var numberOfContours = 1;
    var endPoint = 0;
    var endPtsOfContours = '';
    var flags = '';
    var xCoordinates = '';
    var yCoordinates = '';
    var x = 0;
    var y = 0;
    var xMin = 1024;
    var xMax = -1024;
    var yMin = 1024;
    var yMax = -1024;
    for (var j = 0, record; record = records[j]; ++j) {
      if (record.type) {
        if (record.isStraight) {
          if (record.isGeneral) {
            flags += '\x01';
            var dx = record.deltaX / resolution;
            var dy = -record.deltaY / resolution;
            xCoordinates += toString16(dx);
            yCoordinates += toString16(dy);
            x += dx;
            y += dy;
          } else if (record.isVertical) {
            flags += '\x11';
            var dy = -record.deltaY / resolution;
            yCoordinates += toString16(dy);
            y += dy;
          } else {
            flags += '!';
            var dx = record.deltaX / resolution;
            xCoordinates += toString16(dx);
            x += dx;
          }
        } else {
          flags += '\0';
          var cx = record.controlDeltaX / resolution;
          var cy = -record.controlDeltaY / resolution;
          xCoordinates += toString16(cx);
          yCoordinates += toString16(cy);
          flags += '\x01';
          var dx = record.anchorDeltaX / resolution;
          var dy = -record.anchorDeltaY / resolution;
          xCoordinates += toString16(dx);
          yCoordinates += toString16(dy);
          ++endPoint;
          x += cx + dx;
          y += cy + dy;
        }
        if (x < xMin)
          xMin = x;
        if (x > xMax)
          xMax = x;
        if (y < yMin)
          yMin = y;
        if (y > yMax)
          yMax = y;
        ++endPoint;
      } else {
        if (record.eos)
          break;
        if (record.move) {
          if (endPoint) {
            ++numberOfContours;
            endPtsOfContours += toString16(endPoint - 1);
          }
          flags += '\x01';
          var moveX = record.moveX / resolution;
          var moveY = -record.moveY / resolution;
          var dx = moveX - x;
          var dy = moveY - y;
          xCoordinates += toString16(dx);
          yCoordinates += toString16(dy);
          x = moveX;
          y = moveY;
          if (endPoint > maxPoints)
            maxPoints = endPoint;
          if (x < xMin)
            xMin = x;
          if (x > xMax)
            xMax = x;
          if (y < yMin)
            yMin = y;
          if (y > yMax)
            yMax = y;
          ++endPoint;
        }
      }
    }
    endPtsOfContours += toString16((endPoint || 1) - 1);
    if (!j) {
      xMin = xMax = yMin = yMax = 0;
      flags += '1';
    }
    var entry = toString16(numberOfContours) + toString16(xMin) + toString16(yMin) + toString16(xMax) + toString16(yMax) + endPtsOfContours + '\0\0' + flags + xCoordinates + yCoordinates;
    ;
    if (entry.length & 1)
      entry += '\0';
    glyf += entry;
    loca += toString16(offset / 2);
    offset += entry.length;
    xMins.push(xMin);
    xMaxs.push(xMax);
    yMins.push(yMin);
    yMaxs.push(yMax);
    if (numberOfContours > maxContours)
      maxContours = numberOfContours;
    if (endPoint > maxPoints)
      maxPoints = endPoint;
  }
  loca += toString16(offset / 2);
  tables['glyf'] = glyf;
  tables['head'] = '\0\x01\0\0\0\x01\0\0\0\0\0\0_\x0f<\xf5\0\v\x04\0\0\0\0\0' + toString32(+new Date()) + '\0\0\0\0' + toString32(+new Date()) + toString16(min.apply(null, xMins)) + toString16(min.apply(null, yMins)) + toString16(max.apply(null, xMaxs)) + toString16(max.apply(null, yMaxs)) + toString16((tag.italic ? 2 : 0) | (tag.bold ? 1 : 0)) + '\0\b' + '\0\x02' + '\0\0' + '\0\0';
  ;
  var advance = tag.advance;
  var resolution = tag.resolution || 1;
  tables['hhea'] = '\0\x01\0\0' + toString16(ascent) + toString16(descent) + toString16(leading) + toString16(advance ? max.apply(null, advance) : 1024) + '\0\0' + '\0\0' + '\x03\xb8' + '\0\x01' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + toString16(glyphCount + 1);
  ;
  var hmtx = '\0\0\0\0';
  for (var i = 0; i < glyphCount; ++i)
    hmtx += toString16(advance ? advance[i] / resolution : 1024) + '\0\0';
  tables['hmtx'] = hmtx;
  if (tag.kerning) {
    var kerning = tag.kerning;
    var nPairs = kerning.length;
    var searchRange = maxPower2(nPairs) * 2;
    var kern = '\0\0\0\x01\0\0' + toString16(14 + nPairs * 6) + '\0\x01' + toString16(nPairs) + toString16(searchRange) + toString16(logE(nPairs) / logE(2)) + toString16(2 * nPairs - searchRange);
    ;
    var i = 0;
    var record;
    while (record = kerning[i++]) {
      kern += toString16(glyphIndex[record.code1]) + toString16(glyphIndex[record.code2]) + toString16(record.adjustment);
      ;
    }
    tables['kern'] = kern;
  }
  tables['loca'] = loca;
  tables['maxp'] = '\0\x01\0\0' + toString16(glyphCount + 1) + toString16(maxPoints) + toString16(maxContours) + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0' + '\0\0';
  ;
  var uniqueId = 'swf-font-' + nextFontId++;
  var fontName = tag.name || uniqueId;
  var psName = fontName.replace(/ /g, '');
  var strings = [
      tag.copyright || 'Original licence',
      fontName,
      'Unknown',
      uniqueId,
      fontName,
      '1.0',
      psName,
      'Unknown',
      'Unknown',
      'Unknown'
    ];
  var count = strings.length;
  var name = '\0\0' + toString16(count) + toString16(count * 12 + 6);
  var offset = 0;
  var i = 0;
  var str;
  while (str = strings[i++]) {
    name += '\0\x01\0\0\0\0' + toString16(i - 1) + toString16(str.length) + toString16(offset);
    offset += str.length;
  }
  tables['name'] = name + strings.join('');
  tables['post'] = '\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0';
  ;
  var names = keys(tables);
  var numTables = names.length;
  var header = '\0\x01\0\0' + toString16(numTables) + '\0\x80' + '\0\x03' + '\0 ';
  ;
  var data = '';
  var offset = numTables * 16 + header.length;
  var i = 0;
  var name;
  while (name = names[i++]) {
    var table = tables[name];
    var length = table.length;
    header += name + '\0\0\0\0' + toString32(offset) + toString32(length);
    ;
    while (length & 3) {
      table += '\0';
      ++length;
    }
    data += table;
    while (offset & 3)
      ++offset;
    offset += length;
  }
  var otf = header + data;
  var unitPerEm = 1024;
  var metrics = {
      ascent: ascent / unitPerEm,
      descent: descent / unitPerEm,
      leading: leading / unitPerEm
    };
  return {
    type: 'font',
    id: tag.id,
    name: fontName,
    uniqueName: psName + uniqueId,
    codes: codes,
    metrics: metrics,
    data: otf
  };
}
function getUint16(buff, pos) {
  return buff[pos] << 8 | buff[pos + 1];
}
function parseJpegChunks(imgDef, bytes) {
  var i = 0;
  var n = bytes.length;
  var chunks = [];
  var code;
  do {
    var begin = i;
    while (i < n && bytes[i] !== 255)
      ++i;
    while (i < n && bytes[i] === 255)
      ++i;
    code = bytes[i++];
    if (code === 218) {
      i = n;
    } else if (code === 217) {
      i += 2;
      continue;
    } else if (code < 208 || code > 216) {
      var length = getUint16(bytes, i);
      if (code >= 192 && code <= 195) {
        imgDef.height = getUint16(bytes, i + 3);
        imgDef.width = getUint16(bytes, i + 5);
      }
      i += length;
    }
    chunks.push(bytes.subarray(begin, i));
  } while (i < n);
  return chunks;
}
function defineImage(tag, dictionary) {
  var img = {
      type: 'image',
      id: tag.id,
      mimeType: tag.mimeType
    };
  var imgData = tag.imgData;
  var chunks;
  if (tag.mimeType === 'image/jpeg') {
    chunks = parseJpegChunks(img, imgData);
    var alphaData = tag.alphaData;
    if (alphaData) {
      img.mask = createInflatedStream(alphaData, img.width * img.height).bytes;
    }
    if (tag.incomplete) {
      var tables = dictionary[0];
      var header = tables.data;
      if (header && header.size) {
        chunks[0] = chunks[0].subarray(2);
        chunks.unshift(header.slice(0, header.size - 2));
      }
    }
  } else {
    chunks = [
      imgData
    ];
  }
  img.data = new Blob(chunks, {
    type: tag.mimeType
  });
  return img;
}
function defineLabel(tag, dictionary) {
  var records = tag.records;
  var m = tag.matrix;
  var cmds = [
      'c.save()',
      'c.transform(' + [
        m.a,
        m.b,
        m.c,
        m.d,
        m.tx,
        m.ty
      ].join(',') + ')',
      'c.scale(0.05, 0.05)'
    ];
  var dependencies = [];
  var x = 0;
  var y = 0;
  var i = 0;
  var record;
  var codes;
  while (record = records[i++]) {
    if (record.eot)
      break;
    if (record.hasFont) {
      var font = dictionary[record.fontId];
      codes = font.codes;
      cmds.push('c.font="' + record.fontHeight + 'px \'' + font.uniqueName + '\'"');
      dependencies.push(font.id);
    }
    if (record.hasColor) {
      cmds.push('ct.setFillStyle(c,"' + rgbaObjToStr(record.color) + '")');
      cmds.push('ct.setAlpha(c)');
    } else {
      cmds.push('ct.setAlpha(c,true)');
    }
    if (record.hasMoveX)
      x = record.moveX;
    if (record.hasMoveY)
      y = record.moveY;
    var entries = record.entries;
    var j = 0;
    var entry;
    while (entry = entries[j++]) {
      var code = codes[entry.glyphIndex];
      var text = code >= 32 && code != 34 && code != 92 ? fromCharCode(code) : '\\u' + (code + 65536).toString(16).substring(1);
      cmds.push('c.fillText("' + text + '",' + x + ',' + y + ')');
      x += entry.advance;
    }
  }
  cmds.push('c.restore()');
  var label = {
      type: 'label',
      id: tag.id,
      bbox: tag.bbox,
      data: cmds.join('\n')
    };
  if (dependencies.length)
    label.require = dependencies;
  return label;
}
var GRAPHICS_FILL_CLIPPED_BITMAP = 65;
var GRAPHICS_FILL_FOCAL_RADIAL_GRADIENT = 19;
var GRAPHICS_FILL_LINEAR_GRADIENT = 16;
var GRAPHICS_FILL_NONSMOOTHED_CLIPPED_BITMAP = 67;
var GRAPHICS_FILL_NONSMOOTHED_REPEATING_BITMAP = 66;
var GRAPHICS_FILL_RADIAL_GRADIENT = 18;
var GRAPHICS_FILL_REPEATING_BITMAP = 64;
var GRAPHICS_FILL_SOLID = 0;
function applySegmentToStyles(segment, styles, linePaths, fillPaths, isMorph) {
  if (!segment) {
    return;
  }
  var commands = segment.commands;
  var data = segment.data;
  var morphData = segment.morphData;
  if (morphData) {
  }
  var path;
  var targetSegment;
  var command;
  var i;
  if (styles.fill0) {
    path = fillPaths[styles.fill0 - 1];
    if (!(styles.fill1 || styles.line)) {
      targetSegment = path.head();
      targetSegment.commands = [];
      targetSegment.data = [];
      targetSegment.morphData = isMorph ? [] : null;
    } else {
      targetSegment = path.addSegment([], [], isMorph ? [] : null);
    }
    var targetCommands = targetSegment.commands;
    var targetData = targetSegment.data;
    var targetMorphData = targetSegment.morphData;
    targetCommands.push(SHAPE_MOVE_TO);
    var j = data.length - 2;
    targetData.push(data[j], data[j + 1]);
    if (isMorph) {
      targetMorphData.push(morphData[j], morphData[j + 1]);
    }
    for (i = commands.length; i-- > 1; j -= 2) {
      command = commands[i];
      targetCommands.push(command);
      targetData.push(data[j - 2], data[j - 1]);
      if (isMorph) {
        targetMorphData.push(morphData[j - 2], morphData[j - 1]);
      }
      if (command === SHAPE_CURVE_TO) {
        targetData.push(data[j - 4], data[j - 3]);
        if (isMorph) {
          targetMorphData.push(morphData[j - 4], morphData[j - 3]);
        }
        j -= 2;
      }
    }
    if (isMorph) {
    }
  }
  if (styles.line && styles.fill1) {
    path = linePaths[styles.line - 1];
    path.addSegment(commands, data, morphData);
  }
}
function convertRecordsToStyledPaths(records, fillPaths, linePaths, dictionary, dependencies, recordsMorph) {
  var isMorph = recordsMorph !== null;
  var styles = {
      fill0: 0,
      fill1: 0,
      line: 0
    };
  var segment = null;
  var allFillPaths = fillPaths;
  var allLinePaths = linePaths;
  var numRecords = records.length - 1;
  var x = 0;
  var y = 0;
  var morphX = 0;
  var morphY = 0;
  var path;
  for (var i = 0, j = 0; i < numRecords; i++) {
    var record = records[i];
    var morphRecord;
    if (isMorph) {
      morphRecord = recordsMorph[j++];
    }
    if (record.type === 0) {
      if (segment) {
        applySegmentToStyles(segment, styles, linePaths, fillPaths, isMorph);
      }
      if (record.hasNewStyles) {
        fillPaths = createPathsList(record.fillStyles, false, dictionary, dependencies);
        push.apply(allFillPaths, fillPaths);
        linePaths = createPathsList(record.lineStyles, true, dictionary, dependencies);
        push.apply(allLinePaths, linePaths);
        styles = {
          fill0: 0,
          fill1: 0,
          line: 0
        };
      }
      if (record.hasFillStyle0) {
        styles.fill0 = record.fillStyle0;
      }
      if (record.hasFillStyle1) {
        styles.fill1 = record.fillStyle1;
      }
      if (record.hasLineStyle) {
        styles.line = record.lineStyle;
      }
      if (styles.fill1) {
        path = fillPaths[styles.fill1 - 1];
      } else if (styles.line) {
        path = linePaths[styles.line - 1];
      } else if (styles.fill0) {
        path = fillPaths[styles.fill0 - 1];
      }
      if (record.move) {
        x = record.moveX | 0;
        y = record.moveY | 0;
      }
      if (path) {
        segment = path.addSegment([], [], isMorph ? [] : null);
        segment.commands.push(SHAPE_MOVE_TO);
        segment.data.push(x, y);
        if (isMorph) {
          if (morphRecord.type === 0) {
            morphX = morphRecord.moveX | 0;
            morphY = morphRecord.moveY | 0;
          } else {
            morphX = x;
            morphY = y;
            j--;
          }
          segment.morphData.push(morphX, morphY);
        }
      }
    } else {
      if (isMorph) {
      }
      if (record.isStraight && (!isMorph || morphRecord.isStraight)) {
        x += record.deltaX | 0;
        y += record.deltaY | 0;
        segment.commands.push(SHAPE_LINE_TO);
        segment.data.push(x, y);
        if (isMorph) {
          morphX += morphRecord.deltaX | 0;
          morphY += morphRecord.deltaY | 0;
          segment.morphData.push(morphX, morphY);
        }
      } else {
        var cx, cy;
        var deltaX, deltaY;
        if (!record.isStraight) {
          cx = x + record.controlDeltaX | 0;
          cy = y + record.controlDeltaY | 0;
          x = cx + record.anchorDeltaX | 0;
          y = cy + record.anchorDeltaY | 0;
        } else {
          deltaX = record.deltaX | 0;
          deltaY = record.deltaY | 0;
          cx = x + (deltaX >> 1);
          cy = y + (deltaY >> 1);
          x += deltaX;
          y += deltaY;
        }
        segment.commands.push(SHAPE_CURVE_TO);
        segment.data.push(cx, cy, x, y);
        if (isMorph) {
          if (!morphRecord.isStraight) {
            cx = morphX + morphRecord.controlDeltaX | 0;
            cy = morphY + morphRecord.controlDeltaY | 0;
            morphX = cx + morphRecord.anchorDeltaX | 0;
            morphY = cy + morphRecord.anchorDeltaY | 0;
          } else {
            deltaX = morphRecord.deltaX | 0;
            deltaY = morphRecord.deltaY | 0;
            cx = morphX + (deltaX >> 1);
            cy = morphY + (deltaY >> 1);
            morphX += deltaX;
            morphY += deltaY;
          }
          segment.morphData.push(cx, cy, morphX, morphY);
        }
      }
    }
  }
  applySegmentToStyles(segment, styles, linePaths, fillPaths, isMorph);
  push.apply(allFillPaths, allLinePaths);
  var removeCount = 0;
  for (i = 0; i < allFillPaths.length; i++) {
    path = allFillPaths[i];
    if (!path.head()) {
      removeCount++;
      continue;
    }
    allFillPaths[i - removeCount] = segmentedPathToShapePath(path, isMorph);
  }
  allFillPaths.length -= removeCount;
  return allFillPaths;
}
function segmentedPathToShapePath(path, isMorph) {
  var start = path.head();
  var end = start;
  var finalRoot = null;
  var finalHead = null;
  var skippedMoves = 0;
  var current = start.prev;
  while (start) {
    while (current) {
      if (path.segmentsConnect(current, start)) {
        if (current.next !== start) {
          path.removeSegment(current);
          path.insertSegment(current, start);
        }
        start = current;
        current = start.prev;
        skippedMoves++;
        continue;
      }
      if (path.segmentsConnect(end, current)) {
        path.removeSegment(current);
        end.next = current;
        current = current.prev;
        end.next.prev = end;
        end.next.next = null;
        end = end.next;
        skippedMoves++;
        continue;
      }
      current = current.prev;
    }
    current = start.prev;
    if (!finalRoot) {
      finalRoot = start;
      finalHead = end;
    } else {
      finalHead.next = start;
      start.prev = finalHead;
      finalHead = end;
      finalHead.next = null;
    }
    if (!current) {
      break;
    }
    start = end = current;
    current = start.prev;
  }
  var totalCommandsLength = -skippedMoves;
  var totalDataLength = -skippedMoves << 1;
  current = finalRoot;
  while (current) {
    totalCommandsLength += current.commands.length;
    totalDataLength += current.data.length;
    current = current.next;
  }
  var shape = new ShapePath(path.fillStyle, path.lineStyle, totalCommandsLength, totalDataLength, isMorph);
  var allCommands = shape.commands;
  var allData = shape.data;
  var allMorphData = shape.morphData;
  var commandsIndex = 0;
  var dataIndex = 0;
  current = finalRoot;
  while (current) {
    var offset = 0;
    var commands = current.commands;
    var data = current.data;
    var morphData = current.morphData;
    if (data[0] === allData[dataIndex - 2] && data[1] === allData[dataIndex - 1]) {
      offset = 1;
    }
    for (var i = offset; i < commands.length; i++, commandsIndex++) {
      allCommands[commandsIndex] = commands[i];
    }
    for (i = offset << 1; i < data.length; i++, dataIndex++) {
      allData[dataIndex] = data[i];
      if (isMorph) {
        allMorphData[dataIndex] = morphData[i];
      }
    }
    current = current.next;
  }
  return shape;
}
var CAPS_STYLE_TYPES = [
    'round',
    'none',
    'square'
  ];
var JOIN_STYLE_TYPES = [
    'round',
    'bevel',
    'miter'
  ];
function processStyle(style, isLineStyle, dictionary, dependencies) {
  if (isLineStyle) {
    style.lineCap = CAPS_STYLE_TYPES[style.endCapStyle | 0];
    style.lineJoin = JOIN_STYLE_TYPES[style.joinStyle | 0];
    style.miterLimit = style.miterLimitFactor * 2;
    if (!style.color && style.hasFill) {
      var fillStyle = processStyle(style.fillStyle, false, dictionary, dependencies);
      style.style = fillStyle.style;
      style.type = fillStyle.type;
      style.transform = fillStyle.transform;
      style.records = fillStyle.records;
      style.focalPoint = fillStyle.focalPoint;
      style.bitmapId = fillStyle.bitmapId;
      style.repeat = fillStyle.repeat;
      style.fillStyle = null;
      return style;
    }
  }
  var color;
  if (style.type === undefined || style.type === GRAPHICS_FILL_SOLID) {
    color = style.color;
    style.style = 'rgba(' + color.red + ',' + color.green + ',' + color.blue + ',' + color.alpha / 255 + ')';
    style.color = null;
    return style;
  }
  var scale;
  switch (style.type) {
  case GRAPHICS_FILL_LINEAR_GRADIENT:
  case GRAPHICS_FILL_RADIAL_GRADIENT:
  case GRAPHICS_FILL_FOCAL_RADIAL_GRADIENT:
    scale = 819.2;
    break;
  case GRAPHICS_FILL_REPEATING_BITMAP:
  case GRAPHICS_FILL_CLIPPED_BITMAP:
  case GRAPHICS_FILL_NONSMOOTHED_REPEATING_BITMAP:
  case GRAPHICS_FILL_NONSMOOTHED_CLIPPED_BITMAP:
    if (dictionary[style.bitmapId]) {
      dependencies.push(dictionary[style.bitmapId].id);
      scale = 0.05;
    }
    break;
  default:
    fail('invalid fill style', 'shape');
  }
  if (!style.matrix) {
    return style;
  }
  var matrix = style.matrix;
  style.transform = {
    a: matrix.a * scale,
    b: matrix.b * scale,
    c: matrix.c * scale,
    d: matrix.d * scale,
    e: matrix.tx,
    f: matrix.ty
  };
  style.matrix = null;
  return style;
}
function createPathsList(styles, isLineStyle, dictionary, dependencies) {
  var paths = [];
  for (var i = 0; i < styles.length; i++) {
    var style = processStyle(styles[i], isLineStyle, dictionary, dependencies);
    if (!isLineStyle) {
      paths[i] = new SegmentedPath(style, null);
    } else {
      paths[i] = new SegmentedPath(null, style);
    }
  }
  return paths;
}
function defineShape(tag, dictionary) {
  var dependencies = [];
  var fillPaths = createPathsList(tag.fillStyles, false, dictionary, dependencies);
  var linePaths = createPathsList(tag.lineStyles, true, dictionary, dependencies);
  var paths = convertRecordsToStyledPaths(tag.records, fillPaths, linePaths, dictionary, dependencies, tag.recordsMorph || null);
  if (tag.bboxMorph) {
    var mbox = tag.bboxMorph;
    extendBoundsByPoint(tag.bbox, mbox.xMin, mbox.yMin);
    extendBoundsByPoint(tag.bbox, mbox.xMax, mbox.yMax);
    mbox = tag.strokeBboxMorph;
    if (mbox) {
      extendBoundsByPoint(tag.strokeBbox, mbox.xMin, mbox.yMin);
      extendBoundsByPoint(tag.strokeBbox, mbox.xMax, mbox.yMax);
    }
  }
  return {
    type: 'shape',
    id: tag.id,
    strokeBbox: tag.strokeBbox,
    strokeBboxMorph: tag.strokeBboxMorph,
    bbox: tag.bbox,
    bboxMorph: tag.bboxMorph,
    isMorph: tag.isMorph,
    paths: paths,
    require: dependencies.length ? dependencies : null
  };
}
function SegmentedPath(fillStyle, lineStyle) {
  this.fillStyle = fillStyle;
  this.lineStyle = lineStyle;
  this._head = null;
}
SegmentedPath.prototype = {
  addSegment: function (commands, data, morphData) {
    var segment = {
        commands: commands,
        data: data,
        morphData: morphData,
        prev: this._head,
        next: null
      };
    if (this._head) {
      this._head.next = segment;
    }
    this._head = segment;
    return segment;
  },
  removeSegment: function (segment) {
    if (segment.prev) {
      segment.prev.next = segment.next;
    }
    if (segment.next) {
      segment.next.prev = segment.prev;
    }
  },
  insertSegment: function (segment, next) {
    var prev = next.prev;
    segment.prev = prev;
    segment.next = next;
    if (prev) {
      prev.next = segment;
    }
    next.prev = segment;
  },
  head: function () {
    return this._head;
  },
  segmentsConnect: function (first, second) {
    var firstLength = first.data.length;
    return first.data[firstLength - 2] === second.data[0] && first.data[firstLength - 1] === second.data[1];
  }
};
var SHAPE_MOVE_TO = 1;
var SHAPE_LINE_TO = 2;
var SHAPE_CURVE_TO = 3;
var SHAPE_WIDE_MOVE_TO = 4;
var SHAPE_WIDE_LINE_TO = 5;
var SHAPE_CUBIC_CURVE_TO = 6;
var SHAPE_ROUND_CORNER = 7;
var SHAPE_CIRCLE = 8;
var SHAPE_ELLIPSE = 9;
function ShapePath(fillStyle, lineStyle, commandsCount, dataLength, isMorph) {
  this.fillStyle = fillStyle;
  this.lineStyle = lineStyle;
  if (commandsCount) {
    this.commands = new Uint8Array(commandsCount);
    this.data = new Int32Array(dataLength);
    this.morphData = isMorph ? new Int32Array(dataLength) : null;
  } else {
    this.commands = [];
    this.data = [];
  }
  this.bounds = null;
  this.strokeBounds = null;
  this.isMorph = isMorph;
  this.fullyInitialized = false;
}
ShapePath.prototype = {
  moveTo: function (x, y) {
    if (this.commands[this.commands.length - 1] === SHAPE_MOVE_TO) {
      this.data[this.data.length - 2] = x;
      this.data[this.data.length - 1] = y;
      return;
    }
    this.commands.push(SHAPE_MOVE_TO);
    this.data.push(x, y);
  },
  lineTo: function (x, y) {
    this.commands.push(SHAPE_LINE_TO);
    this.data.push(x, y);
  },
  curveTo: function (controlX, controlY, anchorX, anchorY) {
    this.commands.push(SHAPE_CURVE_TO);
    this.data.push(controlX, controlY, anchorX, anchorY);
  },
  cubicCurveTo: function (control1X, control1Y, control2X, control2Y, anchorX, anchorY) {
    this.commands.push(SHAPE_CUBIC_CURVE_TO);
    this.data.push(control1X, control1Y, control2X, control2Y, anchorX, anchorY);
  },
  rect: function (x, y, w, h) {
    var x2 = x + w;
    var y2 = y + h;
    this.commands.push(SHAPE_MOVE_TO, SHAPE_LINE_TO, SHAPE_LINE_TO, SHAPE_LINE_TO, SHAPE_LINE_TO);
    this.data.push(x, y, x2, y, x2, y2, x, y2, x, y);
  },
  circle: function (x, y, radius) {
    this.commands.push(SHAPE_CIRCLE);
    this.data.push(x, y, radius);
  },
  drawRoundCorner: function (cornerX, cornerY, curveEndX, curveEndY, radiusX, radiusY) {
    this.commands.push(SHAPE_ROUND_CORNER);
    this.data.push(cornerX, cornerY, curveEndX, curveEndY, radiusX, radiusY);
  },
  ellipse: function (x, y, radiusX, radiusY) {
    this.commands.push(SHAPE_ELLIPSE);
    this.data.push(x, y, radiusX, radiusY);
  },
  draw: function (ctx, clip, ratio, colorTransform) {
    if (clip && !this.fillStyle) {
      return;
    }
    ctx.beginPath();
    var commands = this.commands;
    var data = this.data;
    var morphData = this.morphData;
    var formOpen = false;
    var formOpenX = 0;
    var formOpenY = 0;
    for (var j = 0, k = 0; j < commands.length; j++) {
      if (!this.isMorph) {
        switch (commands[j]) {
        case SHAPE_MOVE_TO:
          formOpen = true;
          formOpenX = data[k++] / 20;
          formOpenY = data[k++] / 20;
          ctx.moveTo(formOpenX, formOpenY);
          break;
        case SHAPE_WIDE_MOVE_TO:
          ctx.moveTo(data[k++] / 20, data[k++] / 20);
          k += 2;
          break;
        case SHAPE_LINE_TO:
          ctx.lineTo(data[k++] / 20, data[k++] / 20);
          break;
        case SHAPE_WIDE_LINE_TO:
          ctx.lineTo(data[k++] / 20, data[k++] / 20);
          k += 2;
          break;
        case SHAPE_CURVE_TO:
          ctx.quadraticCurveTo(data[k++] / 20, data[k++] / 20, data[k++] / 20, data[k++] / 20);
          break;
        case SHAPE_CUBIC_CURVE_TO:
          ctx.bezierCurveTo(data[k++] / 20, data[k++] / 20, data[k++] / 20, data[k++] / 20, data[k++] / 20, data[k++] / 20);
          break;
        case SHAPE_ROUND_CORNER:
          ctx.arcTo(data[k++] / 20, data[k++] / 20, data[k++] / 20, data[k++] / 20, data[k++] / 20, data[k++] / 20);
          break;
        case SHAPE_CIRCLE:
          if (formOpen) {
            ctx.lineTo(formOpenX, formOpenY);
            formOpen = false;
          }
          ctx.moveTo((data[k] + data[k + 2]) / 20, data[k + 1] / 20);
          ctx.arc(data[k++] / 20, data[k++] / 20, data[k++] / 20, 0, Math.PI * 2, false);
          break;
        case SHAPE_ELLIPSE:
          if (formOpen) {
            ctx.lineTo(formOpenX, formOpenY);
            formOpen = false;
          }
          var x = data[k++];
          var y = data[k++];
          var rX = data[k++];
          var rY = data[k++];
          var radius;
          if (rX !== rY) {
            ctx.save();
            var ellipseScale;
            if (rX > rY) {
              ellipseScale = rX / rY;
              radius = rY;
              x /= ellipseScale;
              ctx.scale(ellipseScale, 1);
            } else {
              ellipseScale = rY / rX;
              radius = rX;
              y /= ellipseScale;
              ctx.scale(1, ellipseScale);
            }
          }
          ctx.moveTo((x + radius) / 20, y / 20);
          ctx.arc(x / 20, y / 20, radius / 20, 0, Math.PI * 2, false);
          if (rX !== rY) {
            ctx.restore();
          }
          break;
        default:
          console.warn('Unknown drawing command encountered: ' + commands[j]);
        }
      } else {
        switch (commands[j]) {
        case SHAPE_MOVE_TO:
          ctx.moveTo(morph(data[k] / 20, morphData[k++] / 20, ratio), morph(data[k] / 20, morphData[k++] / 20, ratio));
          break;
        case SHAPE_LINE_TO:
          ctx.lineTo(morph(data[k] / 20, morphData[k++] / 20, ratio), morph(data[k] / 20, morphData[k++] / 20, ratio));
          break;
        case SHAPE_CURVE_TO:
          ctx.quadraticCurveTo(morph(data[k] / 20, morphData[k++] / 20, ratio), morph(data[k] / 20, morphData[k++] / 20, ratio), morph(data[k] / 20, morphData[k++] / 20, ratio), morph(data[k] / 20, morphData[k++] / 20, ratio));
          break;
        default:
          console.warn('Drawing command not supported for morph shapes: ' + commands[j]);
        }
      }
    }
    if (!clip) {
      var fillStyle = this.fillStyle;
      if (fillStyle) {
        colorTransform.setFillStyle(ctx, fillStyle.style);
        var m = fillStyle.transform;
        ctx.save();
        colorTransform.setAlpha(ctx);
        if (m) {
          ctx.transform(m.a, m.b, m.c, m.d, m.e / 20, m.f / 20);
        }
        ctx.fill();
        ctx.restore();
      }
      var lineStyle = this.lineStyle;
      if (lineStyle) {
        colorTransform.setStrokeStyle(ctx, lineStyle.style);
        ctx.save();
        colorTransform.setAlpha(ctx);
        ctx.lineWidth = Math.max(lineStyle.width / 20, 1);
        ctx.lineCap = lineStyle.lineCap;
        ctx.lineJoin = lineStyle.lineJoin;
        ctx.miterLimit = lineStyle.miterLimit;
        ctx.stroke();
        ctx.restore();
      }
    }
    ctx.closePath();
  },
  isPointInPath: function (x, y) {
    if (!(this.fillStyle || this.lineStyle)) {
      return false;
    }
    var bounds = this.strokeBounds || this.bounds || this._calculateBounds();
    if (x < bounds.xMin || x > bounds.xMax || y < bounds.yMin || y > bounds.yMax) {
      return false;
    }
    if (this.fillStyle && this.isPointInFill(x, y)) {
      return true;
    }
    return this.lineStyle && this.lineStyle.width !== undefined && this.isPointInStroke(x, y);
  },
  isPointInFill: function (x, y) {
    var commands = this.commands;
    var data = this.data;
    var length = commands.length;
    var inside = false;
    var fromX = 0;
    var fromY = 0;
    var toX = 0;
    var toY = 0;
    var localX;
    var localY;
    var cpX;
    var cpY;
    var rX;
    var rY;
    var formOpen = false;
    var formOpenX = 0;
    var formOpenY = 0;
    for (var commandIndex = 0, dataIndex = 0; commandIndex < length; commandIndex++) {
      switch (commands[commandIndex]) {
      case SHAPE_WIDE_MOVE_TO:
        dataIndex += 2;
      case SHAPE_MOVE_TO:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        if (formOpen && intersectsLine(x, y, fromX, fromY, formOpenX, formOpenY)) {
          inside = !inside;
        }
        formOpen = true;
        formOpenX = toX;
        formOpenY = toY;
        break;
      case SHAPE_WIDE_LINE_TO:
        dataIndex += 2;
      case SHAPE_LINE_TO:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        if (intersectsLine(x, y, fromX, fromY, toX, toY)) {
          inside = !inside;
        }
        break;
      case SHAPE_CURVE_TO:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        if (cpY > y === fromY > y && toY > y === fromY > y) {
          break;
        }
        if (fromX >= x && cpX >= x && toX >= x) {
          inside = !inside;
          break;
        }
        var a = fromY - 2 * cpY + toY;
        var c = fromY - y;
        var b = 2 * (cpY - fromY);
        var d = b * b - 4 * a * c;
        if (d < 0) {
          break;
        }
        d = Math.sqrt(d);
        a = 1 / (a + a);
        var t1 = (d - b) * a;
        var t2 = (-b - d) * a;
        if (t1 >= 0 && t1 <= 1 && quadraticBezier(fromX, cpX, toX, t1) > x) {
          inside = !inside;
        }
        if (t2 >= 0 && t2 <= 1 && quadraticBezier(fromX, cpX, toX, t2) > x) {
          inside = !inside;
        }
        break;
      case SHAPE_CUBIC_CURVE_TO:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        var cp2X = data[dataIndex++];
        var cp2Y = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        if (cpY > y === fromY > y && cp2Y > y === fromY > y && toY > y === fromY > y) {
          break;
        }
        if (fromX >= x && cpX >= x && cp2X >= x && toX >= x) {
          inside = !inside;
          break;
        }
        var roots = cubicXAtY(fromX, fromY, cpX, cpY, cp2X, cp2Y, toX, toY, y);
        for (var i = roots.length; i--;) {
          if (roots[i] >= x) {
            inside = !inside;
          }
        }
        break;
      case SHAPE_ROUND_CORNER:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        rX = data[dataIndex++];
        rY = data[dataIndex++];
        if (toY > y === fromY > y || fromX < x && toX < x) {
          break;
        }
        if (fromX >= x && toX >= x) {
          inside = !inside;
          break;
        }
        if (toX > fromX === toY > fromY) {
          cp2X = fromX;
          cp2Y = toY;
        } else {
          cp2X = toX;
          cp2Y = fromY;
        }
        localX = x - cp2X;
        localY = y - cp2Y;
        if (localX * localX / (rX * rX) + localY * localY / (rY * rY) <= 1 !== localX <= 0) {
          inside = !inside;
        }
        break;
      case SHAPE_CIRCLE:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        var r = data[dataIndex++];
        localX = x - toX;
        localY = y - toY;
        if (localX * localX + localY * localY < r * r) {
          inside = !inside;
        }
        toX += r;
        break;
      case SHAPE_ELLIPSE:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        rX = data[dataIndex++];
        rY = data[dataIndex++];
        localX = x - cpX;
        localY = y - cpY;
        if (localX * localX / (rX * rX) + localY * localY / (rY * rY) <= 1) {
          inside = !inside;
        }
        toX = cpX + rX;
        toY = cpY;
        break;
      default:
        if (!inWorker) {
          console.warn('Drawing command not handled in isPointInPath: ' + commands[commandIndex]);
        }
      }
      fromX = toX;
      fromY = toY;
    }
    if (formOpen && intersectsLine(x, y, fromX, fromY, formOpenX, formOpenY)) {
      inside = !inside;
    }
    return inside;
  },
  isPointInStroke: function (x, y) {
    var commands = this.commands;
    var data = this.data;
    var length = commands.length;
    var width = this.lineStyle.width;
    var halfWidth = width / 2;
    var halfWidthSq = halfWidth * halfWidth;
    var minX = x - halfWidth;
    var maxX = x + halfWidth;
    var minY = y - halfWidth;
    var maxY = y + halfWidth;
    var fromX = 0;
    var fromY = 0;
    var toX = 0;
    var toY = 0;
    var localX;
    var localY;
    var cpX;
    var cpY;
    var rX;
    var rY;
    var curveX;
    var curveY;
    var t;
    for (var commandIndex = 0, dataIndex = 0; commandIndex < length; commandIndex++) {
      switch (commands[commandIndex]) {
      case SHAPE_WIDE_MOVE_TO:
        dataIndex += 2;
      case SHAPE_MOVE_TO:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        break;
      case SHAPE_WIDE_LINE_TO:
        dataIndex += 2;
      case SHAPE_LINE_TO:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        if (fromX === toX && fromY === toY) {
          break;
        }
        if (maxX < fromX && maxX < toX || minX > fromX && minX > toX || maxY < fromY && maxY < toY || minY > fromY && minY > toY) {
          break;
        }
        if (toX === fromX || toY === fromY) {
          return true;
        }
        t = ((x - fromX) * (toX - fromX) + (y - fromY) * (toY - fromY)) / distanceSq(fromX, fromY, toX, toY);
        if (t < 0) {
          if (distanceSq(x, y, fromX, fromY) <= halfWidthSq) {
            return true;
          }
          break;
        }
        if (t > 1) {
          if (distanceSq(x, y, toX, toY) <= halfWidthSq) {
            return true;
          }
          break;
        }
        if (distanceSq(x, y, fromX + t * (toX - fromX), fromY + t * (toY - fromY)) <= halfWidthSq) {
          return true;
        }
        break;
      case SHAPE_CURVE_TO:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        var extremeX = quadraticBezierExtreme(fromX, cpX, toX);
        if (maxX < fromX && maxX < extremeX && maxX < toX || minX > fromX && minX > extremeX && minX > toX) {
          break;
        }
        var extremeY = quadraticBezierExtreme(fromY, cpY, toY);
        if (maxY < fromY && maxY < extremeY && maxY < toY || minY > fromY && minY > extremeY && minY > toY) {
          break;
        }
        for (t = 0; t < 1; t += 0.02) {
          curveX = quadraticBezier(fromX, cpX, toX, t);
          if (curveX < minX || curveX > maxX) {
            continue;
          }
          curveY = quadraticBezier(fromY, cpY, toY, t);
          if (curveY < minY || curveY > maxY) {
            continue;
          }
          if ((x - curveX) * (x - curveX) + (y - curveY) * (y - curveY) < halfWidthSq) {
            return true;
          }
        }
        break;
      case SHAPE_CUBIC_CURVE_TO:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        var cp2X = data[dataIndex++];
        var cp2Y = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        var extremesX = cubicBezierExtremes(fromX, cpX, cp2X, toX);
        while (extremesX.length < 2) {
          extremesX.push(toX);
        }
        if (maxX < fromX && maxX < toX && maxX < extremesX[0] && maxX < extremesX[1] || minX > fromX && minX > toX && minX > extremesX[0] && minX > extremesX[1]) {
          break;
        }
        var extremesY = cubicBezierExtremes(fromY, cpY, cp2Y, toY);
        while (extremesY.length < 2) {
          extremesY.push(toY);
        }
        if (maxY < fromY && maxY < toY && maxY < extremesY[0] && maxY < extremesY[1] || minY > fromY && minY > toY && minY > extremesY[0] && minY > extremesY[1]) {
          break;
        }
        for (t = 0; t < 1; t += 0.02) {
          curveX = cubicBezier(fromX, cpX, cp2X, toX, t);
          if (curveX < minX || curveX > maxX) {
            continue;
          }
          curveY = cubicBezier(fromY, cpY, cp2Y, toY, t);
          if (curveY < minY || curveY > maxY) {
            continue;
          }
          if ((x - curveX) * (x - curveX) + (y - curveY) * (y - curveY) < halfWidthSq) {
            return true;
          }
        }
        break;
      case SHAPE_ROUND_CORNER:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        rX = data[dataIndex++];
        rY = data[dataIndex++];
        if (maxX < fromX && maxX < toX || minX > fromX && minX > toX || maxY < fromY && maxY < toY || minY > fromY && minY > toY) {
          break;
        }
        if (toX > fromX === toY > fromY) {
          cp2X = fromX;
          cp2Y = toY;
        } else {
          cp2X = toX;
          cp2Y = fromY;
        }
        localX = Math.abs(x - cp2X);
        localY = Math.abs(y - cp2Y);
        localX -= halfWidth;
        localY -= halfWidth;
        if (localX * localX / (rX * rX) + localY * localY / (rY * rY) > 1) {
          break;
        }
        localX += width;
        localY += width;
        if (localX * localX / (rX * rX) + localY * localY / (rY * rY) > 1) {
          return true;
        }
        break;
      case SHAPE_CIRCLE:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        var r = data[dataIndex++];
        toX = cpX + r;
        toY = cpY;
        if (maxX < cpX - r || minX > cpX + r || maxY < cpY - r || minY > cpY + r) {
          break;
        }
        localX = x - cpX;
        localY = y - cpY;
        var rMin = r - halfWidth;
        var rMax = r + halfWidth;
        var distSq = localX * localX + localY * localY;
        if (distSq >= rMin * rMin && distSq <= rMax * rMax) {
          return true;
        }
        break;
      case SHAPE_ELLIPSE:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        rX = data[dataIndex++];
        rY = data[dataIndex++];
        toX = cpX + rX;
        toY = cpY;
        localX = Math.abs(x - cpX);
        localY = Math.abs(y - cpY);
        localX -= halfWidth;
        localY -= halfWidth;
        if (localX * localX / (rX * rX) + localY * localY / (rY * rY) > 1) {
          break;
        }
        localX += width;
        localY += width;
        if (localX * localX / (rX * rX) + localY * localY / (rY * rY) > 1) {
          return true;
        }
        break;
      default:
        if (!inWorker) {
          console.warn('Drawing command not handled in isPointInPath: ' + commands[commandIndex]);
        }
      }
      fromX = toX;
      fromY = toY;
    }
    return false;
  },
  getBounds: function (includeStroke) {
    var bounds = includeStroke ? this.strokeBounds : this.bounds;
    if (!bounds) {
      this._calculateBounds();
      bounds = includeStroke ? this.strokeBounds : this.bounds;
    }
    return bounds;
  },
  _calculateBounds: function () {
    var commands = this.commands;
    var data = this.data;
    var length = commands.length;
    var bounds;
    if (commands[0] === SHAPE_MOVE_TO || commands[0] > SHAPE_ROUND_CORNER) {
      bounds = {
        xMin: data[0],
        yMin: data[1]
      };
    } else {
      bounds = {
        xMin: 0,
        yMin: 0
      };
    }
    bounds.xMax = bounds.xMin;
    bounds.yMax = bounds.yMin;
    var fromX = bounds.xMin;
    var fromY = bounds.yMin;
    for (var commandIndex = 0, dataIndex = 0; commandIndex < length; commandIndex++) {
      var toX;
      var toY;
      var cpX;
      var cpY;
      switch (commands[commandIndex]) {
      case SHAPE_WIDE_MOVE_TO:
        dataIndex += 2;
      case SHAPE_MOVE_TO:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        extendBoundsByPoint(bounds, toX, toY);
        break;
      case SHAPE_WIDE_LINE_TO:
        dataIndex += 2;
      case SHAPE_LINE_TO:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        extendBoundsByPoint(bounds, toX, toY);
        break;
      case SHAPE_CURVE_TO:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        extendBoundsByPoint(bounds, toX, toY);
        if (cpX < fromX || cpX > toX) {
          extendBoundsByX(bounds, quadraticBezierExtreme(fromX, cpX, toX));
        }
        if (cpY < fromY || cpY > toY) {
          extendBoundsByY(bounds, quadraticBezierExtreme(fromY, cpY, toY));
        }
        break;
      case SHAPE_CUBIC_CURVE_TO:
        cpX = data[dataIndex++];
        cpY = data[dataIndex++];
        var cp2X = data[dataIndex++];
        var cp2Y = data[dataIndex++];
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        extendBoundsByPoint(bounds, toX, toY);
        var extremes;
        var i;
        if (cpX < fromX || cp2X < fromX || cpX > toX || cp2X > toX) {
          extremes = cubicBezierExtremes(fromX, cpX, cp2X, toX);
          for (i = extremes.length; i--;) {
            extendBoundsByX(bounds, extremes[i]);
          }
        }
        if (cpY < fromY || cp2Y < fromY || cpY > toY || cp2Y > toY) {
          extremes = cubicBezierExtremes(fromY, cpY, cp2Y, toY);
          for (i = extremes.length; i--;) {
            extendBoundsByY(bounds, extremes[i]);
          }
        }
        break;
      case SHAPE_ROUND_CORNER:
        dataIndex += 6;
        break;
      case SHAPE_CIRCLE:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        var radius = data[dataIndex++];
        extendBoundsByPoint(bounds, toX - radius, toY - radius);
        extendBoundsByPoint(bounds, toX + radius, toY + radius);
        toX += radius;
        break;
      case SHAPE_ELLIPSE:
        toX = data[dataIndex++];
        toY = data[dataIndex++];
        var radiusX = data[dataIndex++];
        var radiusY = data[dataIndex++];
        extendBoundsByPoint(bounds, toX - radiusX, toY - radiusY);
        extendBoundsByPoint(bounds, toX + radiusX, toY + radiusY);
        toX += radiusX;
        break;
      default:
        if (!inWorker) {
          console.warn('Drawing command not handled in bounds calculation: ' + commands[commandIndex]);
        }
      }
      fromX = toX;
      fromY = toY;
    }
    this.bounds = bounds;
    if (this.lineStyle) {
      var halfLineWidth = this.lineStyle.width / 2;
      this.strokeBounds = {
        xMin: bounds.xMin - halfLineWidth,
        yMin: bounds.yMin - halfLineWidth,
        xMax: bounds.xMax + halfLineWidth,
        yMax: bounds.yMax + halfLineWidth
      };
      return this.strokeBounds;
    } else {
      this.strokeBounds = bounds;
    }
    return bounds;
  }
};
function distanceSq(x1, y1, x2, y2) {
  var dX = x2 - x1;
  var dY = y2 - y1;
  return dX * dX + dY * dY;
}
function intersectsLine(x, y, x1, y1, x2, y2) {
  return y2 > y !== y1 > y && x < (x1 - x2) * (y - y2) / (y1 - y2) + x2;
}
function quadraticBezier(from, cp, to, t) {
  var inverseT = 1 - t;
  return from * inverseT * inverseT + 2 * cp * inverseT * t + to * t * t;
}
function quadraticBezierExtreme(from, cp, to) {
  var t = (from - cp) / (from - 2 * cp + to);
  if (t < 0) {
    return from;
  }
  if (t > 1) {
    return to;
  }
  return quadraticBezier(from, cp, to, t);
}
function cubicBezier(from, cp, cp2, to, t) {
  var tSq = t * t;
  var inverseT = 1 - t;
  var inverseTSq = inverseT * inverseT;
  return from * inverseT * inverseTSq + 3 * cp * t * inverseTSq + 3 * cp2 * inverseT * tSq + to * t * tSq;
}
function cubicBezierExtremes(from, cp, cp2, to) {
  var d1 = cp - from;
  var d2 = cp2 - cp;
  d2 *= 2;
  var d3 = to - cp2;
  if (d1 + d3 === d2) {
    d3 *= 1.0001;
  }
  var fHead = 2 * d1 - d2;
  var part1 = d2 - 2 * d1;
  var fCenter = Math.sqrt(part1 * part1 - 4 * d1 * (d1 - d2 + d3));
  var fTail = 2 * (d1 - d2 + d3);
  var t1 = (fHead + fCenter) / fTail;
  var t2 = (fHead - fCenter) / fTail;
  var result = [];
  if (t1 >= 0 && t1 <= 1) {
    result.push(cubicBezier(from, cp, cp2, to, t1));
  }
  if (t2 >= 0 && t2 <= 1) {
    result.push(cubicBezier(from, cp, cp2, to, t2));
  }
  return result;
}
function cubicXAtY(x0, y0, cx, cy, cx1, cy1, x1, y1, y) {
  var dX = 3 * (cx - x0);
  var dY = 3 * (cy - y0);
  var bX = 3 * (cx1 - cx) - dX;
  var bY = 3 * (cy1 - cy) - dY;
  var c3X = x1 - x0 - dX - bX;
  var c3Y = y1 - y0 - dY - bY;
  function f(t) {
    return t * (dY + t * (bY + t * c3Y)) + y0 - y;
  }
  function pointAt(t) {
    if (t < 0) {
      t = 0;
    } else if (t > 1) {
      t = 1;
    }
    return x0 + t * (dX + t * (bX + t * c3X));
  }
  function bisectCubicBezierRange(f, l, r, limit) {
    if (Math.abs(r - l) <= limit) {
      return;
    }
    var middle = 0.5 * (l + r);
    if (f(l) * f(r) <= 0) {
      left = l;
      right = r;
      return;
    }
    bisectCubicBezierRange(f, l, middle, limit);
    bisectCubicBezierRange(f, middle, r, limit);
  }
  var left = 0;
  var right = 1;
  bisectCubicBezierRange(f, 0, 1, 0.05);
  var t0 = findRoot(left, right, f, 50, 0.000001);
  var evalResult = Math.abs(f(t0));
  if (evalResult > 0.00001) {
    return [];
  }
  var result = [];
  if (t0 <= 1) {
    result.push(pointAt(t0));
  }
  var a = c3Y;
  var b = t0 * a + bY;
  var c = t0 * b + dY;
  var d = b * b - 4 * a * c;
  if (d < 0) {
    return result;
  }
  d = Math.sqrt(d);
  a = 1 / (a + a);
  var t1 = (d - b) * a;
  var t2 = (-b - d) * a;
  if (t1 >= 0 && t1 <= 1) {
    result.push(pointAt(t1));
  }
  if (t2 >= 0 && t2 <= 1) {
    result.push(pointAt(t2));
  }
  return result;
}
function findRoot(x0, x2, f, maxIterations, epsilon) {
  var x1;
  var y0;
  var y1;
  var y2;
  var b;
  var c;
  var y10;
  var y20;
  var y21;
  var xm;
  var ym;
  var temp;
  var xmlast = x0;
  y0 = f(x0);
  if (y0 === 0) {
    return x0;
  }
  y2 = f(x2);
  if (y2 === 0) {
    return x2;
  }
  if (y2 * y0 > 0) {
    return x0;
  }
  var __iter = 0;
  for (var i = 0; i < maxIterations; ++i) {
    __iter++;
    x1 = 0.5 * (x2 + x0);
    y1 = f(x1);
    if (y1 === 0) {
      return x1;
    }
    if (Math.abs(x1 - x0) < epsilon) {
      return x1;
    }
    if (y1 * y0 > 0) {
      temp = x0;
      x0 = x2;
      x2 = temp;
      temp = y0;
      y0 = y2;
      y2 = temp;
    }
    y10 = y1 - y0;
    y21 = y2 - y1;
    y20 = y2 - y0;
    if (y2 * y20 < 2 * y1 * y10) {
      x2 = x1;
      y2 = y1;
    } else {
      b = (x1 - x0) / y10;
      c = (y10 - y21) / (y21 * y20);
      xm = x0 - b * y0 * (1 - c * y1);
      ym = f(xm);
      if (ym === 0) {
        return xm;
      }
      if (Math.abs(xm - xmlast) < epsilon) {
        return xm;
      }
      xmlast = xm;
      if (ym * y0 < 0) {
        x2 = xm;
        y2 = ym;
      } else {
        x0 = xm;
        y0 = ym;
        x2 = x1;
        y2 = y1;
      }
    }
  }
  return x1;
}
function extendBoundsByPoint(bounds, x, y) {
  if (x < bounds.xMin) {
    bounds.xMin = x;
  } else if (x > bounds.xMax) {
    bounds.xMax = x;
  }
  if (y < bounds.yMin) {
    bounds.yMin = y;
  } else if (y > bounds.yMax) {
    bounds.yMax = y;
  }
}
function extendBoundsByX(bounds, x) {
  if (x < bounds.xMin) {
    bounds.xMin = x;
  } else if (x > bounds.xMax) {
    bounds.xMax = x;
  }
}
function extendBoundsByY(bounds, y) {
  if (y < bounds.yMin) {
    bounds.yMin = y;
  } else if (y > bounds.yMax) {
    bounds.yMax = y;
  }
}
function morph(start, end, ratio) {
  return start + (end - start) * ratio;
}
function finishShapePaths(paths, dictionary) {
  for (var i = 0; i < paths.length; i++) {
    var path = paths[i];
    if (path.fullyInitialized) {
      continue;
    }
    if (!(path instanceof ShapePath)) {
      var untypedPath = path;
      path = paths[i] = new ShapePath(path.fillStyle, path.lineStyle, 0, 0, path.isMorph);
      path.commands = untypedPath.commands;
      path.data = untypedPath.data;
      path.morphData = untypedPath.morphData;
    }
    path.fillStyle && initStyle(path.fillStyle, dictionary);
    path.lineStyle && initStyle(path.lineStyle, dictionary);
    path.fullyInitialized = true;
  }
}
var inWorker = typeof window === 'undefined';
var factoryCtx = !inWorker ? document.createElement('canvas').getContext('kanvas-2d') : null;
function buildLinearGradientFactory(colorStops) {
  var defaultGradient = factoryCtx.createLinearGradient(-1, 0, 1, 0);
  for (var i = 0; i < colorStops.length; i++) {
    defaultGradient.addColorStop(colorStops[i].ratio, colorStops[i].color);
  }
  var fn = function createLinearGradient(ctx, colorTransform) {
    var gradient = ctx.createLinearGradient(-1, 0, 1, 0);
    for (var i = 0; i < colorStops.length; i++) {
      colorTransform.addGradientColorStop(gradient, colorStops[i].ratio, colorStops[i].color);
    }
    return gradient;
  };
  fn.defaultGradient = defaultGradient;
  return fn;
}
function buildRadialGradientFactory(focalPoint, colorStops) {
  var defaultGradient = factoryCtx.createRadialGradient(focalPoint, 0, 0, 0, 0, 1);
  for (var i = 0; i < colorStops.length; i++) {
    defaultGradient.addColorStop(colorStops[i].ratio, colorStops[i].color);
  }
  var fn = function createRadialGradient(ctx, colorTransform) {
    var gradient = ctx.createRadialGradient(focalPoint, 0, 0, 0, 0, 1);
    for (var i = 0; i < colorStops.length; i++) {
      colorTransform.addGradientColorStop(gradient, colorStops[i].ratio, colorStops[i].color);
    }
    return gradient;
  };
  fn.defaultGradient = defaultGradient;
  return fn;
}
function initStyle(style, dictionary) {
  if (style.type === undefined) {
    return;
  }
  switch (style.type) {
  case GRAPHICS_FILL_SOLID:
    break;
  case GRAPHICS_FILL_LINEAR_GRADIENT:
  case GRAPHICS_FILL_RADIAL_GRADIENT:
  case GRAPHICS_FILL_FOCAL_RADIAL_GRADIENT:
    var records = style.records, colorStops = [];
    for (var j = 0, n = records.length; j < n; j++) {
      var record = records[j];
      var colorStr = rgbaObjToStr(record.color);
      colorStops.push({
        ratio: record.ratio / 255,
        color: colorStr
      });
    }
    var gradientConstructor;
    var isLinear = style.type === GRAPHICS_FILL_LINEAR_GRADIENT;
    if (isLinear) {
      gradientConstructor = buildLinearGradientFactory(colorStops);
    } else {
      gradientConstructor = buildRadialGradientFactory((style.focalPoint | 0) / 20, colorStops);
    }
    style.style = gradientConstructor;
    break;
  case GRAPHICS_FILL_REPEATING_BITMAP:
  case GRAPHICS_FILL_CLIPPED_BITMAP:
  case GRAPHICS_FILL_NONSMOOTHED_REPEATING_BITMAP:
  case GRAPHICS_FILL_NONSMOOTHED_CLIPPED_BITMAP:
    var bitmap = dictionary[style.bitmapId];
    var repeat = style.type === GRAPHICS_FILL_REPEATING_BITMAP || style.type === GRAPHICS_FILL_NONSMOOTHED_REPEATING_BITMAP;
    style.style = factoryCtx.createPattern(bitmap.value.props.img, repeat ? 'repeat' : 'no-repeat');
    break;
  default:
    fail('invalid fill style', 'shape');
  }
}
var SOUND_SIZE_8_BIT = 0;
var SOUND_SIZE_16_BIT = 1;
var SOUND_TYPE_MONO = 0;
var SOUND_TYPE_STEREO = 1;
var SOUND_FORMAT_PCM_BE = 0;
var SOUND_FORMAT_ADPCM = 1;
var SOUND_FORMAT_MP3 = 2;
var SOUND_FORMAT_PCM_LE = 3;
var SOUND_FORMAT_NELLYMOSER_16 = 4;
var SOUND_FORMAT_NELLYMOSER_8 = 5;
var SOUND_FORMAT_NELLYMOSER = 6;
var SOUND_FORMAT_SPEEX = 11;
var SOUND_RATES = [
    5512,
    11250,
    22500,
    44100
  ];
var WaveHeader = new Uint8Array([
    82,
    73,
    70,
    70,
    0,
    0,
    0,
    0,
    87,
    65,
    86,
    69,
    102,
    109,
    116,
    32,
    16,
    0,
    0,
    0,
    1,
    0,
    2,
    0,
    68,
    172,
    0,
    0,
    16,
    177,
    2,
    0,
    4,
    0,
    16,
    0,
    100,
    97,
    116,
    97,
    0,
    0,
    0,
    0
  ]);
function packageWave(data, sampleRate, channels, size, swapBytes) {
  var sizeInBytes = size >> 3;
  var sizePerSecond = channels * sampleRate * sizeInBytes;
  var sizePerSample = channels * sizeInBytes;
  var dataLength = data.length + (data.length & 1);
  var buffer = new ArrayBuffer(WaveHeader.length + dataLength);
  var bytes = new Uint8Array(buffer);
  bytes.set(WaveHeader);
  if (swapBytes) {
    for (var i = 0, j = WaveHeader.length; i < data.length; i += 2, j += 2) {
      bytes[j] = data[i + 1];
      bytes[j + 1] = data[i];
    }
  } else {
    bytes.set(data, WaveHeader.length);
  }
  var view = new DataView(buffer);
  view.setUint32(4, dataLength + 36, true);
  view.setUint16(22, channels, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sizePerSecond, true);
  view.setUint16(32, sizePerSample, true);
  view.setUint16(34, size, true);
  view.setUint32(40, dataLength, true);
  return {
    data: bytes,
    mimeType: 'audio/wav'
  };
}
function defineSound(tag, dictionary) {
  var channels = tag.soundType == SOUND_TYPE_STEREO ? 2 : 1;
  var samplesCount = tag.samplesCount;
  var sampleRate = SOUND_RATES[tag.soundRate];
  var data = tag.soundData;
  var pcm, packaged;
  switch (tag.soundFormat) {
  case SOUND_FORMAT_PCM_BE:
    pcm = new Float32Array(samplesCount * channels);
    if (tag.soundSize == SOUND_SIZE_16_BIT) {
      for (var i = 0, j = 0; i < pcm.length; i++, j += 2)
        pcm[i] = (data[j] << 24 | data[j + 1] << 16) / 2147483648;
      packaged = packageWave(data, sampleRate, channels, 16, true);
    } else {
      for (var i = 0; i < pcm.length; i++)
        pcm[i] = (data[i] - 128) / 128;
      packaged = packageWave(data, sampleRate, channels, 8, false);
    }
    break;
  case SOUND_FORMAT_PCM_LE:
    pcm = new Float32Array(samplesCount * channels);
    if (tag.soundSize == SOUND_SIZE_16_BIT) {
      for (var i = 0, j = 0; i < pcm.length; i++, j += 2)
        pcm[i] = (data[j + 1] << 24 | data[j] << 16) / 2147483648;
      packaged = packageWave(data, sampleRate, channels, 16, false);
    } else {
      for (var i = 0; i < pcm.length; i++)
        pcm[i] = (data[i] - 128) / 128;
      packaged = packageWave(data, sampleRate, channels, 8, false);
    }
    break;
  case SOUND_FORMAT_MP3:
    packaged = {
      data: new Uint8Array(data.subarray(2)),
      mimeType: 'audio/mpeg'
    };
    break;
  case SOUND_FORMAT_ADPCM:
    var pcm16 = new Int16Array(samplesCount * channels);
    decodeACPCMSoundData(data, pcm16, channels);
    pcm = new Float32Array(samplesCount * channels);
    for (var i = 0; i < pcm.length; i++)
      pcm[i] = pcm16[i] / 32768;
    packaged = packageWave(new Uint8Array(pcm16.buffer), sampleRate, channels, 16, !new Uint8Array(new Uint16Array([
      1
    ]).buffer)[0]);
    break;
  default:
    throw new Error('Unsupported audio format: ' + tag.soundFormat);
  }
  var sound = {
      type: 'sound',
      id: tag.id,
      sampleRate: sampleRate,
      channels: channels,
      pcm: pcm
    };
  if (packaged)
    sound.packaged = packaged;
  return sound;
}
var ACPCMIndexTables = [
    [
      -1,
      2
    ],
    [
      -1,
      -1,
      2,
      4
    ],
    [
      -1,
      -1,
      -1,
      -1,
      2,
      4,
      6,
      8
    ],
    [
      -1,
      -1,
      -1,
      -1,
      -1,
      -1,
      -1,
      -1,
      1,
      2,
      4,
      6,
      8,
      10,
      13,
      16
    ]
  ];
var ACPCMStepSizeTable = [
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    16,
    17,
    19,
    21,
    23,
    25,
    28,
    31,
    34,
    37,
    41,
    45,
    50,
    55,
    60,
    66,
    73,
    80,
    88,
    97,
    107,
    118,
    130,
    143,
    157,
    173,
    190,
    209,
    230,
    253,
    279,
    307,
    337,
    371,
    408,
    449,
    494,
    544,
    598,
    658,
    724,
    796,
    876,
    963,
    1060,
    1166,
    1282,
    1411,
    1552,
    1707,
    1878,
    2066,
    2272,
    2499,
    2749,
    3024,
    3327,
    3660,
    4026,
    4428,
    4871,
    5358,
    5894,
    6484,
    7132,
    7845,
    8630,
    9493,
    10442,
    11487,
    12635,
    13899,
    15289,
    16818,
    18500,
    20350,
    22385,
    24623,
    27086,
    29794,
    32767
  ];
function decodeACPCMSoundData(data, pcm16, channels) {
  function readBits(n, signed) {
    while (dataBufferLength < n) {
      dataBuffer = dataBuffer << 8 | data[dataPosition++];
      dataBufferLength += 8;
    }
    dataBufferLength -= n;
    return dataBuffer >>> dataBufferLength & (1 << n) - 1;
  }
  var dataPosition = 0;
  var dataBuffer = 0;
  var dataBufferLength = 0;
  var pcmPosition = 0;
  var codeSize = readBits(2);
  var indexTable = ACPCMIndexTables[codeSize];
  while (pcmPosition < pcm16.length) {
    var x = pcm16[pcmPosition++] = readBits(16) << 16 >> 16, x2;
    var stepIndex = readBits(6), stepIndex2;
    if (channels > 1) {
      x2 = pcm16[pcmPosition++] = readBits(16) << 16 >> 16;
      stepIndex2 = readBits(6);
    }
    var signMask = 1 << codeSize + 1;
    for (var i = 0; i < 4095; i++) {
      var nibble = readBits(codeSize + 2);
      var step = ACPCMStepSizeTable[stepIndex];
      var sum = 0;
      for (var currentBit = signMask >> 1; currentBit; currentBit >>= 1, step >>= 1) {
        if (nibble & currentBit)
          sum += step;
      }
      x += (nibble & signMask ? -1 : 1) * (sum + step);
      pcm16[pcmPosition++] = x = x < -32768 ? -32768 : x > 32767 ? 32767 : x;
      stepIndex += indexTable[nibble & ~signMask];
      stepIndex = stepIndex < 0 ? 0 : stepIndex > 88 ? 88 : stepIndex;
      if (channels > 1) {
        nibble = readBits(codeSize + 2);
        step = ACPCMStepSizeTable[stepIndex2];
        sum = 0;
        for (var currentBit = signMask >> 1; currentBit; currentBit >>= 1, step >>= 1) {
          if (nibble & currentBit)
            sum += step;
        }
        x2 += (nibble & signMask ? -1 : 1) * (sum + step);
        pcm16[pcmPosition++] = x2 = x2 < -32768 ? -32768 : x2 > 32767 ? 32767 : x2;
        stepIndex2 += indexTable[nibble & ~signMask];
        stepIndex2 = stepIndex2 < 0 ? 0 : stepIndex2 > 88 ? 88 : stepIndex2;
      }
    }
  }
}
var nextSoundStreamId = 0;
function SwfSoundStream(samplesCount, sampleRate, channels) {
  this.streamId = nextSoundStreamId++;
  this.samplesCount = samplesCount;
  this.sampleRate = sampleRate;
  this.channels = channels;
  this.format = null;
  this.currentSample = 0;
}
SwfSoundStream.prototype = {
  get info() {
    return {
      samplesCount: this.samplesCount,
      sampleRate: this.sampleRate,
      channels: this.channels,
      format: this.format,
      streamId: this.streamId
    };
  },
  decode: function (data) {
    throw new Error('SwfSoundStream.decode: not implemented');
  }
};
function SwfSoundStream_decode_PCM(data) {
  var pcm = new Float32Array(data.length);
  for (var i = 0; i < pcm.length; i++)
    pcm[i] = (data[i] - 128) / 128;
  this.currentSample += pcm.length / this.channels;
  return {
    streamId: this.streamId,
    samplesCount: pcm.length / this.channels,
    pcm: pcm
  };
}
function SwfSoundStream_decode_PCM_be(data) {
  var pcm = new Float32Array(data.length / 2);
  for (var i = 0, j = 0; i < pcm.length; i++, j += 2)
    pcm[i] = (data[j] << 24 | data[j + 1] << 16) / 2147483648;
  this.currentSample += pcm.length / this.channels;
  return {
    streamId: this.streamId,
    samplesCount: pcm.length / this.channels,
    pcm: pcm
  };
}
function SwfSoundStream_decode_PCM_le(data) {
  var pcm = new Float32Array(data.length / 2);
  for (var i = 0, j = 0; i < pcm.length; i++, j += 2)
    pcm[i] = (data[j + 1] << 24 | data[j] << 16) / 2147483648;
  this.currentSample += pcm.length / this.channels;
  return {
    streamId: this.streamId,
    samplesCount: pcm.length / this.channels,
    pcm: pcm
  };
}
function SwfSoundStream_decode_MP3(data) {
  var samplesCount = data[1] << 8 | data[0];
  var seek = data[3] << 8 | data[2];
  this.currentSample += samplesCount;
  return {
    streamId: this.streamId,
    samplesCount: samplesCount,
    data: new Uint8Array(data.subarray(4)),
    seek: seek
  };
}
function createSoundStream(tag) {
  var channels = tag.streamType == SOUND_TYPE_STEREO ? 2 : 1;
  var samplesCount = tag.samplesCount;
  var sampleRate = SOUND_RATES[tag.streamRate];
  var stream = new SwfSoundStream(samplesCount, sampleRate, channels);
  switch (tag.streamCompression) {
  case SOUND_FORMAT_PCM_BE:
    stream.format = 'wave';
    if (tag.soundSize == SOUND_SIZE_16_BIT) {
      stream.decode = SwfSoundStream_decode_PCM_be;
    } else {
      stream.decode = SwfSoundStream_decode_PCM;
    }
    break;
  case SOUND_FORMAT_PCM_LE:
    stream.format = 'wave';
    if (tag.soundSize == SOUND_SIZE_16_BIT) {
      stream.decode = SwfSoundStream_decode_PCM_le;
    } else {
      stream.decode = SwfSoundStream_decode_PCM;
    }
    break;
  case SOUND_FORMAT_MP3:
    stream.format = 'mp3';
    stream.decode = SwfSoundStream_decode_MP3;
    break;
  default:
    throw new Error('Unsupported audio format: ' + tag.soundFormat);
  }
  return stream;
}
function defineText(tag, dictionary) {
  var dependencies = [];
  if (tag.hasFont) {
    var font = dictionary[tag.fontId];
    tag.font = font.uniqueName;
    dependencies.push(font.id);
  }
  var props = {
      type: 'text',
      id: tag.id,
      variableName: tag.variableName,
      tag: tag
    };
  if (dependencies.length)
    props.require = dependencies;
  return props;
}
var $RELEASE = false;
var LoaderDefinition = function () {
    var WORKERS_ENABLED = true;
    var LOADER_PATH;
    var workerScripts;
    if (true) {
      LOADER_PATH = 'shumway-worker.js';
    } else {
      LOADER_PATH = 'flash/display/Loader.js';
      workerScripts = [
        '../../../lib/DataView.js/DataView.js',
        '../util.js',
        '../../swf/config.js',
        '../../swf/util.js',
        '../../swf/swf.js',
        '../../swf/types.js',
        '../../swf/structs.js',
        '../../swf/tags.js',
        '../../swf/inflate.js',
        '../../swf/stream.js',
        '../../swf/templates.js',
        '../../swf/generator.js',
        '../../swf/handlers.js',
        '../../swf/parser.js',
        '../../swf/bitmap.js',
        '../../swf/button.js',
        '../../swf/font.js',
        '../../swf/image.js',
        '../../swf/label.js',
        '../../swf/shape.js',
        '../../swf/sound.js',
        '../../swf/text.js'
      ];
    }
    var isWorker = typeof window === 'undefined';
    function loadFromWorker(loader, request, context) {
      var symbols = {};
      var commitData;
      if (loader) {
        commitData = function (data) {
          return loader._commitData(data);
        };
      } else {
        commitData = function (data) {
          self.postMessage(data);
        };
      }
      function defineSymbol(swfTag) {
        var symbol;
        switch (swfTag.code) {
        case SWF_TAG_CODE_DEFINE_BITS:
        case SWF_TAG_CODE_DEFINE_BITS_JPEG2:
        case SWF_TAG_CODE_DEFINE_BITS_JPEG3:
        case SWF_TAG_CODE_DEFINE_BITS_JPEG4:
        case SWF_TAG_CODE_JPEG_TABLES:
          symbol = defineImage(swfTag, symbols);
          break;
        case SWF_TAG_CODE_DEFINE_BITS_LOSSLESS:
        case SWF_TAG_CODE_DEFINE_BITS_LOSSLESS2:
          symbol = defineBitmap(swfTag, symbols);
          break;
        case SWF_TAG_CODE_DEFINE_BUTTON:
        case SWF_TAG_CODE_DEFINE_BUTTON2:
          symbol = defineButton(swfTag, symbols);
          break;
        case SWF_TAG_CODE_DEFINE_EDIT_TEXT:
          symbol = defineText(swfTag, symbols);
          break;
        case SWF_TAG_CODE_DEFINE_FONT:
        case SWF_TAG_CODE_DEFINE_FONT2:
        case SWF_TAG_CODE_DEFINE_FONT3:
        case SWF_TAG_CODE_DEFINE_FONT4:
          symbol = defineFont(swfTag, symbols);
          break;
        case SWF_TAG_CODE_DEFINE_MORPH_SHAPE:
        case SWF_TAG_CODE_DEFINE_MORPH_SHAPE2:
        case SWF_TAG_CODE_DEFINE_SHAPE:
        case SWF_TAG_CODE_DEFINE_SHAPE2:
        case SWF_TAG_CODE_DEFINE_SHAPE3:
        case SWF_TAG_CODE_DEFINE_SHAPE4:
          symbol = defineShape(swfTag, symbols);
          break;
        case SWF_TAG_CODE_DEFINE_SOUND:
          symbol = defineSound(swfTag, symbols);
          break;
        case SWF_TAG_CODE_DEFINE_BINARY_DATA:
          symbol = {
            type: 'binary',
            id: swfTag.id,
            data: swfTag.data
          };
          break;
        case SWF_TAG_CODE_DEFINE_SPRITE:
          var depths = {};
          var frame = {
              type: 'frame'
            };
          var frames = [];
          var tags = swfTag.tags;
          var frameScripts = null;
          var frameIndex = 0;
          var soundStream = null;
          for (var i = 0, n = tags.length; i < n; i++) {
            var tag = tags[i];
            switch (tag.code) {
            case SWF_TAG_CODE_DO_ACTION:
              if (!frameScripts)
                frameScripts = [];
              frameScripts.push(frameIndex);
              frameScripts.push(tag.actionsData);
              break;
            case SWF_TAG_CODE_START_SOUND:
              var startSounds = frame.startSounds || (frame.startSounds = []);
              startSounds.push(tag);
              break;
            case SWF_TAG_CODE_SOUND_STREAM_HEAD:
              try {
                soundStream = createSoundStream(tag);
                frame.soundStream = soundStream.info;
              } catch (e) {
              }
              break;
            case SWF_TAG_CODE_SOUND_STREAM_BLOCK:
              if (soundStream) {
                frame.soundStreamBlock = soundStream.decode(tag.data);
              }
              break;
            case SWF_TAG_CODE_FRAME_LABEL:
              frame.labelName = tag.name;
              break;
            case SWF_TAG_CODE_PLACE_OBJECT:
            case SWF_TAG_CODE_PLACE_OBJECT2:
            case SWF_TAG_CODE_PLACE_OBJECT3:
              depths[tag.depth] = tag;
              break;
            case SWF_TAG_CODE_REMOVE_OBJECT:
            case SWF_TAG_CODE_REMOVE_OBJECT2:
              depths[tag.depth] = null;
              break;
            case SWF_TAG_CODE_SHOW_FRAME:
              var repeat = 1;
              while (i < n - 1) {
                var nextTag = tags[i + 1];
                if (nextTag.code !== SWF_TAG_CODE_SHOW_FRAME)
                  break;
                i++;
                repeat++;
              }
              frameIndex += repeat;
              frame.repeat = repeat;
              frame.depths = depths;
              frames.push(frame);
              depths = {};
              frame = {
                type: 'frame'
              };
              break;
            }
          }
          symbol = {
            type: 'sprite',
            id: swfTag.id,
            frameCount: swfTag.frameCount,
            frames: frames,
            frameScripts: frameScripts
          };
          break;
        case SWF_TAG_CODE_DEFINE_TEXT:
        case SWF_TAG_CODE_DEFINE_TEXT2:
          symbol = defineLabel(swfTag, symbols);
          break;
        }
        if (!symbol) {
          commitData({
            command: 'error',
            message: 'unknown symbol type: ' + swfTag.code
          });
          return;
        }
        symbol.isSymbol = true;
        symbols[swfTag.id] = symbol;
        commitData(symbol);
      }
      function createParsingContext() {
        var depths = {};
        var frame = {
            type: 'frame'
          };
        var tagsProcessed = 0;
        var soundStream = null;
        var lastProgressSent = 0;
        return {
          onstart: function (result) {
            commitData({
              command: 'init',
              result: result
            });
          },
          onprogress: function (result) {
            if (Date.now() - lastProgressSent > 1000 / 24 || result.bytesLoaded === result.bytesTotal) {
              commitData({
                command: 'progress',
                result: {
                  bytesLoaded: result.bytesLoaded,
                  bytesTotal: result.bytesTotal
                }
              });
              lastProgressSent = Date.now();
            }
            var tags = result.tags;
            for (var n = tags.length; tagsProcessed < n; tagsProcessed++) {
              var tag = tags[tagsProcessed];
              if ('id' in tag) {
                defineSymbol(tag);
                continue;
              }
              switch (tag.code) {
              case SWF_TAG_CODE_DEFINE_SCENE_AND_FRAME_LABEL_DATA:
                frame.sceneData = tag;
                break;
              case SWF_TAG_CODE_DEFINE_SCALING_GRID:
                var symbolUpdate = {
                    isSymbol: true,
                    id: tag.symbolId,
                    updates: {
                      scale9Grid: tag.splitter
                    }
                  };
                commitData(symbolUpdate);
                break;
              case SWF_TAG_CODE_DO_ABC:
              case SWF_TAG_CODE_DO_ABC_:
                var abcBlocks = frame.abcBlocks;
                if (abcBlocks)
                  abcBlocks.push({
                    data: tag.data,
                    flags: tag.flags
                  });
                else
                  frame.abcBlocks = [
                    {
                      data: tag.data,
                      flags: tag.flags
                    }
                  ];
                break;
              case SWF_TAG_CODE_DO_ACTION:
                var actionBlocks = frame.actionBlocks;
                if (actionBlocks)
                  actionBlocks.push(tag.actionsData);
                else
                  frame.actionBlocks = [
                    tag.actionsData
                  ];
                break;
              case SWF_TAG_CODE_DO_INIT_ACTION:
                var initActionBlocks = frame.initActionBlocks || (frame.initActionBlocks = []);
                initActionBlocks.push({
                  spriteId: tag.spriteId,
                  actionsData: tag.actionsData
                });
                break;
              case SWF_TAG_CODE_START_SOUND:
                var startSounds = frame.startSounds;
                if (!startSounds)
                  frame.startSounds = startSounds = [];
                startSounds.push(tag);
                break;
              case SWF_TAG_CODE_SOUND_STREAM_HEAD:
                try {
                  soundStream = createSoundStream(tag);
                  frame.soundStream = soundStream.info;
                } catch (e) {
                }
                break;
              case SWF_TAG_CODE_SOUND_STREAM_BLOCK:
                if (soundStream) {
                  frame.soundStreamBlock = soundStream.decode(tag.data);
                }
                break;
              case SWF_TAG_CODE_SYMBOL_CLASS:
                var exports = frame.exports;
                if (exports)
                  frame.exports = exports.concat(tag.exports);
                else
                  frame.exports = tag.exports.slice(0);
                break;
              case SWF_TAG_CODE_FRAME_LABEL:
                frame.labelName = tag.name;
                break;
              case SWF_TAG_CODE_PLACE_OBJECT:
              case SWF_TAG_CODE_PLACE_OBJECT2:
              case SWF_TAG_CODE_PLACE_OBJECT3:
                depths[tag.depth] = tag;
                break;
              case SWF_TAG_CODE_REMOVE_OBJECT:
              case SWF_TAG_CODE_REMOVE_OBJECT2:
                depths[tag.depth] = null;
                break;
              case SWF_TAG_CODE_SET_BACKGROUND_COLOR:
                frame.bgcolor = tag.color;
                break;
              case SWF_TAG_CODE_SHOW_FRAME:
                var repeat = 1;
                while (tagsProcessed < n) {
                  var nextTag = tags[tagsProcessed + 1];
                  if (!nextTag || nextTag.code !== SWF_TAG_CODE_SHOW_FRAME)
                    break;
                  tagsProcessed++;
                  repeat++;
                }
                frame.repeat = repeat;
                frame.depths = depths;
                commitData(frame);
                depths = {};
                frame = {
                  type: 'frame'
                };
                break;
              }
            }
          },
          oncomplete: function (result) {
            commitData(result);
            commitData({
              command: 'complete'
            });
          }
        };
      }
      function parseBytes(bytes) {
        SWF.parse(bytes, createParsingContext());
      }
      if (isWorker || !flash.net.URLRequest.class.isInstanceOf(request)) {
        var input = request;
        if (input instanceof ArrayBuffer) {
          parseBytes(input);
        } else if ('subscribe' in input) {
          var pipe = SWF.parseAsync(createParsingContext());
          input.subscribe(function (data, progress) {
            if (data) {
              pipe.push(data, progress);
            } else {
              pipe.close();
            }
          });
        } else if (typeof FileReaderSync !== 'undefined') {
          var reader = new FileReaderSync();
          var buffer = reader.readAsArrayBuffer(input);
          parseBytes(buffer);
        } else {
          var reader = new FileReader();
          reader.onload = function () {
            parseBytes(this.result);
          };
          reader.readAsArrayBuffer(input);
        }
      } else {
        var session = FileLoadingService.createSession();
        var pipe = SWF.parseAsync(createParsingContext());
        session.onprogress = function (data, progressState) {
          pipe.push(data, progressState);
          var data = {
              command: 'progress',
              result: {
                bytesLoaded: progressState.bytesLoaded,
                bytesTotal: progressState.bytesTotal
              }
            };
          loader._commitData(data);
        };
        session.onerror = function (error) {
          loader._commitData({
            command: 'error',
            error: error
          });
        };
        session.onopen = function () {
        };
        session.onclose = function () {
          pipe.close();
        };
        session.open(request._toFileRequest());
      }
    }
    if (isWorker) {
      if (workerScripts) {
        importScripts.apply(null, workerScripts);
      }
      var subscription = null;
      self.onmessage = function (evt) {
        if (subscription) {
          subscription.callback(evt.data.data, evt.data.progress);
        } else if (evt.data === 'pipe:') {
          subscription = {
            subscribe: function (callback) {
              this.callback = callback;
            }
          };
          loadFromWorker(null, subscription);
        } else {
          loadFromWorker(null, evt.data);
        }
      };
      return;
    }
    var head = document.head;
    head.insertBefore(document.createElement('style'), head.firstChild);
    var style = document.styleSheets[0];
    var def = {
        __class__: 'flash.display.Loader',
        initialize: function () {
          this._contentLoaderInfo = new flash.display.LoaderInfo();
          this._contentLoaderInfo._loader = this;
          this._dictionary = {};
          this._displayList = null;
          this._timeline = [];
          this._lastPromise = null;
          this._uncaughtErrorEvents = null;
          this._worker = null;
        },
        _commitData: function (data) {
          switch (data.command) {
          case 'init':
            this._init(data.result);
            break;
          case 'progress':
            this._updateProgress(data.result);
            break;
          case 'complete':
            var frameConstructed = new Promise();
            avm2.systemDomain.onMessage.register('frameConstructed', function waitForFrame(type) {
              if (type === 'frameConstructed') {
                frameConstructed.resolve();
                avm2.systemDomain.onMessage.unregister('frameConstructed', waitForFrame);
              }
            });
            Promise.when(frameConstructed, this._lastPromise).then(function () {
              this.contentLoaderInfo._dispatchEvent('complete');
            }.bind(this));
            this._worker && this._worker.terminate();
            break;
          case 'empty':
            this._lastPromise = new Promise();
            this._lastPromise.resolve();
            break;
          case 'error':
            this.contentLoaderInfo._dispatchEvent('ioError', flash.events.IOErrorEvent);
            break;
          default:
            if (data.id === 0)
              break;
            if (data.isSymbol)
              this._commitSymbol(data);
            else if (data.type === 'frame')
              this._commitFrame(data);
            else if (data.type === 'image')
              this._commitImage(data);
            break;
          }
        },
        _updateProgress: function (state) {
          var loaderInfo = this.contentLoaderInfo;
          loaderInfo._bytesLoaded = state.bytesLoaded || 0;
          loaderInfo._bytesTotal = state.bytesTotal || 0;
          var event = new flash.events.ProgressEvent('progress', false, false, loaderInfo._bytesLoaded, loaderInfo._bytesTotal);
          loaderInfo._dispatchEvent(event);
        },
        _buildFrame: function (currentDisplayList, timeline, promiseQueue, frame, frameNum) {
          var loader = this;
          var dictionary = loader._dictionary;
          var displayList = {};
          var depths = [];
          var cmds = frame.depths;
          if (currentDisplayList) {
            var currentDepths = currentDisplayList.depths;
            for (var i = 0; i < currentDepths.length; i++) {
              var depth = currentDepths[i];
              if (cmds[depth] === null) {
                continue;
              }
              displayList[depth] = currentDisplayList[depth];
              depths.push(depth);
            }
          }
          for (var depth in cmds) {
            var cmd = cmds[depth];
            if (!cmd) {
              continue;
            }
            if (cmd.move) {
              var oldCmd = cmd;
              cmd = cloneObject(currentDisplayList[depth]);
              for (var prop in oldCmd) {
                var val = oldCmd[prop];
                if (val) {
                  cmd[prop] = val;
                }
              }
            }
            if (cmd.symbolId) {
              var itemPromise = dictionary[cmd.symbolId];
              if (itemPromise && !itemPromise.resolved) {
                promiseQueue.push(itemPromise);
              }
              cmd = cloneObject(cmd);
              cmd.promise = itemPromise;
            }
            if (!displayList[depth]) {
              depths.push(depth);
            }
            displayList[depth] = cmd;
          }
          depths.sort(sortNumeric);
          displayList.depths = depths;
          var i = frame.repeat;
          while (i--) {
            timeline.push(displayList);
          }
          return displayList;
        },
        _commitFrame: function (frame) {
          var abcBlocks = frame.abcBlocks;
          var actionBlocks = frame.actionBlocks;
          var initActionBlocks = frame.initActionBlocks;
          var exports = frame.exports;
          var sceneData = frame.sceneData;
          var loader = this;
          var dictionary = loader._dictionary;
          var loaderInfo = loader.contentLoaderInfo;
          var timeline = loader._timeline;
          var frameNum = timeline.length + 1;
          var framePromise = new Promise();
          var labelName = frame.labelName;
          var prevPromise = this._lastPromise;
          this._lastPromise = framePromise;
          var promiseQueue = [
              prevPromise
            ];
          this._displayList = this._buildFrame(this._displayList, timeline, promiseQueue, frame, frameNum);
          if (frame.bgcolor)
            loaderInfo._backgroundColor = frame.bgcolor;
          else if (isNullOrUndefined(loaderInfo._backgroundColor))
            loaderInfo._backgroundColor = {
              red: 255,
              green: 255,
              blue: 255,
              alpha: 255
            };
          Promise.when.apply(Promise, promiseQueue).then(function () {
            if (abcBlocks && loader._isAvm2Enabled) {
              var appDomain = avm2.applicationDomain;
              for (var i = 0, n = abcBlocks.length; i < n; i++) {
                var abc = new AbcFile(abcBlocks[i].data, 'abc_block_' + i);
                if (abcBlocks[i].flags) {
                  appDomain.loadAbc(abc);
                } else {
                  appDomain.executeAbc(abc);
                }
              }
            }
            if (exports && loader._isAvm2Enabled) {
              var exportPromises = [];
              for (var i = 0, n = exports.length; i < n; i++) {
                var asset = exports[i];
                var symbolPromise = dictionary[asset.symbolId];
                if (!symbolPromise)
                  continue;
                symbolPromise.then(function (symbolPromise, className) {
                  return function symbolPromiseResolved() {
                    var symbolInfo = symbolPromise.value;
                    symbolInfo.className = className;
                    avm2.applicationDomain.getClass(className).setSymbol(symbolInfo.props);
                  };
                }(symbolPromise, asset.className));
                exportPromises.push(symbolPromise);
              }
              return Promise.when.apply(Promise, exportPromises);
            }
          }).then(function () {
            var root = loader._content;
            var labelMap;
            if (!root) {
              var parent = loader._parent;
              true;
              var rootInfo = dictionary[0].value;
              var rootClass = avm2.applicationDomain.getClass(rootInfo.className);
              root = rootClass.createAsSymbol({
                framesLoaded: timeline.length,
                loader: loader,
                parent: parent || loader,
                index: parent ? 0 : -1,
                level: parent ? 0 : -1,
                timeline: timeline,
                totalFrames: rootInfo.props.totalFrames,
                stage: loader._stage
              });
              var isRootMovie = parent && parent == loader._stage && loader._stage._children.length === 0;
              if (isRootMovie) {
                parent._frameRate = loaderInfo._frameRate;
                parent._stageHeight = loaderInfo._height;
                parent._stageWidth = loaderInfo._width;
                parent._root = root;
                parent._setup();
              } else {
                loader._children.push(root);
              }
              var labels;
              labelMap = root.symbol.labelMap = createEmptyObject();
              if (sceneData) {
                var scenes = [];
                var startFrame;
                var endFrame = root.symbol.totalFrames - 1;
                var sd = sceneData.scenes;
                var ld = sceneData.labels;
                var i = sd.length;
                while (i--) {
                  var s = sd[i];
                  startFrame = s.offset;
                  labels = [];
                  var j = ld.length;
                  while (j--) {
                    var lbl = ld[j];
                    if (lbl.frame >= startFrame && lbl.frame <= endFrame) {
                      labelMap[lbl.name] = lbl.frame + 1;
                      labels.unshift(new flash.display.FrameLabel(lbl.name, lbl.frame - startFrame + 1));
                    }
                  }
                  var scene = new flash.display.Scene(s.name, labels, endFrame - startFrame + 1);
                  scene._startFrame = startFrame + 1;
                  scene._endFrame = endFrame + 1;
                  scenes.unshift(scene);
                  endFrame = startFrame - 1;
                }
                root.symbol.scenes = scenes;
              } else {
                labels = [];
                if (labelName) {
                  labelMap[labelName] = frameNum;
                  labels.push(new flash.display.FrameLabel(labelName, frameNum));
                }
                var scene = new flash.display.Scene('Scene 1', labels, root.symbol.totalFrames);
                scene._endFrame = root.symbol.totalFrames;
                root.symbol.scenes = [
                  scene
                ];
              }
              if (!loader._isAvm2Enabled) {
                var avm1Context = loader._avm1Context;
                var _root = root;
                if (parent && parent !== loader._stage) {
                  var parentLoader = parent._loader;
                  while (parentLoader._parent && parentLoader._parent !== loader._stage) {
                    parentLoader = parentLoader._parent._loader;
                  }
                  _root = parentLoader._content;
                }
                var as2Object = _root._getAS2Object();
                avm1Context.globals.asSetPublicProperty('_root', as2Object);
                avm1Context.globals.asSetPublicProperty('_level0', as2Object);
                avm1Context.globals.asSetPublicProperty('_level1', as2Object);
                var parameters = loader.loaderInfo._parameters;
                for (var paramName in parameters) {
                  if (!(paramName in as2Object)) {
                    as2Object[paramName] = parameters[paramName];
                  }
                }
              }
              rootClass.instanceConstructor.call(root);
              loader._content = root;
            } else {
              root._framesLoaded += frame.repeat;
              if (labelName && root._labelMap) {
                if (root._labelMap[labelName] === undefined) {
                  root._labelMap[labelName] = frameNum;
                  for (var i = 0, n = root.symbol.scenes.length; i < n; i++) {
                    var scene = root.symbol.scenes[i];
                    if (frameNum >= scene._startFrame && frameNum <= scene._endFrame) {
                      scene.labels.push(new flash.display.FrameLabel(labelName, frameNum - scene._startFrame));
                      break;
                    }
                  }
                }
              }
            }
            if (frame.startSounds) {
              root._registerStartSounds(frameNum, frame.startSounds);
            }
            if (frame.soundStream) {
              root._initSoundStream(frame.soundStream);
            }
            if (frame.soundStreamBlock) {
              root._addSoundStreamBlock(frameNum, frame.soundStreamBlock);
            }
            if (!loader._isAvm2Enabled) {
              var avm1Context = loader._avm1Context;
              if (initActionBlocks) {
                for (var i = 0; i < initActionBlocks.length; i++) {
                  var spriteId = initActionBlocks[i].spriteId;
                  var actionsData = initActionBlocks[i].actionsData;
                  root.addFrameScript(frameNum - 1, function (actionsData, spriteId, state) {
                    if (state.executed)
                      return;
                    state.executed = true;
                    return executeActions(actionsData, avm1Context, this._getAS2Object(), exports);
                  }.bind(root, actionsData, spriteId, {
                    executed: false
                  }));
                }
              }
              if (actionBlocks) {
                for (var i = 0; i < actionBlocks.length; i++) {
                  var block = actionBlocks[i];
                  root.addFrameScript(frameNum - 1, function (block) {
                    return function () {
                      return executeActions(block, avm1Context, this._getAS2Object(), exports);
                    };
                  }(block));
                }
              }
            }
            if (frameNum === 1)
              loaderInfo._dispatchEvent(new flash.events.Event('init', false, false));
            framePromise.resolve(frame);
          });
        },
        _commitImage: function (imageInfo) {
          var loader = this;
          var imgPromise = this._lastPromise = new Promise();
          var img = new Image();
          imageInfo.props.img = img;
          img.onload = function () {
            var props = imageInfo.props;
            props.parent = loader._parent;
            props.stage = loader._stage;
            props.skipCopyToCanvas = true;
            var Bitmap = avm2.systemDomain.getClass('flash.display.Bitmap');
            var BitmapData = avm2.systemDomain.getClass('flash.display.BitmapData');
            var bitmapData = BitmapData.createAsSymbol(props);
            BitmapData.instanceConstructor.call(bitmapData, 0, 0, true, 4294967295);
            var image = Bitmap.createAsSymbol(bitmapData);
            loader._children.push(image);
            Bitmap.instanceConstructor.call(image, bitmapData);
            image._parent = loader;
            loader._content = image;
            imgPromise.resolve(imageInfo);
            loader.contentLoaderInfo._dispatchEvent('init');
          };
          img.src = URL.createObjectURL(imageInfo.data);
          delete imageInfo.data;
        },
        _commitSymbol: function (symbol) {
          var dictionary = this._dictionary;
          if ('updates' in symbol) {
            dictionary[symbol.id].then(function (s) {
              for (var i in symbol.updates) {
                s.props[i] = symbol.updates[i];
              }
            });
            return;
          }
          var className = 'flash.display.DisplayObject';
          var dependencies = symbol.require;
          var promiseQueue = [];
          var props = {
              symbolId: symbol.id,
              loader: this
            };
          var symbolPromise = new Promise();
          if (dependencies && dependencies.length) {
            for (var i = 0, n = dependencies.length; i < n; i++) {
              var dependencyId = dependencies[i];
              var dependencyPromise = dictionary[dependencyId];
              if (dependencyPromise && !dependencyPromise.resolved)
                promiseQueue.push(dependencyPromise);
            }
          }
          switch (symbol.type) {
          case 'button':
            var states = {};
            for (var stateName in symbol.states) {
              var characters = [];
              var displayList = {};
              var state = symbol.states[stateName];
              var depths = Object.keys(state);
              for (var i = 0; i < depths.length; i++) {
                var depth = depths[i];
                var cmd = state[depth];
                var characterPromise = dictionary[cmd.symbolId];
                if (characterPromise && !characterPromise.resolved)
                  promiseQueue.push(characterPromise);
                characters.push(characterPromise);
                displayList[depth] = Object.create(cmd, {
                  promise: {
                    value: characterPromise
                  }
                });
              }
              depths.sort(sortNumeric);
              displayList.depths = depths;
              var statePromise = new Promise();
              statePromise.resolve({
                className: 'flash.display.Sprite',
                props: {
                  loader: this,
                  timeline: [
                    displayList
                  ]
                }
              });
              states[stateName] = statePromise;
            }
            className = 'flash.display.SimpleButton';
            props.states = states;
            props.buttonActions = symbol.buttonActions;
            break;
          case 'font':
            var charset = fromCharCode.apply(null, symbol.codes);
            if (charset) {
              style.insertRule('@font-face{font-family:"' + symbol.uniqueName + '";' + 'src:url(data:font/opentype;base64,' + btoa(symbol.data) + ')' + '}', style.cssRules.length);
              if (!/Mozilla\/5.0.*?rv:(\d+).*? Gecko/.test(window.navigator.userAgent)) {
                var testDiv = document.createElement('div');
                testDiv.setAttribute('style', 'position: absolute; top: 0; right: 0;visibility: hidden; z-index: -500;font-family:"' + symbol.uniqueName + '";');
                testDiv.textContent = 'font test';
                document.body.appendChild(testDiv);
                var fontPromise = new Promise();
                setTimeout(function () {
                  fontPromise.resolve();
                  document.body.removeChild(testDiv);
                }, 200);
                promiseQueue.push(fontPromise);
              }
            }
            className = 'flash.text.Font';
            props.name = symbol.name;
            props.uniqueName = symbol.uniqueName;
            props.charset = symbol.charset;
            props.metrics = symbol.metrics;
            this._registerFont(className, props);
            break;
          case 'image':
            var img = new Image();
            var imgPromise = new Promise();
            img.onload = function () {
              if (symbol.mask) {
                var maskCanvas = document.createElement('canvas');
                maskCanvas.width = symbol.width;
                maskCanvas.height = symbol.height;
                var maskContext = maskCanvas.getContext('2d');
                maskContext.drawImage(img, 0, 0);
                var maskImageData = maskContext.getImageData(0, 0, symbol.width, symbol.height);
                var maskImageDataBytes = maskImageData.data;
                var symbolMaskBytes = symbol.mask;
                var length = maskImageData.width * maskImageData.height;
                for (var i = 0, j = 3; i < length; i++, j += 4) {
                  maskImageDataBytes[j] = symbolMaskBytes[i];
                }
                maskContext.putImageData(maskImageData, 0, 0);
                props.img = maskCanvas;
              }
              imgPromise.resolve();
            };
            img.src = URL.createObjectURL(symbol.data);
            promiseQueue.push(imgPromise);
            className = 'flash.display.Bitmap';
            props.img = img;
            props.width = symbol.width;
            props.height = symbol.height;
            break;
          case 'label':
            var drawFn = new Function('c,r,ct', symbol.data);
            className = 'flash.text.StaticText';
            props.bbox = symbol.bbox;
            props.draw = drawFn;
            break;
          case 'text':
            props.bbox = symbol.bbox;
            props.html = symbol.html;
            if (symbol.type === 'label') {
              className = 'flash.text.StaticText';
            } else {
              className = 'flash.text.TextField';
              props.tag = symbol.tag;
              props.variableName = symbol.variableName;
            }
            break;
          case 'shape':
            className = symbol.morph ? 'flash.display.MorphShape' : 'flash.display.Shape';
            props.bbox = symbol.bbox;
            props.strokeBbox = symbol.strokeBbox;
            props.paths = symbol.paths;
            props.dictionary = dictionary;
            break;
          case 'sound':
            if (!symbol.pcm && !PLAY_USING_AUDIO_TAG) {
              var decodePromise = new Promise();
              MP3DecoderSession.processAll(symbol.packaged.data, function (props, pcm, id3tags, error) {
                props.pcm = pcm || new Uint8Array(0);
                decodePromise.resolve();
                if (error) {
                  console.error('ERROR: ' + error);
                }
              }.bind(null, props));
              promiseQueue.push(decodePromise);
            }
            className = 'flash.media.Sound';
            props.sampleRate = symbol.sampleRate;
            props.channels = symbol.channels;
            props.pcm = symbol.pcm;
            props.packaged = symbol.packaged;
            break;
          case 'binary':
            props.data = symbol.data;
            break;
          case 'sprite':
            var displayList = null;
            var frameCount = symbol.frameCount;
            var labelMap = {};
            var frameNum = 1;
            var frames = symbol.frames;
            var timeline = [];
            var startSoundRegistrations = [];
            for (var i = 0, n = frames.length; i < n; i++) {
              var frame = frames[i];
              var frameNum = timeline.length + 1;
              if (frame.labelName) {
                labelMap[frame.labelName] = frameNum;
              }
              if (frame.startSounds) {
                startSoundRegistrations[frameNum] = frame.startSounds;
                for (var j = 0; j < frame.startSounds.length; j++) {
                  var itemPromise = dictionary[frame.startSounds[j].soundId];
                  if (itemPromise && !itemPromise.resolved)
                    promiseQueue.push(itemPromise);
                }
              }
              displayList = this._buildFrame(displayList, timeline, promiseQueue, frame, frameNum);
            }
            var frameScripts = {};
            if (!this._isAvm2Enabled) {
              if (symbol.frameScripts) {
                var data = symbol.frameScripts;
                for (var i = 0; i < data.length; i += 2) {
                  var frameNum = data[i] + 1;
                  var block = data[i + 1];
                  var script = function (block, loader) {
                      return function () {
                        var avm1Context = loader._avm1Context;
                        return executeActions(block, avm1Context, this._getAS2Object());
                      };
                    }(block, this);
                  if (!frameScripts[frameNum])
                    frameScripts[frameNum] = [
                      script
                    ];
                  else
                    frameScripts[frameNum].push(script);
                }
              }
            }
            className = 'flash.display.MovieClip';
            props.timeline = timeline;
            props.framesLoaded = frameCount;
            props.labelMap = labelMap;
            props.frameScripts = frameScripts;
            props.totalFrames = frameCount;
            props.startSoundRegistrations = startSoundRegistrations;
            break;
          }
          dictionary[symbol.id] = symbolPromise;
          Promise.when.apply(Promise, promiseQueue).then(function () {
            symbolPromise.resolve({
              className: className,
              props: props
            });
          });
        },
        _registerFont: function (className, props) {
          this._vmPromise.then(function () {
            var fontClass = avm2.applicationDomain.getClass(className);
            var font = fontClass.createAsSymbol(props);
            fontClass.instanceConstructor.call(font);
          });
        },
        _init: function (info) {
          var loader = this;
          var loaderInfo = loader.contentLoaderInfo;
          loaderInfo._swfVersion = info.swfVersion;
          var bbox = info.bbox;
          loaderInfo._width = bbox.xMax - bbox.xMin;
          loaderInfo._height = bbox.yMax - bbox.yMin;
          loaderInfo._frameRate = info.frameRate;
          var documentPromise = new Promise();
          var vmPromise = new Promise();
          vmPromise.then(function () {
            documentPromise.resolve({
              className: 'flash.display.MovieClip',
              props: {
                totalFrames: info.frameCount
              }
            });
          });
          loader._dictionary[0] = documentPromise;
          loader._lastPromise = documentPromise;
          loader._vmPromise = vmPromise;
          loader._isAvm2Enabled = info.fileAttributes.doAbc;
          this._setup();
        },
        _load: function (request, checkPolicyFile, applicationDomain, securityDomain, deblockingFilter) {
          if (!isWorker && WORKERS_ENABLED) {
            var loader = this;
            var worker = loader._worker = new Worker(SHUMWAY_ROOT + LOADER_PATH);
            worker.onmessage = function (evt) {
              loader._commitData(evt.data);
            };
            if (flash.net.URLRequest.class.isInstanceOf(request)) {
              var session = FileLoadingService.createSession();
              session.onprogress = function (data, progress) {
                worker.postMessage({
                  data: data,
                  progress: progress
                });
              };
              session.onerror = function (error) {
                loader._commitData({
                  command: 'error',
                  error: error
                });
              };
              session.onopen = function () {
                worker.postMessage('pipe:');
              };
              session.onclose = function () {
                worker.postMessage({
                  data: null
                });
              };
              session.open(request._toFileRequest());
            } else {
              worker.postMessage(request);
            }
          } else {
            loadFromWorker(this, request);
          }
        },
        _setup: function () {
          var loader = this;
          var stage = loader._stage;
          if (loader._isAvm2Enabled) {
            var mouseClass = avm2.systemDomain.getClass('flash.ui.Mouse');
            mouseClass._stage = stage;
            loader._vmPromise.resolve();
            return;
          }
          if (!avm2.loadAVM1) {
            loader._vmPromise.reject('AVM1 loader is not found');
            return;
          }
          var loaded = function () {
            var loaderInfo = loader.contentLoaderInfo;
            var avm1Context = new AS2Context(loaderInfo._swfVersion);
            avm1Context.stage = stage;
            loader._avm1Context = avm1Context;
            avm1lib.AS2Key.class.$bind(stage);
            avm1lib.AS2Mouse.class.$bind(stage);
            stage._addEventListener('frameConstructed', avm1Context.flushPendingScripts.bind(avm1Context), false, Number.MAX_VALUE);
            loader._vmPromise.resolve();
          };
          if (avm2.isAVM1Loaded) {
            loaded();
          } else {
            avm2.isAVM1Loaded = true;
            avm2.loadAVM1(loaded);
          }
        },
        get contentLoaderInfo() {
          return this._contentLoaderInfo;
        },
        get content() {
          somewhatImplemented('Loader.content');
          return this._content;
        }
      };
    def.__glue__ = {
      native: {
        instance: {
          content: Object.getOwnPropertyDescriptor(def, 'content'),
          contentLoaderInfo: Object.getOwnPropertyDescriptor(def, 'contentLoaderInfo'),
          _getJPEGLoaderContextdeblockingfilter: function (context) {
            return 0;
          },
          _load: def._load,
          _loadBytes: function _loadBytes(bytes, checkPolicyFile, applicationDomain, securityDomain, requestedContentParent, parameters, deblockingFilter, allowLoadBytesCodeExecution, imageDecodingPolicy) {
            def._load(bytes.a);
          },
          _unload: function _unload(halt, gc) {
            notImplemented('Loader._unload');
          },
          _close: function _close() {
            somewhatImplemented('Loader._close');
          },
          _getUncaughtErrorEvents: function _getUncaughtErrorEvents() {
            somewhatImplemented('Loader._getUncaughtErrorEvents');
            return this._uncaughtErrorEvents;
          },
          _setUncaughtErrorEvents: function _setUncaughtErrorEvents(value) {
            somewhatImplemented('Loader._setUncaughtErrorEvents');
            this._uncaughtErrorEvents = value;
          }
        }
      }
    };
    return def;
  }.call(this);
var codeLengthOrder = [
    16,
    17,
    18,
    0,
    8,
    7,
    9,
    6,
    10,
    5,
    11,
    4,
    12,
    3,
    13,
    2,
    14,
    1,
    15
  ];
var distanceCodes = [];
var distanceExtraBits = [];
for (var i = 0, j = 0, code = 1; i < 30; ++i) {
  distanceCodes[i] = code;
  code += 1 << (distanceExtraBits[i] = ~(~((j += i > 2 ? 1 : 0) / 2)));
}
var bitLengths = [];
for (var i = 0; i < 32; ++i)
  bitLengths[i] = 5;
var fixedDistanceTable = makeHuffmanTable(bitLengths);
var lengthCodes = [];
var lengthExtraBits = [];
for (var i = 0, j = 0, code = 3; i < 29; ++i) {
  lengthCodes[i] = code - (i == 28 ? 1 : 0);
  code += 1 << (lengthExtraBits[i] = ~(~((j += i > 4 ? 1 : 0) / 4 % 6)));
}
for (var i = 0; i < 288; ++i)
  bitLengths[i] = i < 144 || i > 279 ? 8 : i < 256 ? 9 : 7;
var fixedLiteralTable = makeHuffmanTable(bitLengths);
function makeHuffmanTable(bitLengths) {
  var maxBits = max.apply(null, bitLengths);
  var numLengths = bitLengths.length;
  var size = 1 << maxBits;
  var codes = new Uint32Array(size);
  for (var code = 0, len = 1, skip = 2; len <= maxBits; code <<= 1, ++len, skip <<= 1) {
    for (var val = 0; val < numLengths; ++val) {
      if (bitLengths[val] === len) {
        var lsb = 0;
        for (var i = 0; i < len; ++i)
          lsb = lsb * 2 + (code >> i & 1);
        for (var i = lsb; i < size; i += skip)
          codes[i] = len << 16 | val;
        ++code;
      }
    }
  }
  return {
    codes: codes,
    maxBits: maxBits
  };
}
function verifyDeflateHeader(bytes) {
  var header = bytes[0] << 8 | bytes[1];
}
function createInflatedStream(bytes, outputLength) {
  verifyDeflateHeader(bytes);
  var stream = new Stream(bytes, 2);
  var output = {
      data: new Uint8Array(outputLength),
      available: 0,
      completed: false
    };
  var state = {};
  do {
    inflateBlock(stream, output, state);
  } while (!output.completed && stream.pos < stream.end);
  return new Stream(output.data, 0, output.available);
}
var InflateNoDataError = {};
function inflateBlock(stream, output, state) {
  var header = state.header !== undefined ? state.header : state.header = readBits(stream.bytes, stream, 3);
  switch (header >> 1) {
  case 0:
    stream.align();
    var pos = stream.pos;
    if (stream.end - pos < 4) {
      throw InflateNoDataError;
    }
    var len = stream.getUint16(pos, true);
    var nlen = stream.getUint16(pos + 2, true);
    if (stream.end - pos < 4 + len) {
      throw InflateNoDataError;
    }
    var begin = pos + 4;
    var end = stream.pos = begin + len;
    var sbytes = stream.bytes, dbytes = output.data;
    splice.apply(dbytes, [
      output.available,
      len
    ].concat(slice.call(sbytes, begin, end)));
    output.available += len;
    break;
  case 1:
    inflate(stream, output, fixedLiteralTable, fixedDistanceTable, state);
    break;
  case 2:
    var distanceTable, literalTable;
    if (state.distanceTable !== undefined) {
      distanceTable = state.distanceTable;
      literalTable = state.literalTable;
    } else {
      var sbytes = stream.bytes;
      var savedBufferPos = stream.pos;
      var savedBitBuffer = stream.bitBuffer;
      var savedBitLength = stream.bitLength;
      var bitLengths = [];
      var numLiteralCodes, numDistanceCodes;
      try {
        numLiteralCodes = readBits(sbytes, stream, 5) + 257;
        numDistanceCodes = readBits(sbytes, stream, 5) + 1;
        var numCodes = numLiteralCodes + numDistanceCodes;
        var numLengthCodes = readBits(sbytes, stream, 4) + 4;
        for (var i = 0; i < 19; ++i)
          bitLengths[codeLengthOrder[i]] = i < numLengthCodes ? readBits(sbytes, stream, 3) : 0;
        var codeLengthTable = makeHuffmanTable(bitLengths);
        bitLengths = [];
        var i = 0;
        var prev = 0;
        while (i < numCodes) {
          var j = 1;
          var sym = readCode(sbytes, stream, codeLengthTable);
          switch (sym) {
          case 16:
            j = readBits(sbytes, stream, 2) + 3;
            sym = prev;
            break;
          case 17:
            j = readBits(sbytes, stream, 3) + 3;
            sym = 0;
            break;
          case 18:
            j = readBits(sbytes, stream, 7) + 11;
            sym = 0;
            break;
          default:
            prev = sym;
          }
          while (j--)
            bitLengths[i++] = sym;
        }
      } catch (e) {
        stream.pos = savedBufferPos;
        stream.bitBuffer = savedBitBuffer;
        stream.bitLength = savedBitLength;
        throw e;
      }
      distanceTable = state.distanceTable = makeHuffmanTable(bitLengths.splice(numLiteralCodes, numDistanceCodes));
      literalTable = state.literalTable = makeHuffmanTable(bitLengths);
    }
    inflate(stream, output, literalTable, distanceTable, state);
    delete state.distanceTable;
    delete state.literalTable;
    break;
  default:
    fail('unknown block type', 'inflate');
  }
  delete state.header;
  output.completed = !(!(header & 1));
}
function readBits(bytes, stream, size) {
  var bitBuffer = stream.bitBuffer;
  var bitLength = stream.bitLength;
  if (size > bitLength) {
    var pos = stream.pos;
    var end = stream.end;
    do {
      if (pos >= end) {
        stream.pos = pos;
        stream.bitBuffer = bitBuffer;
        stream.bitLength = bitLength;
        throw InflateNoDataError;
      }
      bitBuffer |= bytes[pos++] << bitLength;
      bitLength += 8;
    } while (size > bitLength);
    stream.pos = pos;
  }
  stream.bitBuffer = bitBuffer >>> size;
  stream.bitLength = bitLength - size;
  return bitBuffer & (1 << size) - 1;
}
function inflate(stream, output, literalTable, distanceTable, state) {
  var pos = output.available;
  var dbytes = output.data;
  var sbytes = stream.bytes;
  var sym = state.sym !== undefined ? state.sym : readCode(sbytes, stream, literalTable);
  while (sym !== 256) {
    if (sym < 256) {
      dbytes[pos++] = sym;
    } else {
      state.sym = sym;
      sym -= 257;
      var len = state.len !== undefined ? state.len : state.len = lengthCodes[sym] + readBits(sbytes, stream, lengthExtraBits[sym]);
      var sym2 = state.sym2 !== undefined ? state.sym2 : state.sym2 = readCode(sbytes, stream, distanceTable);
      var distance = distanceCodes[sym2] + readBits(sbytes, stream, distanceExtraBits[sym2]);
      var i = pos - distance;
      while (len--)
        dbytes[pos++] = dbytes[i++];
      delete state.sym2;
      delete state.len;
      delete state.sym;
    }
    output.available = pos;
    sym = readCode(sbytes, stream, literalTable);
  }
}
function readCode(bytes, stream, codeTable) {
  var bitBuffer = stream.bitBuffer;
  var bitLength = stream.bitLength;
  var maxBits = codeTable.maxBits;
  if (maxBits > bitLength) {
    var pos = stream.pos;
    var end = stream.end;
    do {
      if (pos >= end) {
        stream.pos = pos;
        stream.bitBuffer = bitBuffer;
        stream.bitLength = bitLength;
        throw InflateNoDataError;
      }
      bitBuffer |= bytes[pos++] << bitLength;
      bitLength += 8;
    } while (maxBits > bitLength);
    stream.pos = pos;
  }
  var code = codeTable.codes[bitBuffer & (1 << maxBits) - 1];
  var len = code >> 16;
  stream.bitBuffer = bitBuffer >>> len;
  stream.bitLength = bitLength - len;
  return code & 65535;
}
(function (global) {
  global['createInflatedStream'] = createInflatedStream;
}(this));
var StreamNoDataError = {};
var Stream = function StreamClosure() {
    function Stream_align() {
      this.bitBuffer = this.bitLength = 0;
    }
    function Stream_ensure(size) {
      if (this.pos + size > this.end) {
        throw StreamNoDataError;
      }
    }
    function Stream_remaining() {
      return this.end - this.pos;
    }
    function Stream_substream(begin, end) {
      var stream = new Stream(this.bytes);
      stream.pos = begin;
      stream.end = end;
      return stream;
    }
    function Stream_push(data) {
      var bytes = this.bytes;
      var newBytesLength = this.end + data.length;
      if (newBytesLength > bytes.length) {
        throw 'stream buffer overfow';
      }
      bytes.set(data, this.end);
      this.end = newBytesLength;
    }
    function Stream(buffer, offset, length, maxLength) {
      if (offset === undefined)
        offset = 0;
      if (buffer.buffer instanceof ArrayBuffer) {
        offset += buffer.byteOffset;
        buffer = buffer.buffer;
      }
      if (length === undefined)
        length = buffer.byteLength - offset;
      if (maxLength === undefined)
        maxLength = length;
      var bytes = new Uint8Array(buffer, offset, maxLength);
      var stream = new DataView(buffer, offset, maxLength);
      stream.bytes = bytes;
      stream.pos = 0;
      stream.end = length;
      stream.bitBuffer = 0;
      stream.bitLength = 0;
      stream.align = Stream_align;
      stream.ensure = Stream_ensure;
      stream.remaining = Stream_remaining;
      stream.substream = Stream_substream;
      stream.push = Stream_push;
      return stream;
    }
    return Stream;
  }();
(function (global) {
  global['Stream'] = Stream;
}(this));
function readSi8($bytes, $stream) {
  return $stream.getInt8($stream.pos++);
}
function readSi16($bytes, $stream) {
  return $stream.getInt16($stream.pos, $stream.pos += 2);
}
function readSi32($bytes, $stream) {
  return $stream.getInt32($stream.pos, $stream.pos += 4);
}
function readUi8($bytes, $stream) {
  return $bytes[$stream.pos++];
}
function readUi16($bytes, $stream) {
  return $stream.getUint16($stream.pos, $stream.pos += 2);
}
function readUi32($bytes, $stream) {
  return $stream.getUint32($stream.pos, $stream.pos += 4);
}
function readFixed($bytes, $stream) {
  return $stream.getInt32($stream.pos, $stream.pos += 4) / 65536;
}
function readFixed8($bytes, $stream) {
  return $stream.getInt16($stream.pos, $stream.pos += 2) / 256;
}
function readFloat16($bytes, $stream) {
  var ui16 = $stream.getUint16($stream.pos);
  $stream.pos += 2;
  var sign = ui16 >> 15 ? -1 : 1;
  var exponent = (ui16 & 31744) >> 10;
  var fraction = ui16 & 1023;
  if (!exponent)
    return sign * pow(2, -14) * (fraction / 1024);
  if (exponent === 31)
    return fraction ? NaN : sign * Infinity;
  return sign * pow(2, exponent - 15) * (1 + fraction / 1024);
}
function readFloat($bytes, $stream) {
  return $stream.getFloat32($stream.pos, $stream.pos += 4);
}
function readDouble($bytes, $stream) {
  return $stream.getFloat64($stream.pos, $stream.pos += 8);
}
function readEncodedU32($bytes, $stream) {
  var val = $bytes[$stream.pos++];
  if (!(val & 128))
    return val;
  val |= $bytes[$stream.pos++] << 7;
  if (!(val & 16384))
    return val;
  val |= $bytes[$stream.pos++] << 14;
  if (!(val & 2097152))
    return val;
  val |= $bytes[$stream.pos++] << 21;
  if (!(val & 268435456))
    return val;
  return val | $bytes[$stream.pos++] << 28;
}
function readBool($bytes, $stream) {
  return !(!$bytes[$stream.pos++]);
}
function align($bytes, $stream) {
  $stream.align();
}
function readSb($bytes, $stream, size) {
  return readUb($bytes, $stream, size) << 32 - size >> 32 - size;
}
var masks = new Uint32Array(33);
for (var i = 1, mask = 0; i <= 32; ++i)
  masks[i] = mask = mask << 1 | 1;
function readUb($bytes, $stream, size) {
  var buffer = $stream.bitBuffer;
  var bitlen = $stream.bitLength;
  while (size > bitlen) {
    buffer = buffer << 8 | $bytes[$stream.pos++];
    bitlen += 8;
  }
  bitlen -= size;
  var val = buffer >>> bitlen & masks[size];
  $stream.bitBuffer = buffer;
  $stream.bitLength = bitlen;
  return val;
}
function readFb($bytes, $stream, size) {
  return readSb($bytes, $stream, size) / 65536;
}
function readString($bytes, $stream, length) {
  var codes = [];
  var pos = $stream.pos;
  if (length) {
    codes = slice.call($bytes, pos, pos += length);
  } else {
    length = 0;
    for (var code; code = $bytes[pos++]; length++)
      codes[length] = code;
  }
  $stream.pos = pos;
  var numChunks = length / 65536;
  var str = '';
  for (var i = 0; i < numChunks; ++i) {
    var begin = i * 65536;
    var end = begin + 65536;
    var chunk = codes.slice(begin, end);
    str += fromCharCode.apply(null, chunk);
  }
  return decodeURIComponent(escape(str.replace('\0', '', 'g')));
}
function readBinary($bytes, $stream, size) {
  return $bytes.subarray($stream.pos, $stream.pos = size ? $stream.pos + size : $stream.end);
}
(function (global) {
  global['readSi8'] = readSi8;
  global['readUi16'] = readUi16;
  global['readUi32'] = readUi32;
}(this));
var tagHandler = function (global) {
    function defineShape($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var $0 = $.bbox = {};
      bbox($bytes, $stream, $0, swfVersion, tagCode);
      var isMorph = $.isMorph = tagCode === 46 || tagCode === 84;
      if (isMorph) {
        var $1 = $.bboxMorph = {};
        bbox($bytes, $stream, $1, swfVersion, tagCode);
      }
      var hasStrokes = $.hasStrokes = tagCode === 83 || tagCode === 84;
      if (hasStrokes) {
        var $2 = $.strokeBbox = {};
        bbox($bytes, $stream, $2, swfVersion, tagCode);
        if (isMorph) {
          var $3 = $.strokeBboxMorph = {};
          bbox($bytes, $stream, $3, swfVersion, tagCode);
        }
        var reserved = readUb($bytes, $stream, 5);
        $.fillWinding = readUb($bytes, $stream, 1);
        $.nonScalingStrokes = readUb($bytes, $stream, 1);
        $.scalingStrokes = readUb($bytes, $stream, 1);
      }
      if (isMorph) {
        $.offsetMorph = readUi32($bytes, $stream);
        morphShapeWithStyle($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes);
      } else {
        shapeWithStyle($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes);
      }
      return $;
    }
    function placeObject($bytes, $stream, $, swfVersion, tagCode) {
      var flags, hasEvents, clip, hasName, hasRatio, hasCxform, hasMatrix, place;
      var move, hasBackgroundColor, hasVisibility, hasImage, hasClassName, cache;
      var blend, hasFilters, eoe;
      $ || ($ = {});
      if (tagCode > 4) {
        if (tagCode > 26) {
          flags = readUi16($bytes, $stream);
        } else {
          flags = readUi8($bytes, $stream);
        }
        hasEvents = $.hasEvents = flags >> 7 & 1;
        clip = $.clip = flags >> 6 & 1;
        hasName = $.hasName = flags >> 5 & 1;
        hasRatio = $.hasRatio = flags >> 4 & 1;
        hasCxform = $.hasCxform = flags >> 3 & 1;
        hasMatrix = $.hasMatrix = flags >> 2 & 1;
        place = $.place = flags >> 1 & 1;
        move = $.move = flags & 1;
        if (tagCode === 70) {
          hasBackgroundColor = $.hasBackgroundColor = flags >> 15 & 1;
          hasVisibility = $.hasVisibility = flags >> 14 & 1;
          hasImage = $.hasImage = flags >> 12 & 1;
          hasClassName = $.hasClassName = flags >> 11 & 1;
          cache = $.cache = flags >> 10 & 1;
          blend = $.blend = flags >> 9 & 1;
          hasFilters = $.hasFilters = flags >> 8 & 1;
        } else {
          cache = $.cache = 0;
          blend = $.blend = 0;
          hasFilters = $.hasFilters = 0;
        }
        $.depth = readUi16($bytes, $stream);
        if (hasClassName) {
          $.className = readString($bytes, $stream, 0);
        }
        if (place) {
          $.symbolId = readUi16($bytes, $stream);
        }
        if (hasMatrix) {
          var $0 = $.matrix = {};
          matrix($bytes, $stream, $0, swfVersion, tagCode);
        }
        if (hasCxform) {
          var $1 = $.cxform = {};
          cxform($bytes, $stream, $1, swfVersion, tagCode);
        }
        if (hasRatio) {
          $.ratio = readUi16($bytes, $stream);
        }
        if (hasName) {
          $.name = readString($bytes, $stream, 0);
        }
        if (clip) {
          $.clipDepth = readUi16($bytes, $stream);
        }
        if (hasFilters) {
          var count = readUi8($bytes, $stream);
          var $2 = $.filters = [];
          var $3 = count;
          while ($3--) {
            var $4 = {};
            anyFilter($bytes, $stream, $4, swfVersion, tagCode);
            $2.push($4);
          }
        }
        if (blend) {
          $.blendMode = readUi8($bytes, $stream);
        }
        if (cache) {
          $.bmpCache = readUi8($bytes, $stream);
        }
        if (hasEvents) {
          var reserved = readUi16($bytes, $stream);
          if (swfVersion >= 6) {
            var allFlags = readUi32($bytes, $stream);
          } else {
            var allFlags = readUi16($bytes, $stream);
          }
          var $28 = $.events = [];
          do {
            var $29 = {};
            var temp = events($bytes, $stream, $29, swfVersion, tagCode);
            eoe = temp.eoe;
            $28.push($29);
          } while (!eoe);
        }
        if (hasBackgroundColor) {
          var $126 = $.backgroundColor = {};
          argb($bytes, $stream, $126, swfVersion, tagCode);
        }
        if (hasVisibility) {
          $.visibility = readUi8($bytes, $stream);
        }
      } else {
        $.place = 1;
        $.symbolId = readUi16($bytes, $stream);
        $.depth = readUi16($bytes, $stream);
        $.hasMatrix = 1;
        var $30 = $.matrix = {};
        matrix($bytes, $stream, $30, swfVersion, tagCode);
        if ($stream.remaining()) {
          $.hasCxform = 1;
          var $31 = $.cxform = {};
          cxform($bytes, $stream, $31, swfVersion, tagCode);
        }
      }
      return $;
    }
    function removeObject($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      if (tagCode === 5) {
        $.symbolId = readUi16($bytes, $stream);
      }
      $.depth = readUi16($bytes, $stream);
      return $;
    }
    function defineImage($bytes, $stream, $, swfVersion, tagCode) {
      var imgData;
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      if (tagCode > 21) {
        var alphaDataOffset = readUi32($bytes, $stream);
        if (tagCode === 90) {
          $.deblock = readFixed8($bytes, $stream);
        }
        imgData = $.imgData = readBinary($bytes, $stream, alphaDataOffset);
        $.alphaData = readBinary($bytes, $stream, 0);
      } else {
        imgData = $.imgData = readBinary($bytes, $stream, 0);
      }
      switch (imgData[0] << 8 | imgData[1]) {
      case 65496:
      case 65497:
        $.mimeType = 'image/jpeg';
        break;
      case 35152:
        $.mimeType = 'image/png';
        break;
      case 18249:
        $.mimeType = 'image/gif';
        break;
      default:
        $.mimeType = 'application/octet-stream';
      }
      if (tagCode === 6) {
        $.incomplete = 1;
      }
      return $;
    }
    function defineButton($bytes, $stream, $, swfVersion, tagCode) {
      var eob, hasFilters, count, blend;
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      if (tagCode == 7) {
        var $0 = $.characters = [];
        do {
          var $1 = {};
          var temp = button($bytes, $stream, $1, swfVersion, tagCode);
          eob = temp.eob;
          $0.push($1);
        } while (!eob);
        $.actionsData = readBinary($bytes, $stream, 0);
      } else {
        var trackFlags = readUi8($bytes, $stream);
        $.trackAsMenu = trackFlags >> 7 & 1;
        var actionOffset = readUi16($bytes, $stream);
        var $28 = $.characters = [];
        do {
          var $29 = {};
          var flags = readUi8($bytes, $stream);
          var eob = $29.eob = !flags;
          if (swfVersion >= 8) {
            blend = $29.blend = flags >> 5 & 1;
            hasFilters = $29.hasFilters = flags >> 4 & 1;
          } else {
            blend = $29.blend = 0;
            hasFilters = $29.hasFilters = 0;
          }
          $29.stateHitTest = flags >> 3 & 1;
          $29.stateDown = flags >> 2 & 1;
          $29.stateOver = flags >> 1 & 1;
          $29.stateUp = flags & 1;
          if (!eob) {
            $29.symbolId = readUi16($bytes, $stream);
            $29.depth = readUi16($bytes, $stream);
            var $30 = $29.matrix = {};
            matrix($bytes, $stream, $30, swfVersion, tagCode);
            if (tagCode === 34) {
              var $31 = $29.cxform = {};
              cxform($bytes, $stream, $31, swfVersion, tagCode);
            }
            if (hasFilters) {
              $29.filterCount = readUi8($bytes, $stream);
              var $32 = $29.filters = {};
              var type = $32.type = readUi8($bytes, $stream);
              switch (type) {
              case 0:
                if (type === 4 || type === 7) {
                  count = readUi8($bytes, $stream);
                } else {
                  count = 1;
                }
                var $33 = $32.colors = [];
                var $34 = count;
                while ($34--) {
                  var $35 = {};
                  rgba($bytes, $stream, $35, swfVersion, tagCode);
                  $33.push($35);
                }
                if (type === 3) {
                  var $36 = $32.higlightColor = {};
                  rgba($bytes, $stream, $36, swfVersion, tagCode);
                }
                if (type === 4 || type === 7) {
                  var $37 = $32.ratios = [];
                  var $38 = count;
                  while ($38--) {
                    $37.push(readUi8($bytes, $stream));
                  }
                }
                $32.blurX = readFixed($bytes, $stream);
                $32.blurY = readFixed($bytes, $stream);
                if (type !== 2) {
                  $32.angle = readFixed($bytes, $stream);
                  $32.distance = readFixed($bytes, $stream);
                }
                $32.strength = readFixed8($bytes, $stream);
                $32.innerShadow = readUb($bytes, $stream, 1);
                $32.knockout = readUb($bytes, $stream, 1);
                $32.compositeSource = readUb($bytes, $stream, 1);
                if (type === 3) {
                  $32.onTop = readUb($bytes, $stream, 1);
                } else {
                  var reserved = readUb($bytes, $stream, 1);
                }
                if (type === 4 || type === 7) {
                  $32.passes = readUb($bytes, $stream, 4);
                } else {
                  var reserved = readUb($bytes, $stream, 4);
                }
                break;
              case 1:
                $32.blurX = readFixed($bytes, $stream);
                $32.blurY = readFixed($bytes, $stream);
                $32.passes = readUb($bytes, $stream, 5);
                var reserved = readUb($bytes, $stream, 3);
                break;
              case 2:
              case 3:
              case 4:
                if (type === 4 || type === 7) {
                  count = readUi8($bytes, $stream);
                } else {
                  count = 1;
                }
                var $39 = $32.colors = [];
                var $40 = count;
                while ($40--) {
                  var $41 = {};
                  rgba($bytes, $stream, $41, swfVersion, tagCode);
                  $39.push($41);
                }
                if (type === 3) {
                  var $42 = $32.higlightColor = {};
                  rgba($bytes, $stream, $42, swfVersion, tagCode);
                }
                if (type === 4 || type === 7) {
                  var $43 = $32.ratios = [];
                  var $44 = count;
                  while ($44--) {
                    $43.push(readUi8($bytes, $stream));
                  }
                }
                $32.blurX = readFixed($bytes, $stream);
                $32.blurY = readFixed($bytes, $stream);
                if (type !== 2) {
                  $32.angle = readFixed($bytes, $stream);
                  $32.distance = readFixed($bytes, $stream);
                }
                $32.strength = readFixed8($bytes, $stream);
                $32.innerShadow = readUb($bytes, $stream, 1);
                $32.knockout = readUb($bytes, $stream, 1);
                $32.compositeSource = readUb($bytes, $stream, 1);
                if (type === 3) {
                  $32.onTop = readUb($bytes, $stream, 1);
                } else {
                  var reserved = readUb($bytes, $stream, 1);
                }
                if (type === 4 || type === 7) {
                  $32.passes = readUb($bytes, $stream, 4);
                } else {
                  var reserved = readUb($bytes, $stream, 4);
                }
                break;
              case 5:
                var columns = $32.columns = readUi8($bytes, $stream);
                var rows = $32.rows = readUi8($bytes, $stream);
                $32.divisor = readFloat($bytes, $stream);
                $32.bias = readFloat($bytes, $stream);
                var $45 = $32.weights = [];
                var $46 = columns * rows;
                while ($46--) {
                  $45.push(readFloat($bytes, $stream));
                }
                var $47 = $32.defaultColor = {};
                rgba($bytes, $stream, $47, swfVersion, tagCode);
                var reserved = readUb($bytes, $stream, 6);
                $32.clamp = readUb($bytes, $stream, 1);
                $32.preserveAlpha = readUb($bytes, $stream, 1);
                break;
              case 6:
                var $48 = $32.matrix = [];
                var $49 = 20;
                while ($49--) {
                  $48.push(readFloat($bytes, $stream));
                }
                break;
              case 7:
                if (type === 4 || type === 7) {
                  count = readUi8($bytes, $stream);
                } else {
                  count = 1;
                }
                var $50 = $32.colors = [];
                var $51 = count;
                while ($51--) {
                  var $52 = {};
                  rgba($bytes, $stream, $52, swfVersion, tagCode);
                  $50.push($52);
                }
                if (type === 3) {
                  var $53 = $32.higlightColor = {};
                  rgba($bytes, $stream, $53, swfVersion, tagCode);
                }
                if (type === 4 || type === 7) {
                  var $54 = $32.ratios = [];
                  var $55 = count;
                  while ($55--) {
                    $54.push(readUi8($bytes, $stream));
                  }
                }
                $32.blurX = readFixed($bytes, $stream);
                $32.blurY = readFixed($bytes, $stream);
                if (type !== 2) {
                  $32.angle = readFixed($bytes, $stream);
                  $32.distance = readFixed($bytes, $stream);
                }
                $32.strength = readFixed8($bytes, $stream);
                $32.innerShadow = readUb($bytes, $stream, 1);
                $32.knockout = readUb($bytes, $stream, 1);
                $32.compositeSource = readUb($bytes, $stream, 1);
                if (type === 3) {
                  $32.onTop = readUb($bytes, $stream, 1);
                } else {
                  var reserved = readUb($bytes, $stream, 1);
                }
                if (type === 4 || type === 7) {
                  $32.passes = readUb($bytes, $stream, 4);
                } else {
                  var reserved = readUb($bytes, $stream, 4);
                }
                break;
              default:
              }
            }
            if (blend) {
              $29.blendMode = readUi8($bytes, $stream);
            }
          }
          $28.push($29);
        } while (!eob);
        if (!(!actionOffset)) {
          var $56 = $.buttonActions = [];
          do {
            var $57 = {};
            buttonCondAction($bytes, $stream, $57, swfVersion, tagCode);
            $56.push($57);
          } while ($stream.remaining() > 0);
        }
      }
      return $;
    }
    function defineJPEGTables($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = 0;
      $.imgData = readBinary($bytes, $stream, 0);
      $.mimeType = 'application/octet-stream';
      return $;
    }
    function setBackgroundColor($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      var $0 = $.color = {};
      rgb($bytes, $stream, $0, swfVersion, tagCode);
      return $;
    }
    function defineBinaryData($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var reserved = readUi32($bytes, $stream);
      $.data = readBinary($bytes, $stream, 0);
      return $;
    }
    function defineFont($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var firstOffset = readUi16($bytes, $stream);
      var glyphCount = $.glyphCount = firstOffset / 2;
      var restOffsets = [];
      var $0 = glyphCount - 1;
      while ($0--) {
        restOffsets.push(readUi16($bytes, $stream));
      }
      $.offsets = [
        firstOffset
      ].concat(restOffsets);
      var $1 = $.glyphs = [];
      var $2 = glyphCount;
      while ($2--) {
        var $3 = {};
        shape($bytes, $stream, $3, swfVersion, tagCode);
        $1.push($3);
      }
      return $;
    }
    function defineLabel($bytes, $stream, $, swfVersion, tagCode) {
      var eot;
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var $0 = $.bbox = {};
      bbox($bytes, $stream, $0, swfVersion, tagCode);
      var $1 = $.matrix = {};
      matrix($bytes, $stream, $1, swfVersion, tagCode);
      var glyphBits = $.glyphBits = readUi8($bytes, $stream);
      var advanceBits = $.advanceBits = readUi8($bytes, $stream);
      var $2 = $.records = [];
      do {
        var $3 = {};
        var temp = textRecord($bytes, $stream, $3, swfVersion, tagCode, glyphBits, advanceBits);
        eot = temp.eot;
        $2.push($3);
      } while (!eot);
      return $;
    }
    function doAction($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      if (tagCode === 59) {
        $.spriteId = readUi16($bytes, $stream);
      }
      $.actionsData = readBinary($bytes, $stream, 0);
      return $;
    }
    function defineSound($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var soundFlags = readUi8($bytes, $stream);
      $.soundFormat = soundFlags >> 4 & 15;
      $.soundRate = soundFlags >> 2 & 3;
      $.soundSize = soundFlags >> 1 & 1;
      $.soundType = soundFlags & 1;
      $.samplesCount = readUi32($bytes, $stream);
      $.soundData = readBinary($bytes, $stream, 0);
      return $;
    }
    function startSound($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      if (tagCode == 15) {
        $.soundId = readUi16($bytes, $stream);
      }
      if (tagCode == 89) {
        $.soundClassName = readString($bytes, $stream, 0);
      }
      var $0 = $.soundInfo = {};
      soundInfo($bytes, $stream, $0, swfVersion, tagCode);
      return $;
    }
    function soundStreamHead($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      var playbackFlags = readUi8($bytes, $stream);
      $.playbackRate = playbackFlags >> 2 & 3;
      $.playbackSize = playbackFlags >> 1 & 1;
      $.playbackType = playbackFlags & 1;
      var streamFlags = readUi8($bytes, $stream);
      var streamCompression = $.streamCompression = streamFlags >> 4 & 15;
      $.streamRate = streamFlags >> 2 & 3;
      $.streamSize = streamFlags >> 1 & 1;
      $.streamType = streamFlags & 1;
      $.samplesCount = readUi32($bytes, $stream);
      if (streamCompression == 2) {
        $.latencySeek = readSi16($bytes, $stream);
      }
      return $;
    }
    function soundStreamBlock($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.data = readBinary($bytes, $stream, 0);
      return $;
    }
    function defineBitmap($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var format = $.format = readUi8($bytes, $stream);
      $.width = readUi16($bytes, $stream);
      $.height = readUi16($bytes, $stream);
      $.hasAlpha = tagCode === 36;
      if (format === 3) {
        $.colorTableSize = readUi8($bytes, $stream);
      }
      $.bmpData = readBinary($bytes, $stream, 0);
      return $;
    }
    function defineText($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var $0 = $.bbox = {};
      bbox($bytes, $stream, $0, swfVersion, tagCode);
      var flags = readUi16($bytes, $stream);
      var hasText = $.hasText = flags >> 7 & 1;
      $.wordWrap = flags >> 6 & 1;
      $.multiline = flags >> 5 & 1;
      $.password = flags >> 4 & 1;
      $.readonly = flags >> 3 & 1;
      var hasColor = $.hasColor = flags >> 2 & 1;
      var hasMaxLength = $.hasMaxLength = flags >> 1 & 1;
      var hasFont = $.hasFont = flags & 1;
      var hasFontClass = $.hasFontClass = flags >> 15 & 1;
      $.autoSize = flags >> 14 & 1;
      var hasLayout = $.hasLayout = flags >> 13 & 1;
      $.noSelect = flags >> 12 & 1;
      $.border = flags >> 11 & 1;
      $.wasStatic = flags >> 10 & 1;
      $.html = flags >> 9 & 1;
      $.useOutlines = flags >> 8 & 1;
      if (hasFont) {
        $.fontId = readUi16($bytes, $stream);
      }
      if (hasFontClass) {
        $.fontClass = readString($bytes, $stream, 0);
      }
      if (hasFont) {
        $.fontHeight = readUi16($bytes, $stream);
      }
      if (hasColor) {
        var $1 = $.color = {};
        rgba($bytes, $stream, $1, swfVersion, tagCode);
      }
      if (hasMaxLength) {
        $.maxLength = readUi16($bytes, $stream);
      }
      if (hasLayout) {
        $.align = readUi8($bytes, $stream);
        $.leftMargin = readUi16($bytes, $stream);
        $.rightMargin = readUi16($bytes, $stream);
        $.indent = readSi16($bytes, $stream);
        $.leading = readSi16($bytes, $stream);
      }
      $.variableName = readString($bytes, $stream, 0);
      if (hasText) {
        $.initialText = readString($bytes, $stream, 0);
      }
      return $;
    }
    function frameLabel($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.name = readString($bytes, $stream, 0);
      return $;
    }
    function defineFont2($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.id = readUi16($bytes, $stream);
      var hasLayout = $.hasLayout = readUb($bytes, $stream, 1);
      if (swfVersion > 5) {
        $.shiftJis = readUb($bytes, $stream, 1);
      } else {
        var reserved = readUb($bytes, $stream, 1);
      }
      $.smallText = readUb($bytes, $stream, 1);
      $.ansi = readUb($bytes, $stream, 1);
      var wideOffset = $.wideOffset = readUb($bytes, $stream, 1);
      var wide = $.wide = readUb($bytes, $stream, 1);
      $.italic = readUb($bytes, $stream, 1);
      $.bold = readUb($bytes, $stream, 1);
      if (swfVersion > 5) {
        $.language = readUi8($bytes, $stream);
      } else {
        var reserved = readUi8($bytes, $stream);
        $.language = 0;
      }
      var nameLength = readUi8($bytes, $stream);
      $.name = readString($bytes, $stream, nameLength);
      if (tagCode === 75) {
        $.resolution = 20;
      }
      var glyphCount = $.glyphCount = readUi16($bytes, $stream);
      if (wideOffset) {
        var $0 = $.offsets = [];
        var $1 = glyphCount;
        while ($1--) {
          $0.push(readUi32($bytes, $stream));
        }
        $.mapOffset = readUi32($bytes, $stream);
      } else {
        var $2 = $.offsets = [];
        var $3 = glyphCount;
        while ($3--) {
          $2.push(readUi16($bytes, $stream));
        }
        $.mapOffset = readUi16($bytes, $stream);
      }
      var $4 = $.glyphs = [];
      var $5 = glyphCount;
      while ($5--) {
        var $6 = {};
        shape($bytes, $stream, $6, swfVersion, tagCode);
        $4.push($6);
      }
      if (wide) {
        var $47 = $.codes = [];
        var $48 = glyphCount;
        while ($48--) {
          $47.push(readUi16($bytes, $stream));
        }
      } else {
        var $49 = $.codes = [];
        var $50 = glyphCount;
        while ($50--) {
          $49.push(readUi8($bytes, $stream));
        }
      }
      if (hasLayout) {
        $.ascent = readUi16($bytes, $stream);
        $.descent = readUi16($bytes, $stream);
        $.leading = readSi16($bytes, $stream);
        var $51 = $.advance = [];
        var $52 = glyphCount;
        while ($52--) {
          $51.push(readSi16($bytes, $stream));
        }
        var $53 = $.bbox = [];
        var $54 = glyphCount;
        while ($54--) {
          var $55 = {};
          bbox($bytes, $stream, $55, swfVersion, tagCode);
          $53.push($55);
        }
        var kerningCount = readUi16($bytes, $stream);
        var $56 = $.kerning = [];
        var $57 = kerningCount;
        while ($57--) {
          var $58 = {};
          kerning($bytes, $stream, $58, swfVersion, tagCode, wide);
          $56.push($58);
        }
      }
      return $;
    }
    function fileAttributes($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      var reserved = readUb($bytes, $stream, 1);
      $.useDirectBlit = readUb($bytes, $stream, 1);
      $.useGpu = readUb($bytes, $stream, 1);
      $.hasMetadata = readUb($bytes, $stream, 1);
      $.doAbc = readUb($bytes, $stream, 1);
      $.noCrossDomainCaching = readUb($bytes, $stream, 1);
      $.relativeUrls = readUb($bytes, $stream, 1);
      $.network = readUb($bytes, $stream, 1);
      var pad = readUb($bytes, $stream, 24);
      return $;
    }
    function doABC($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      if (tagCode === 82) {
        $.flags = readUi32($bytes, $stream);
      } else {
        $.flags = 0;
      }
      if (tagCode === 82) {
        $.name = readString($bytes, $stream, 0);
      } else {
        $.name = '';
      }
      $.data = readBinary($bytes, $stream, 0);
      return $;
    }
    function symbolClass($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      var symbolCount = readUi16($bytes, $stream);
      var $0 = $.exports = [];
      var $1 = symbolCount;
      while ($1--) {
        var $2 = {};
        $2.symbolId = readUi16($bytes, $stream);
        $2.className = readString($bytes, $stream, 0);
        $0.push($2);
      }
      return $;
    }
    function defineScalingGrid($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      $.symbolId = readUi16($bytes, $stream);
      var $0 = $.splitter = {};
      bbox($bytes, $stream, $0, swfVersion, tagCode);
      return $;
    }
    function defineScene($bytes, $stream, $, swfVersion, tagCode) {
      $ || ($ = {});
      var sceneCount = readEncodedU32($bytes, $stream);
      var $0 = $.scenes = [];
      var $1 = sceneCount;
      while ($1--) {
        var $2 = {};
        $2.offset = readEncodedU32($bytes, $stream);
        $2.name = readString($bytes, $stream, 0);
        $0.push($2);
      }
      var labelCount = readEncodedU32($bytes, $stream);
      var $3 = $.labels = [];
      var $4 = labelCount;
      while ($4--) {
        var $5 = {};
        $5.frame = readEncodedU32($bytes, $stream);
        $5.name = readString($bytes, $stream, 0);
        $3.push($5);
      }
      return $;
    }
    function bbox($bytes, $stream, $, swfVersion, tagCode) {
      align($bytes, $stream);
      var bits = readUb($bytes, $stream, 5);
      var xMin = readSb($bytes, $stream, bits);
      var xMax = readSb($bytes, $stream, bits);
      var yMin = readSb($bytes, $stream, bits);
      var yMax = readSb($bytes, $stream, bits);
      $.xMin = xMin;
      $.xMax = xMax;
      $.yMin = yMin;
      $.yMax = yMax;
      align($bytes, $stream);
    }
    function rgb($bytes, $stream, $, swfVersion, tagCode) {
      $.red = readUi8($bytes, $stream);
      $.green = readUi8($bytes, $stream);
      $.blue = readUi8($bytes, $stream);
      $.alpha = 255;
      return;
    }
    function rgba($bytes, $stream, $, swfVersion, tagCode) {
      $.red = readUi8($bytes, $stream);
      $.green = readUi8($bytes, $stream);
      $.blue = readUi8($bytes, $stream);
      $.alpha = readUi8($bytes, $stream);
      return;
    }
    function argb($bytes, $stream, $, swfVersion, tagCode) {
      $.alpha = readUi8($bytes, $stream);
      $.red = readUi8($bytes, $stream);
      $.green = readUi8($bytes, $stream);
      $.blue = readUi8($bytes, $stream);
    }
    function fillSolid($bytes, $stream, $, swfVersion, tagCode, isMorph) {
      if (tagCode > 22 || isMorph) {
        var $125 = $.color = {};
        rgba($bytes, $stream, $125, swfVersion, tagCode);
      } else {
        var $126 = $.color = {};
        rgb($bytes, $stream, $126, swfVersion, tagCode);
      }
      if (isMorph) {
        var $127 = $.colorMorph = {};
        rgba($bytes, $stream, $127, swfVersion, tagCode);
      }
      return;
    }
    function matrix($bytes, $stream, $, swfVersion, tagCode) {
      align($bytes, $stream);
      var hasScale = readUb($bytes, $stream, 1);
      if (hasScale) {
        var bits = readUb($bytes, $stream, 5);
        $.a = readFb($bytes, $stream, bits);
        $.d = readFb($bytes, $stream, bits);
      } else {
        $.a = 1;
        $.d = 1;
      }
      var hasRotate = readUb($bytes, $stream, 1);
      if (hasRotate) {
        var bits = readUb($bytes, $stream, 5);
        $.b = readFb($bytes, $stream, bits);
        $.c = readFb($bytes, $stream, bits);
      } else {
        $.b = 0;
        $.c = 0;
      }
      var bits = readUb($bytes, $stream, 5);
      var e = readSb($bytes, $stream, bits);
      var f = readSb($bytes, $stream, bits);
      $.tx = e;
      $.ty = f;
      align($bytes, $stream);
    }
    function cxform($bytes, $stream, $, swfVersion, tagCode) {
      align($bytes, $stream);
      var hasOffsets = readUb($bytes, $stream, 1);
      var hasMultipliers = readUb($bytes, $stream, 1);
      var bits = readUb($bytes, $stream, 4);
      if (hasMultipliers) {
        $.redMultiplier = readSb($bytes, $stream, bits);
        $.greenMultiplier = readSb($bytes, $stream, bits);
        $.blueMultiplier = readSb($bytes, $stream, bits);
        if (tagCode > 4) {
          $.alphaMultiplier = readSb($bytes, $stream, bits);
        } else {
          $.alphaMultiplier = 256;
        }
      } else {
        $.redMultiplier = 256;
        $.greenMultiplier = 256;
        $.blueMultiplier = 256;
        $.alphaMultiplier = 256;
      }
      if (hasOffsets) {
        $.redOffset = readSb($bytes, $stream, bits);
        $.greenOffset = readSb($bytes, $stream, bits);
        $.blueOffset = readSb($bytes, $stream, bits);
        if (tagCode > 4) {
          $.alphaOffset = readSb($bytes, $stream, bits);
        } else {
          $.alphaOffset = 0;
        }
      } else {
        $.redOffset = 0;
        $.greenOffset = 0;
        $.blueOffset = 0;
        $.alphaOffset = 0;
      }
      align($bytes, $stream);
    }
    function fillGradient($bytes, $stream, $, swfVersion, tagCode, isMorph, type) {
      var $128 = $.matrix = {};
      matrix($bytes, $stream, $128, swfVersion, tagCode);
      if (isMorph) {
        var $129 = $.matrixMorph = {};
        matrix($bytes, $stream, $129, swfVersion, tagCode);
      }
      gradient($bytes, $stream, $, swfVersion, tagCode, isMorph, type);
    }
    function gradient($bytes, $stream, $, swfVersion, tagCode, isMorph, type) {
      if (tagCode === 83) {
        $.spreadMode = readUb($bytes, $stream, 2);
        $.interpolationMode = readUb($bytes, $stream, 2);
      } else {
        var pad = readUb($bytes, $stream, 4);
      }
      var count = $.count = readUb($bytes, $stream, 4);
      var $130 = $.records = [];
      var $131 = count;
      while ($131--) {
        var $132 = {};
        gradientRecord($bytes, $stream, $132, swfVersion, tagCode, isMorph);
        $130.push($132);
      }
      if (type === 19) {
        $.focalPoint = readFixed8($bytes, $stream);
        if (isMorph) {
          $.focalPointMorph = readFixed8($bytes, $stream);
        }
      }
    }
    function gradientRecord($bytes, $stream, $, swfVersion, tagCode, isMorph) {
      $.ratio = readUi8($bytes, $stream);
      if (tagCode > 22) {
        var $133 = $.color = {};
        rgba($bytes, $stream, $133, swfVersion, tagCode);
      } else {
        var $134 = $.color = {};
        rgb($bytes, $stream, $134, swfVersion, tagCode);
      }
      if (isMorph) {
        $.ratioMorph = readUi8($bytes, $stream);
        var $135 = $.colorMorph = {};
        rgba($bytes, $stream, $135, swfVersion, tagCode);
      }
    }
    function morphShapeWithStyle($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes) {
      var eos, bits;
      var temp = styles($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes);
      var lineBits = temp.lineBits;
      var fillBits = temp.fillBits;
      var $160 = $.records = [];
      do {
        var $161 = {};
        var temp = shapeRecord($bytes, $stream, $161, swfVersion, tagCode, isMorph, fillBits, lineBits, hasStrokes, bits);
        var eos = temp.eos;
        var flags = temp.flags;
        var type = temp.type;
        var fillBits = temp.fillBits;
        var lineBits = temp.lineBits;
        var bits = temp.bits;
        $160.push($161);
      } while (!eos);
      var temp = styleBits($bytes, $stream, $, swfVersion, tagCode);
      var fillBits = temp.fillBits;
      var lineBits = temp.lineBits;
      var $162 = $.recordsMorph = [];
      do {
        var $163 = {};
        var temp = shapeRecord($bytes, $stream, $163, swfVersion, tagCode, isMorph, fillBits, lineBits, hasStrokes, bits);
        eos = temp.eos;
        var flags = temp.flags;
        var type = temp.type;
        var fillBits = temp.fillBits;
        var lineBits = temp.lineBits;
        bits = temp.bits;
        $162.push($163);
      } while (!eos);
    }
    function shapeWithStyle($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes) {
      var eos;
      var temp = styles($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes);
      var fillBits = temp.fillBits;
      var lineBits = temp.lineBits;
      var $160 = $.records = [];
      do {
        var $161 = {};
        var temp = shapeRecord($bytes, $stream, $161, swfVersion, tagCode, isMorph, fillBits, lineBits, hasStrokes, bits);
        eos = temp.eos;
        var flags = temp.flags;
        var type = temp.type;
        var fillBits = temp.fillBits;
        var lineBits = temp.lineBits;
        var bits = temp.bits;
        $160.push($161);
      } while (!eos);
    }
    function shapeRecord($bytes, $stream, $, swfVersion, tagCode, isMorph, fillBits, lineBits, hasStrokes, bits) {
      var type = $.type = readUb($bytes, $stream, 1);
      var flags = readUb($bytes, $stream, 5);
      var eos = $.eos = !(type || flags);
      if (type) {
        var temp = shapeRecordEdge($bytes, $stream, $, swfVersion, tagCode, flags, bits);
        var bits = temp.bits;
      } else {
        var temp = shapeRecordSetup($bytes, $stream, $, swfVersion, tagCode, flags, isMorph, fillBits, lineBits, hasStrokes, bits);
        var fillBits = temp.fillBits;
        var lineBits = temp.lineBits;
        var bits = temp.bits;
      }
      return {
        type: type,
        flags: flags,
        eos: eos,
        fillBits: fillBits,
        lineBits: lineBits,
        bits: bits
      };
    }
    function shapeRecordEdge($bytes, $stream, $, swfVersion, tagCode, flags, bits) {
      var isStraight = 0, tmp = 0, bits = 0, isGeneral = 0, isVertical = 0;
      isStraight = $.isStraight = flags >> 4;
      tmp = flags & 15;
      bits = tmp + 2;
      if (isStraight) {
        isGeneral = $.isGeneral = readUb($bytes, $stream, 1);
        if (isGeneral) {
          $.deltaX = readSb($bytes, $stream, bits);
          $.deltaY = readSb($bytes, $stream, bits);
        } else {
          isVertical = $.isVertical = readUb($bytes, $stream, 1);
          if (isVertical) {
            $.deltaY = readSb($bytes, $stream, bits);
          } else {
            $.deltaX = readSb($bytes, $stream, bits);
          }
        }
      } else {
        $.controlDeltaX = readSb($bytes, $stream, bits);
        $.controlDeltaY = readSb($bytes, $stream, bits);
        $.anchorDeltaX = readSb($bytes, $stream, bits);
        $.anchorDeltaY = readSb($bytes, $stream, bits);
      }
      return {
        bits: bits
      };
    }
    function shapeRecordSetup($bytes, $stream, $, swfVersion, tagCode, flags, isMorph, fillBits, lineBits, hasStrokes, bits) {
      var hasNewStyles = 0, hasLineStyle = 0, hasFillStyle1 = 0;
      var hasFillStyle0 = 0, move = 0;
      if (tagCode > 2) {
        hasNewStyles = $.hasNewStyles = flags >> 4;
      } else {
        hasNewStyles = $.hasNewStyles = 0;
      }
      hasLineStyle = $.hasLineStyle = flags >> 3 & 1;
      hasFillStyle1 = $.hasFillStyle1 = flags >> 2 & 1;
      hasFillStyle0 = $.hasFillStyle0 = flags >> 1 & 1;
      move = $.move = flags & 1;
      if (move) {
        bits = readUb($bytes, $stream, 5);
        $.moveX = readSb($bytes, $stream, bits);
        $.moveY = readSb($bytes, $stream, bits);
      }
      if (hasFillStyle0) {
        $.fillStyle0 = readUb($bytes, $stream, fillBits);
      }
      if (hasFillStyle1) {
        $.fillStyle1 = readUb($bytes, $stream, fillBits);
      }
      if (hasLineStyle) {
        $.lineStyle = readUb($bytes, $stream, lineBits);
      }
      if (hasNewStyles) {
        var temp = styles($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes);
        var lineBits = temp.lineBits;
        var fillBits = temp.fillBits;
      }
      return {
        lineBits: lineBits,
        fillBits: fillBits,
        bits: bits
      };
    }
    function styles($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes) {
      fillStyleArray($bytes, $stream, $, swfVersion, tagCode, isMorph);
      lineStyleArray($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes);
      var temp = styleBits($bytes, $stream, $, swfVersion, tagCode);
      var fillBits = temp.fillBits;
      var lineBits = temp.lineBits;
      return {
        fillBits: fillBits,
        lineBits: lineBits
      };
    }
    function fillStyleArray($bytes, $stream, $, swfVersion, tagCode, isMorph) {
      var count;
      var tmp = readUi8($bytes, $stream);
      if (tagCode > 2 && tmp === 255) {
        count = readUi16($bytes, $stream);
      } else {
        count = tmp;
      }
      var $4 = $.fillStyles = [];
      var $5 = count;
      while ($5--) {
        var $6 = {};
        fillStyle($bytes, $stream, $6, swfVersion, tagCode, isMorph);
        $4.push($6);
      }
    }
    function lineStyleArray($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes) {
      var count;
      var tmp = readUi8($bytes, $stream);
      if (tagCode > 2 && tmp === 255) {
        count = readUi16($bytes, $stream);
      } else {
        count = tmp;
      }
      var $138 = $.lineStyles = [];
      var $139 = count;
      while ($139--) {
        var $140 = {};
        lineStyle($bytes, $stream, $140, swfVersion, tagCode, isMorph, hasStrokes);
        $138.push($140);
      }
    }
    function styleBits($bytes, $stream, $, swfVersion, tagCode) {
      align($bytes, $stream);
      var fillBits = readUb($bytes, $stream, 4);
      var lineBits = readUb($bytes, $stream, 4);
      return {
        fillBits: fillBits,
        lineBits: lineBits
      };
    }
    function fillStyle($bytes, $stream, $, swfVersion, tagCode, isMorph) {
      var type = $.type = readUi8($bytes, $stream);
      switch (type) {
      case 0:
        fillSolid($bytes, $stream, $, swfVersion, tagCode, isMorph);
        break;
      case 16:
      case 18:
      case 19:
        fillGradient($bytes, $stream, $, swfVersion, tagCode, isMorph, type);
        break;
      case 64:
      case 65:
      case 66:
      case 67:
        fillBitmap($bytes, $stream, $, swfVersion, tagCode, isMorph, type);
        break;
      default:
      }
    }
    function lineStyle($bytes, $stream, $, swfVersion, tagCode, isMorph, hasStrokes) {
      $.width = readUi16($bytes, $stream);
      if (isMorph) {
        $.widthMorph = readUi16($bytes, $stream);
      }
      if (hasStrokes) {
        align($bytes, $stream);
        $.startCapStyle = readUb($bytes, $stream, 2);
        var joinStyle = $.joinStyle = readUb($bytes, $stream, 2);
        var hasFill = $.hasFill = readUb($bytes, $stream, 1);
        $.noHscale = readUb($bytes, $stream, 1);
        $.noVscale = readUb($bytes, $stream, 1);
        $.pixelHinting = readUb($bytes, $stream, 1);
        var reserved = readUb($bytes, $stream, 5);
        $.noClose = readUb($bytes, $stream, 1);
        $.endCapStyle = readUb($bytes, $stream, 2);
        if (joinStyle === 2) {
          $.miterLimitFactor = readFixed8($bytes, $stream);
        }
        if (hasFill) {
          var $141 = $.fillStyle = {};
          fillStyle($bytes, $stream, $141, swfVersion, tagCode, isMorph);
        } else {
          var $155 = $.color = {};
          rgba($bytes, $stream, $155, swfVersion, tagCode);
          if (isMorph) {
            var $156 = $.colorMorph = {};
            rgba($bytes, $stream, $156, swfVersion, tagCode);
          }
        }
      } else {
        if (tagCode > 22) {
          var $157 = $.color = {};
          rgba($bytes, $stream, $157, swfVersion, tagCode);
        } else {
          var $158 = $.color = {};
          rgb($bytes, $stream, $158, swfVersion, tagCode);
        }
        if (isMorph) {
          var $159 = $.colorMorph = {};
          rgba($bytes, $stream, $159, swfVersion, tagCode);
        }
      }
    }
    function fillBitmap($bytes, $stream, $, swfVersion, tagCode, isMorph, type) {
      $.bitmapId = readUi16($bytes, $stream);
      var $18 = $.matrix = {};
      matrix($bytes, $stream, $18, swfVersion, tagCode);
      if (isMorph) {
        var $19 = $.matrixMorph = {};
        matrix($bytes, $stream, $19, swfVersion, tagCode);
      }
      $.condition = type === 64 || type === 67;
    }
    function filterGlow($bytes, $stream, $, swfVersion, tagCode, type) {
      var count;
      if (type === 4 || type === 7) {
        count = readUi8($bytes, $stream);
      } else {
        count = 1;
      }
      var $5 = $.colors = [];
      var $6 = count;
      while ($6--) {
        var $7 = {};
        rgba($bytes, $stream, $7, swfVersion, tagCode);
        $5.push($7);
      }
      if (type === 3) {
        var $8 = $.higlightColor = {};
        rgba($bytes, $stream, $8, swfVersion, tagCode);
      }
      if (type === 4 || type === 7) {
        var $9 = $.ratios = [];
        var $10 = count;
        while ($10--) {
          $9.push(readUi8($bytes, $stream));
        }
      }
      $.blurX = readFixed($bytes, $stream);
      $.blurY = readFixed($bytes, $stream);
      if (type !== 2) {
        $.angle = readFixed($bytes, $stream);
        $.distance = readFixed($bytes, $stream);
      }
      $.strength = readFixed8($bytes, $stream);
      $.innerShadow = readUb($bytes, $stream, 1);
      $.knockout = readUb($bytes, $stream, 1);
      $.compositeSource = readUb($bytes, $stream, 1);
      if (type === 3) {
        $.onTop = readUb($bytes, $stream, 1);
      } else {
        var reserved = readUb($bytes, $stream, 1);
      }
      if (type === 4 || type === 7) {
        $.passes = readUb($bytes, $stream, 4);
      } else {
        var reserved = readUb($bytes, $stream, 4);
      }
    }
    function filterBlur($bytes, $stream, $, swfVersion, tagCode) {
      $.blurX = readFixed($bytes, $stream);
      $.blurY = readFixed($bytes, $stream);
      $.passes = readUb($bytes, $stream, 5);
      var reserved = readUb($bytes, $stream, 3);
    }
    function filterConvolution($bytes, $stream, $, swfVersion, tagCode) {
      var columns = $.columns = readUi8($bytes, $stream);
      var rows = $.rows = readUi8($bytes, $stream);
      $.divisor = readFloat($bytes, $stream);
      $.bias = readFloat($bytes, $stream);
      var $17 = $.weights = [];
      var $18 = columns * rows;
      while ($18--) {
        $17.push(readFloat($bytes, $stream));
      }
      var $19 = $.defaultColor = {};
      rgba($bytes, $stream, $19, swfVersion, tagCode);
      var reserved = readUb($bytes, $stream, 6);
      $.clamp = readUb($bytes, $stream, 1);
      $.preserveAlpha = readUb($bytes, $stream, 1);
    }
    function filterColorMatrix($bytes, $stream, $, swfVersion, tagCode) {
      var $20 = $.matrix = [];
      var $21 = 20;
      while ($21--) {
        $20.push(readFloat($bytes, $stream));
      }
    }
    function anyFilter($bytes, $stream, $, swfVersion, tagCode) {
      var type = $.type = readUi8($bytes, $stream);
      switch (type) {
      case 0:
      case 2:
      case 3:
      case 4:
      case 7:
        filterGlow($bytes, $stream, $, swfVersion, tagCode, type);
        break;
      case 1:
        filterBlur($bytes, $stream, $, swfVersion, tagCode);
        break;
      case 5:
        filterConvolution($bytes, $stream, $, swfVersion, tagCode);
        break;
      case 6:
        filterColorMatrix($bytes, $stream, $, swfVersion, tagCode);
        break;
      default:
      }
    }
    function events($bytes, $stream, $, swfVersion, tagCode) {
      var flags, keyPress;
      if (swfVersion >= 6) {
        flags = readUi32($bytes, $stream);
      } else {
        flags = readUi16($bytes, $stream);
      }
      var eoe = $.eoe = !flags;
      $.onKeyUp = flags >> 7 & 1;
      $.onKeyDown = flags >> 6 & 1;
      $.onMouseUp = flags >> 5 & 1;
      $.onMouseDown = flags >> 4 & 1;
      $.onMouseMove = flags >> 3 & 1;
      $.onUnload = flags >> 2 & 1;
      $.onEnterFrame = flags >> 1 & 1;
      $.onLoad = flags & 1;
      if (swfVersion >= 6) {
        $.onDragOver = flags >> 15 & 1;
        $.onRollOut = flags >> 14 & 1;
        $.onRollOver = flags >> 13 & 1;
        $.onReleaseOutside = flags >> 12 & 1;
        $.onRelease = flags >> 11 & 1;
        $.onPress = flags >> 10 & 1;
        $.onInitialize = flags >> 9 & 1;
        $.onData = flags >> 8 & 1;
        if (swfVersion >= 7) {
          $.onConstruct = flags >> 18 & 1;
        } else {
          $.onConstruct = 0;
        }
        keyPress = $.keyPress = flags >> 17 & 1;
        $.onDragOut = flags >> 16 & 1;
      }
      if (!eoe) {
        var length = $.length = readUi32($bytes, $stream);
        if (keyPress) {
          $.keyCode = readUi8($bytes, $stream);
        }
        $.actionsData = readBinary($bytes, $stream, length - (keyPress ? 1 : 0));
      }
      return {
        eoe: eoe
      };
    }
    function kerning($bytes, $stream, $, swfVersion, tagCode, wide) {
      if (wide) {
        $.code1 = readUi16($bytes, $stream);
        $.code2 = readUi16($bytes, $stream);
      } else {
        $.code1 = readUi8($bytes, $stream);
        $.code2 = readUi8($bytes, $stream);
      }
      $.adjustment = readUi16($bytes, $stream);
    }
    function textEntry($bytes, $stream, $, swfVersion, tagCode, glyphBits, advanceBits) {
      $.glyphIndex = readUb($bytes, $stream, glyphBits);
      $.advance = readSb($bytes, $stream, advanceBits);
    }
    function textRecordSetup($bytes, $stream, $, swfVersion, tagCode, flags) {
      var hasFont = $.hasFont = flags >> 3 & 1;
      var hasColor = $.hasColor = flags >> 2 & 1;
      var hasMoveY = $.hasMoveY = flags >> 1 & 1;
      var hasMoveX = $.hasMoveX = flags & 1;
      if (hasFont) {
        $.fontId = readUi16($bytes, $stream);
      }
      if (hasColor) {
        if (tagCode === 33) {
          var $4 = $.color = {};
          rgba($bytes, $stream, $4, swfVersion, tagCode);
        } else {
          var $5 = $.color = {};
          rgb($bytes, $stream, $5, swfVersion, tagCode);
        }
      }
      if (hasMoveX) {
        $.moveX = readSi16($bytes, $stream);
      }
      if (hasMoveY) {
        $.moveY = readSi16($bytes, $stream);
      }
      if (hasFont) {
        $.fontHeight = readUi16($bytes, $stream);
      }
    }
    function textRecord($bytes, $stream, $, swfVersion, tagCode, glyphBits, advanceBits) {
      var glyphCount;
      align($bytes, $stream);
      var flags = readUb($bytes, $stream, 8);
      var eot = $.eot = !flags;
      textRecordSetup($bytes, $stream, $, swfVersion, tagCode, flags);
      if (!eot) {
        var tmp = readUi8($bytes, $stream);
        if (swfVersion > 6) {
          glyphCount = $.glyphCount = tmp;
        } else {
          glyphCount = $.glyphCount = tmp & 127;
        }
        var $6 = $.entries = [];
        var $7 = glyphCount;
        while ($7--) {
          var $8 = {};
          textEntry($bytes, $stream, $8, swfVersion, tagCode, glyphBits, advanceBits);
          $6.push($8);
        }
      }
      return {
        eot: eot
      };
    }
    function soundEnvelope($bytes, $stream, $, swfVersion, tagCode) {
      $.pos44 = readUi32($bytes, $stream);
      $.volumeLeft = readUi16($bytes, $stream);
      $.volumeRight = readUi16($bytes, $stream);
    }
    function soundInfo($bytes, $stream, $, swfVersion, tagCode) {
      var reserved = readUb($bytes, $stream, 2);
      $.stop = readUb($bytes, $stream, 1);
      $.noMultiple = readUb($bytes, $stream, 1);
      var hasEnvelope = $.hasEnvelope = readUb($bytes, $stream, 1);
      var hasLoops = $.hasLoops = readUb($bytes, $stream, 1);
      var hasOutPoint = $.hasOutPoint = readUb($bytes, $stream, 1);
      var hasInPoint = $.hasInPoint = readUb($bytes, $stream, 1);
      if (hasInPoint) {
        $.inPoint = readUi32($bytes, $stream);
      }
      if (hasOutPoint) {
        $.outPoint = readUi32($bytes, $stream);
      }
      if (hasLoops) {
        $.loopCount = readUi16($bytes, $stream);
      }
      if (hasEnvelope) {
        var envelopeCount = $.envelopeCount = readUi8($bytes, $stream);
        var $1 = $.envelopes = [];
        var $2 = envelopeCount;
        while ($2--) {
          var $3 = {};
          soundEnvelope($bytes, $stream, $3, swfVersion, tagCode);
          $1.push($3);
        }
      }
    }
    function button($bytes, $stream, $, swfVersion, tagCode) {
      var hasFilters, blend;
      var flags = readUi8($bytes, $stream);
      var eob = $.eob = !flags;
      if (swfVersion >= 8) {
        blend = $.blend = flags >> 5 & 1;
        hasFilters = $.hasFilters = flags >> 4 & 1;
      } else {
        blend = $.blend = 0;
        hasFilters = $.hasFilters = 0;
      }
      $.stateHitTest = flags >> 3 & 1;
      $.stateDown = flags >> 2 & 1;
      $.stateOver = flags >> 1 & 1;
      $.stateUp = flags & 1;
      if (!eob) {
        $.symbolId = readUi16($bytes, $stream);
        $.depth = readUi16($bytes, $stream);
        var $2 = $.matrix = {};
        matrix($bytes, $stream, $2, swfVersion, tagCode);
        if (tagCode === 34) {
          var $3 = $.cxform = {};
          cxform($bytes, $stream, $3, swfVersion, tagCode);
        }
        if (hasFilters) {
          $.filterCount = readUi8($bytes, $stream);
          var $4 = $.filters = {};
          anyFilter($bytes, $stream, $4, swfVersion, tagCode);
        }
        if (blend) {
          $.blendMode = readUi8($bytes, $stream);
        }
      }
      return {
        eob: eob
      };
    }
    function buttonCondAction($bytes, $stream, $, swfVersion, tagCode) {
      var buttonCondSize = readUi16($bytes, $stream);
      var buttonConditions = readUi16($bytes, $stream);
      $.idleToOverDown = buttonConditions >> 7 & 1;
      $.outDownToIdle = buttonConditions >> 6 & 1;
      $.outDownToOverDown = buttonConditions >> 5 & 1;
      $.overDownToOutDown = buttonConditions >> 4 & 1;
      $.overDownToOverUp = buttonConditions >> 3 & 1;
      $.overUpToOverDown = buttonConditions >> 2 & 1;
      $.overUpToIdle = buttonConditions >> 1 & 1;
      $.idleToOverUp = buttonConditions & 1;
      $.mouseEventFlags = buttonConditions & 511;
      $.keyPress = buttonConditions >> 9 & 127;
      $.overDownToIdle = buttonConditions >> 8 & 1;
      if (!buttonCondSize) {
        $.actionsData = readBinary($bytes, $stream, 0);
      } else {
        $.actionsData = readBinary($bytes, $stream, buttonCondSize - 4);
      }
    }
    function shape($bytes, $stream, $, swfVersion, tagCode) {
      var eos;
      var temp = styleBits($bytes, $stream, $, swfVersion, tagCode);
      var fillBits = temp.fillBits;
      var lineBits = temp.lineBits;
      var $4 = $.records = [];
      do {
        var $5 = {};
        var isMorph = false;
        var hasStrokes = false;
        var temp = shapeRecord($bytes, $stream, $5, swfVersion, tagCode, isMorph, fillBits, lineBits, hasStrokes, bits);
        eos = temp.eos;
        var fillBits = temp.fillBits;
        var lineBits = temp.lineBits;
        var bits = bits;
        $4.push($5);
      } while (!eos);
    }
    return {
      0: undefined,
      1: undefined,
      2: defineShape,
      4: placeObject,
      5: removeObject,
      6: defineImage,
      7: defineButton,
      8: defineJPEGTables,
      9: setBackgroundColor,
      10: defineFont,
      11: defineLabel,
      12: doAction,
      13: undefined,
      14: defineSound,
      15: startSound,
      17: undefined,
      18: soundStreamHead,
      19: soundStreamBlock,
      20: defineBitmap,
      21: defineImage,
      22: defineShape,
      23: undefined,
      24: undefined,
      26: placeObject,
      28: removeObject,
      32: defineShape,
      33: defineLabel,
      34: defineButton,
      35: defineImage,
      36: defineBitmap,
      37: defineText,
      39: undefined,
      43: frameLabel,
      45: soundStreamHead,
      46: defineShape,
      48: defineFont2,
      56: undefined,
      57: undefined,
      58: undefined,
      59: doAction,
      60: undefined,
      61: undefined,
      62: undefined,
      64: undefined,
      65: undefined,
      66: undefined,
      69: fileAttributes,
      70: placeObject,
      71: undefined,
      72: doABC,
      73: undefined,
      74: undefined,
      75: defineFont2,
      76: symbolClass,
      77: undefined,
      78: defineScalingGrid,
      82: doABC,
      83: defineShape,
      84: defineShape,
      86: defineScene,
      87: defineBinaryData,
      88: undefined,
      89: startSound,
      90: defineImage,
      91: undefined
    };
  }(this);
var readHeader = function readHeader($bytes, $stream, $, swfVersion, tagCode) {
  $ || ($ = {});
  var $0 = $.bbox = {};
  align($bytes, $stream);
  var bits = readUb($bytes, $stream, 5);
  var xMin = readSb($bytes, $stream, bits);
  var xMax = readSb($bytes, $stream, bits);
  var yMin = readSb($bytes, $stream, bits);
  var yMax = readSb($bytes, $stream, bits);
  $0.xMin = xMin;
  $0.xMax = xMax;
  $0.yMin = yMin;
  $0.yMax = yMax;
  align($bytes, $stream);
  var frameRateFraction = readUi8($bytes, $stream);
  $.frameRate = readUi8($bytes, $stream) + frameRateFraction / 256;
  $.frameCount = readUi16($bytes, $stream);
  return $;
};
(function (global) {
  global['tagHandler'] = tagHandler;
  global['readHeader'] = readHeader;
}(this));
function readTags(context, stream, swfVersion, onprogress) {
  var tags = context.tags;
  var bytes = stream.bytes;
  var lastSuccessfulPosition;
  try {
    do {
      lastSuccessfulPosition = stream.pos;
      stream.ensure(2);
      var tagCodeAndLength = readUi16(bytes, stream);
      var tagCode = tagCodeAndLength >> 6;
      var length = tagCodeAndLength & 63;
      if (length === 63) {
        stream.ensure(4);
        length = readUi32(bytes, stream);
      }
      stream.ensure(length);
      if (tagCode === 0) {
        break;
      }
      var substream = stream.substream(stream.pos, stream.pos += length);
      var subbytes = substream.bytes;
      var tag = {
          code: tagCode
        };
      if (tagCode === 39) {
        tag.type = 'sprite';
        tag.id = readUi16(subbytes, substream);
        tag.frameCount = readUi16(subbytes, substream);
        tag.tags = [];
        readTags(tag, substream, swfVersion);
      } else {
        var handler = tagHandler[tagCode];
        if (handler) {
          handler(subbytes, substream, tag, swfVersion, tagCode);
        }
      }
      tags.push(tag);
      if (tagCode === 1) {
        while (stream.pos + 2 <= stream.end && stream.getUint16(stream.pos, true) >> 6 === 1) {
          tags.push(tag);
          stream.pos += 2;
        }
        if (onprogress)
          onprogress(context);
      } else if (onprogress && tag.id !== undefined) {
        onprogress(context);
      }
    } while (stream.pos < stream.end);
  } catch (e) {
    if (e !== StreamNoDataError)
      throw e;
    stream.pos = lastSuccessfulPosition;
  }
}
function HeadTailBuffer(defaultSize) {
  this.bufferSize = defaultSize || 16;
  this.buffer = new Uint8Array(this.bufferSize);
  this.pos = 0;
}
HeadTailBuffer.prototype = {
  push: function (data, need) {
    var bufferLengthNeed = this.pos + data.length;
    if (this.bufferSize < bufferLengthNeed) {
      var newBufferSize = this.bufferSize;
      while (newBufferSize < bufferLengthNeed) {
        newBufferSize <<= 1;
      }
      var newBuffer = new Uint8Array(newBufferSize);
      if (this.bufferSize > 0) {
        newBuffer.set(this.buffer);
      }
      this.buffer = newBuffer;
      this.bufferSize = newBufferSize;
    }
    this.buffer.set(data, this.pos);
    this.pos += data.length;
    if (need)
      return this.pos >= need;
  },
  getHead: function (size) {
    return this.buffer.subarray(0, size);
  },
  getTail: function (offset) {
    return this.buffer.subarray(offset, this.pos);
  },
  removeHead: function (size) {
    var tail = this.getTail(size);
    this.buffer = new Uint8Array(this.bufferSize);
    this.buffer.set(tail);
    this.pos = tail.length;
  },
  get arrayBuffer() {
    return this.buffer.buffer;
  },
  get length() {
    return this.pos;
  },
  createStream: function () {
    return new Stream(this.arrayBuffer, 0, this.length);
  }
};
function CompressedPipe(target, length) {
  this.target = target;
  this.length = length;
  this.initialize = true;
  this.buffer = new HeadTailBuffer(8096);
  this.state = {
    bitBuffer: 0,
    bitLength: 0,
    compression: {}
  };
  this.output = {
    data: new Uint8Array(length),
    available: 0,
    completed: false
  };
}
CompressedPipe.prototype = {
  push: function (data, progressInfo) {
    var buffer = this.buffer;
    if (this.initialize) {
      if (!buffer.push(data, 2))
        return;
      var headerBytes = buffer.getHead(2);
      verifyDeflateHeader(headerBytes);
      buffer.removeHead(2);
      this.initialize = false;
    } else {
      buffer.push(data);
    }
    var stream = buffer.createStream();
    stream.bitBuffer = this.state.bitBuffer;
    stream.bitLength = this.state.bitLength;
    var output = this.output;
    var lastAvailable = output.available;
    try {
      do {
        inflateBlock(stream, output, this.state.compression);
      } while (stream.pos < buffer.length && !output.completed);
    } catch (e) {
      if (e !== InflateNoDataError)
        throw e;
    } finally {
      this.state.bitBuffer = stream.bitBuffer;
      this.state.bitLength = stream.bitLength;
    }
    buffer.removeHead(stream.pos);
    this.target.push(output.data.subarray(lastAvailable, output.available), progressInfo);
  }
};
function BodyParser(swfVersion, length, options) {
  this.swf = {
    swfVersion: swfVersion
  };
  this.buffer = new HeadTailBuffer(32768);
  this.initialize = true;
  this.totalRead = 0;
  this.length = length;
  this.options = options;
}
BodyParser.prototype = {
  push: function (data, progressInfo) {
    if (data.length === 0)
      return;
    var swf = this.swf;
    var swfVersion = swf.swfVersion;
    var buffer = this.buffer;
    var options = this.options;
    var stream;
    if (this.initialize) {
      var PREFETCH_SIZE = 27;
      if (!buffer.push(data, PREFETCH_SIZE))
        return;
      stream = buffer.createStream();
      var bytes = stream.bytes;
      readHeader(bytes, stream, swf);
      var nextTagHeader = readUi16(bytes, stream);
      var FILE_ATTRIBUTES_LENGTH = 4;
      if (nextTagHeader == (SWF_TAG_CODE_FILE_ATTRIBUTES << 6 | FILE_ATTRIBUTES_LENGTH)) {
        stream.ensure(FILE_ATTRIBUTES_LENGTH);
        var substream = stream.substream(stream.pos, stream.pos += FILE_ATTRIBUTES_LENGTH);
        var handler = tagHandler[SWF_TAG_CODE_FILE_ATTRIBUTES];
        var fileAttributesTag = {
            code: SWF_TAG_CODE_FILE_ATTRIBUTES
          };
        handler(substream.bytes, substream, fileAttributesTag, swfVersion, SWF_TAG_CODE_FILE_ATTRIBUTES);
        swf.fileAttributes = fileAttributesTag;
      } else {
        stream.pos -= 2;
        swf.fileAttributes = {};
      }
      if (options.onstart)
        options.onstart(swf);
      swf.tags = [];
      this.initialize = false;
    } else {
      buffer.push(data);
      stream = buffer.createStream();
    }
    if (progressInfo) {
      swf.bytesLoaded = progressInfo.bytesLoaded;
      swf.bytesTotal = progressInfo.bytesTotal;
    }
    readTags(swf, stream, swfVersion, options.onprogress);
    var read = stream.pos;
    buffer.removeHead(read);
    this.totalRead += read;
    if (this.totalRead >= this.length && options.oncomplete) {
      options.oncomplete(swf);
    }
  }
};
SWF.parseAsync = function swf_parseAsync(options) {
  var buffer = new HeadTailBuffer();
  var pipe = {
      push: function (data, progressInfo) {
        if (this.target !== undefined) {
          return this.target.push(data, progressInfo);
        }
        if (!buffer.push(data, 8)) {
          return null;
        }
        var bytes = buffer.getHead(8);
        var magic1 = bytes[0];
        var magic2 = bytes[1];
        var magic3 = bytes[2];
        if ((magic1 === 70 || magic1 === 67) && magic2 === 87 && magic3 === 83) {
          var swfVersion = bytes[3];
          var compressed = magic1 === 67;
          parseSWF(compressed, swfVersion, progressInfo);
          buffer = null;
          return;
        }
        var isImage = false;
        var imageType;
        if (magic1 === 255 && magic2 === 216 && magic3 === 255) {
          isImage = true;
          imageType = 'image/jpeg';
        } else if (magic1 === 137 && magic2 === 80 && magic3 === 78) {
          isImage = true;
          imageType = 'image/png';
        }
        if (isImage) {
          parseImage(data, progressInfo.bytesTotal, imageType);
        }
        buffer = null;
      },
      close: function () {
        if (buffer) {
          var symbol = {
              command: 'empty',
              data: buffer.buffer.subarray(0, buffer.pos)
            };
          options.oncomplete && options.oncomplete(symbol);
        }
        if (this.target !== undefined && this.target.close) {
          this.target.close();
        }
      }
    };
  function parseSWF(compressed, swfVersion, progressInfo) {
    var stream = buffer.createStream();
    stream.pos += 4;
    var fileLength = readUi32(null, stream);
    var bodyLength = fileLength - 8;
    var target = new BodyParser(swfVersion, bodyLength, options);
    if (compressed) {
      target = new CompressedPipe(target, bodyLength);
    }
    target.push(buffer.getTail(8), progressInfo);
    pipe['target'] = target;
  }
  function parseImage(data, bytesTotal, type) {
    var buffer = new Uint8Array(bytesTotal);
    buffer.set(data);
    var bufferPos = data.length;
    pipe['target'] = {
      push: function (data) {
        buffer.set(data, bufferPos);
        bufferPos += data.length;
      },
      close: function () {
        var props = {};
        var chunks;
        if (type == 'image/jpeg') {
          chunks = parseJpegChunks(props, buffer);
        } else {
          chunks = [
            buffer
          ];
        }
        var symbol = {
            type: 'image',
            props: props,
            data: new Blob(chunks, {
              type: type
            })
          };
        options.oncomplete && options.oncomplete(symbol);
      }
    };
  }
  return pipe;
};
SWF.parse = function (buffer, options) {
  if (!options)
    options = {};
  var pipe = SWF.parseAsync(options);
  var bytes = new Uint8Array(buffer);
  var progressInfo = {
      bytesLoaded: bytes.length,
      bytesTotal: bytes.length
    };
  pipe.push(bytes, progressInfo);
};
(function (global) {
  global['SWF']['parse'] = SWF.parse;
  global['SWF']['parseAsync'] = SWF.parseAsync;
}(this));
