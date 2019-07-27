











!(function(window) {

!(function(window, OTHelpers, undefined) {


























var prototypeSlice = Array.prototype.slice;



var detectIdSelectors = /^#([\w-]*)$/;


































var OTHelpers = function(selector, context) {
  var results = [];

  if (typeof(selector) === 'string') {
    var idSelector = detectIdSelectors.exec(selector);
    context = context || document;

    if (idSelector && idSelector[1]) {
      var element = context.getElementById(idSelector[1]);
      if (element) results.push(element);
    }
    else {
      results = context.querySelectorAll(selector);
    }
  }
  else if (selector.nodeType || window.XMLHttpRequest && selector instanceof XMLHttpRequest) {
    
    results = [selector];
    context = selector;
  }
  else if (OTHelpers.isArray(selector)) {
    results = selector.slice();
    context  = null;
  }

  return new ElementCollection(results, context);
};


var $ = OTHelpers;


var nodeListToArray = function nodeListToArray (nodes) {
  if ($.env.name !== 'IE' || $.env.version > 9) {
    return prototypeSlice.call(nodes);
  }

  
  
  var array = [];

  for (var i=0, num=nodes.length; i<num; ++i) {
    array.push(nodes[i]);
  }

  return array;
};



























var ElementCollection = function ElementCollection (elems, context) {
  var elements = nodeListToArray(elems);
  this.context = context;
  this.toArray = function() { return elements; };

  this.length = elements.length;
  this.first = elements[0];
  this.last = elements[elements.length-1];

  this.get = function(index) {
    if (index === void 0) return elements;
    return elements[index];
  };
};


ElementCollection.prototype.some = function (iter, context) {
  return $.some(this.get(), iter, context);
};

ElementCollection.prototype.forEach = function(fn, context) {
  $.forEach(this.get(), fn, context);
  return this;
};

ElementCollection.prototype.map = function(fn, context) {
  return new ElementCollection($.map(this.get(), fn, context), this.context);
};

ElementCollection.prototype.filter = function(fn, context) {
  return new ElementCollection($.filter(this.get(), fn, context), this.context);
};

ElementCollection.prototype.find = function(selector) {
  return $(selector, this.first);
};


var previousOTHelpers = window.OTHelpers;

window.OTHelpers = OTHelpers;


window.___othelpers = true;

OTHelpers.keys = Object.keys || function(object) {
  var keys = [], hasOwnProperty = Object.prototype.hasOwnProperty;
  for(var key in object) {
    if(hasOwnProperty.call(object, key)) {
      keys.push(key);
    }
  }
  return keys;
};

var _each = Array.prototype.forEach || function(iter, ctx) {
  for(var idx = 0, count = this.length || 0; idx < count; ++idx) {
    if(idx in this) {
      iter.call(ctx, this[idx], idx);
    }
  }
};

OTHelpers.forEach = function(array, iter, ctx) {
  return _each.call(array, iter, ctx);
};

var _map = Array.prototype.map || function(iter, ctx) {
  var collect = [];
  _each.call(this, function(item, idx) {
    collect.push(iter.call(ctx, item, idx));
  });
  return collect;
};

OTHelpers.map = function(array, iter) {
  return _map.call(array, iter);
};

var _filter = Array.prototype.filter || function(iter, ctx) {
  var collect = [];
  _each.call(this, function(item, idx) {
    if(iter.call(ctx, item, idx)) {
      collect.push(item);
    }
  });
  return collect;
};

OTHelpers.filter = function(array, iter, ctx) {
  return _filter.call(array, iter, ctx);
};

var _some = Array.prototype.some || function(iter, ctx) {
  var any = false;
  for(var idx = 0, count = this.length || 0; idx < count; ++idx) {
    if(idx in this) {
      if(iter.call(ctx, this[idx], idx)) {
        any = true;
        break;
      }
    }
  }
  return any;
};

OTHelpers.find = function(array, iter, ctx) {
  if (!$.isFunction(iter)) {
    throw new TypeError('iter must be a function');
  }

  var any = void 0;
  for(var idx = 0, count = array.length || 0; idx < count; ++idx) {
    if(idx in array) {
      if(iter.call(ctx, array[idx], idx)) {
        any = array[idx];
        break;
      }
    }
  }
  return any;
};

OTHelpers.findIndex = function(array, iter, ctx) {
  if (!$.isFunction(iter)) {
    throw new TypeError('iter must be a function');
  }

  for (var i = 0, count = array.length || 0; i < count; ++i) {
    if (i in array && iter.call(ctx, array[i], i, array)) {
      return i;
    }
  }

  return -1;
};


OTHelpers.some = function(array, iter, ctx) {
  return _some.call(array, iter, ctx);
};

var _indexOf = Array.prototype.indexOf || function(searchElement, fromIndex) {
  var i,
      pivot = (fromIndex) ? fromIndex : 0,
      length;

  if (!this) {
    throw new TypeError();
  }

  length = this.length;

  if (length === 0 || pivot >= length) {
    return -1;
  }

  if (pivot < 0) {
    pivot = length - Math.abs(pivot);
  }

  for (i = pivot; i < length; i++) {
    if (this[i] === searchElement) {
      return i;
    }
  }
  return -1;
};

OTHelpers.arrayIndexOf = function(array, searchElement, fromIndex) {
  return _indexOf.call(array, searchElement, fromIndex);
};

var _bind = Function.prototype.bind || function() {
  var args = prototypeSlice.call(arguments),
      ctx = args.shift(),
      fn = this;
  return function() {
    return fn.apply(ctx, args.concat(prototypeSlice.call(arguments)));
  };
};

OTHelpers.bind = function() {
  var args = prototypeSlice.call(arguments),
      fn = args.shift();
  return _bind.apply(fn, args);
};

var _trim = String.prototype.trim || function() {
  return this.replace(/^\s+|\s+$/g, '');
};

OTHelpers.trim = function(str) {
  return _trim.call(str);
};

OTHelpers.noConflict = function() {
  OTHelpers.noConflict = function() {
    return OTHelpers;
  };
  window.OTHelpers = previousOTHelpers;
  return OTHelpers;
};

OTHelpers.isNone = function(obj) {
  return obj === void 0 || obj === null;
};

OTHelpers.isObject = function(obj) {
  return obj === Object(obj);
};

OTHelpers.isFunction = function(obj) {
  return !!obj && (obj.toString().indexOf('()') !== -1 ||
    Object.prototype.toString.call(obj) === '[object Function]');
};

OTHelpers.isArray = OTHelpers.isFunction(Array.isArray) && Array.isArray ||
  function (vArg) {
    return Object.prototype.toString.call(vArg) === '[object Array]';
  };

OTHelpers.isEmpty = function(obj) {
  if (obj === null || obj === void 0) return true;
  if (OTHelpers.isArray(obj) || typeof(obj) === 'string') return obj.length === 0;

  
  for (var key in obj) {
    if (obj.hasOwnProperty(key)) return false;
  }

  return true;
};







OTHelpers.extend = function() {
  var sources = prototypeSlice.call(arguments),
      dest = sources.shift();

  OTHelpers.forEach(sources, function(source) {
    for (var key in source) {
      dest[key] = source[key];
    }
  });

  return dest;
};








OTHelpers.defaults = function() {
  var sources = prototypeSlice.call(arguments),
      dest = sources.shift();

  OTHelpers.forEach(sources, function(source) {
    for (var key in source) {
      if (dest[key] === void 0) dest[key] = source[key];
    }
  });

  return dest;
};

OTHelpers.clone = function(obj) {
  if (!OTHelpers.isObject(obj)) return obj;
  return OTHelpers.isArray(obj) ? obj.slice() : OTHelpers.extend({}, obj);
};




OTHelpers.noop = function() {};







OTHelpers.now = (function() {
  var performance = window.performance || {},
      navigationStart,
      now =  performance.now       ||
             performance.mozNow    ||
             performance.msNow     ||
             performance.oNow      ||
             performance.webkitNow;

  if (now) {
    now = OTHelpers.bind(now, performance);
    navigationStart = performance.timing.navigationStart;

    return  function() { return navigationStart + now(); };
  } else {
    return function() { return new Date().getTime(); };
  }
})();

OTHelpers.canDefineProperty = true;

try {
  Object.defineProperty({}, 'x', {});
} catch (err) {
  OTHelpers.canDefineProperty = false;
}













OTHelpers.defineGetters = function(self, getters, enumerable) {
  var propsDefinition = {};

  if (enumerable === void 0) enumerable = false;

  for (var key in getters) {
    propsDefinition[key] = {
      get: getters[key],
      enumerable: enumerable
    };
  }

  OTHelpers.defineProperties(self, propsDefinition);
};

var generatePropertyFunction = function(object, getter, setter) {
  if(getter && !setter) {
    return function() {
      return getter.call(object);
    };
  } else if(getter && setter) {
    return function(value) {
      if(value !== void 0) {
        setter.call(object, value);
      }
      return getter.call(object);
    };
  } else {
    return function(value) {
      if(value !== void 0) {
        setter.call(object, value);
      }
    };
  }
};

OTHelpers.defineProperties = function(object, getterSetters) {
  for (var key in getterSetters) {
    object[key] = generatePropertyFunction(object, getterSetters[key].get,
      getterSetters[key].set);
  }
};






if (!Object.create) {
  Object.create = function (o) {
    if (arguments.length > 1) {
      throw new Error('Object.create implementation only accepts the first parameter.');
    }
    function F() {}
    F.prototype = o;
    return new F();
  };
}










OTHelpers.invert = function(obj) {
  var result = {};
  for (var key in obj) if (obj.hasOwnProperty(key)) result[obj[key]] = key;
  return result;
};





OTHelpers.statable = function(self, possibleStates, initialState, stateChanged,
  stateChangedFailed) {
  var previousState,
      currentState = self.currentState = initialState;

  var setState = function(state) {
    if (currentState !== state) {
      if (OTHelpers.arrayIndexOf(possibleStates, state) === -1) {
        if (stateChangedFailed && OTHelpers.isFunction(stateChangedFailed)) {
          stateChangedFailed('invalidState', state);
        }
        return;
      }

      self.previousState = previousState = currentState;
      self.currentState = currentState = state;
      if (stateChanged && OTHelpers.isFunction(stateChanged)) stateChanged(state, previousState);
    }
  };


  
  
  
  
  
  
  
  
  self.is = function () {
    return OTHelpers.arrayIndexOf(arguments, currentState) !== -1;
  };


  
  
  
  
  
  
  
  
  self.isNot = function () {
    return OTHelpers.arrayIndexOf(arguments, currentState) === -1;
  };

  return setState;
};


















(function() {

  
  
  
  
  var mathRNG, whatwgRNG;

  
  var _rndBytes = new Array(16);
  mathRNG = function() {
    var r, b = _rndBytes, i = 0;

    for (i = 0; i < 16; i++) {
      if ((i & 0x03) === 0) r = Math.random() * 0x100000000;
      b[i] = r >>> ((i & 0x03) << 3) & 0xff;
    }

    return b;
  };

  
  
  if (window.crypto && crypto.getRandomValues) {
    var _rnds = new Uint32Array(4);
    whatwgRNG = function() {
      crypto.getRandomValues(_rnds);

      for (var c = 0 ; c < 16; c++) {
        _rndBytes[c] = _rnds[c >> 2] >>> ((c & 0x03) * 8) & 0xff;
      }
      return _rndBytes;
    };
  }

  
  var _rng = whatwgRNG || mathRNG;

  
  var BufferClass = typeof(Buffer) === 'function' ? Buffer : Array;

  
  var _byteToHex = [];
  var _hexToByte = {};
  for (var i = 0; i < 256; i++) {
    _byteToHex[i] = (i + 0x100).toString(16).substr(1);
    _hexToByte[_byteToHex[i]] = i;
  }

  
  function parse(s, buf, offset) {
    var i = (buf && offset) || 0, ii = 0;

    buf = buf || [];
    s.toLowerCase().replace(/[0-9a-f]{2}/g, function(oct) {
      if (ii < 16) { 
        buf[i + ii++] = _hexToByte[oct];
      }
    });

    
    while (ii < 16) {
      buf[i + ii++] = 0;
    }

    return buf;
  }

  
  function unparse(buf, offset) {
    var i = offset || 0, bth = _byteToHex;
    return  bth[buf[i++]] + bth[buf[i++]] +
            bth[buf[i++]] + bth[buf[i++]] + '-' +
            bth[buf[i++]] + bth[buf[i++]] + '-' +
            bth[buf[i++]] + bth[buf[i++]] + '-' +
            bth[buf[i++]] + bth[buf[i++]] + '-' +
            bth[buf[i++]] + bth[buf[i++]] +
            bth[buf[i++]] + bth[buf[i++]] +
            bth[buf[i++]] + bth[buf[i++]];
  }

  

  
  function v4(options, buf, offset) {
    
    var i = buf && offset || 0;

    if (typeof(options) === 'string') {
      buf = options === 'binary' ? new BufferClass(16) : null;
      options = null;
    }
    options = options || {};

    var rnds = options.random || (options.rng || _rng)();

    
    rnds[6] = (rnds[6] & 0x0f) | 0x40;
    rnds[8] = (rnds[8] & 0x3f) | 0x80;

    
    if (buf) {
      for (var ii = 0; ii < 16; ii++) {
        buf[i + ii] = rnds[ii];
      }
    }

    return buf || unparse(rnds);
  }

  
  var uuid = v4;
  uuid.v4 = v4;
  uuid.parse = parse;
  uuid.unparse = unparse;
  uuid.BufferClass = BufferClass;

  
  uuid.mathRNG = mathRNG;
  uuid.whatwgRNG = whatwgRNG;

  OTHelpers.uuid = uuid;

}());





var getErrorLocation;


var safeErrorProps = [
  'description',
  'fileName',
  'lineNumber',
  'message',
  'name',
  'number',
  'stack'
];
























































OTHelpers.Error = function (message, name, props) {
  switch (arguments.length) {
  case 1:
    if ($.isObject(message)) {
      props = message;
      name = void 0;
      message = void 0;
    }
    
    break;

  case 2:
    if ($.isObject(name)) {
      props = name;
      name = void 0;
    }
    

    break;
  }

  if ( props instanceof Error) {
    
    
    
    for (var i = 0, num = safeErrorProps.length; i < num; ++i) {
      this[safeErrorProps[i]] = props[safeErrorProps[i]];
    }
  }
  else if ( $.isObject(props)) {
    
    for (var key in props) {
      if (props.hasOwnProperty(key)) {
        this[key] = props[key];
      }
    }
  }

  
  
  if ( !(this.fileName && this.lineNumber && this.columnNumber && this.stack) ) {
    var err = getErrorLocation();

    if (!this.fileName && err.fileName) {
      this.fileName = err.fileName;
    }

    if (!this.lineNumber && err.lineNumber) {
      this.lineNumber = err.lineNumber;
    }

    if (!this.columnNumber && err.columnNumber) {
      this.columnNumber = err.columnNumber;
    }

    if (!this.stack && err.stack) {
      this.stack = err.stack;
    }
  }

  if (!this.message && message) this.message = message;
  if (!this.name && name) this.name = name;
};

OTHelpers.Error.prototype.toString =
OTHelpers.Error.prototype.valueOf = function() {
  var locationDetails = '';
  if (this.fileName) locationDetails += ' ' + this.fileName;
  if (this.lineNumber) {
    locationDetails += ' ' + this.lineNumber;
    if (this.columnNumber) locationDetails += ':' + this.columnNumber;
  }

  return '<' + (this.name ? this.name + ' ' : '') + this.message + locationDetails + '>';
};








var prepareStackTrace = function prepareStackTrace (_, stack){
  return $.map(stack.slice(2), function(frame) {
    var _f = {
      fileName: frame.getFileName(),
      linenumber: frame.getLineNumber(),
      columnNumber: frame.getColumnNumber()
    };

    if (frame.getFunctionName()) _f.functionName = frame.getFunctionName();
    if (frame.getMethodName()) _f.methodName = frame.getMethodName();
    if (frame.getThis()) _f.self = frame.getThis();

    return _f;
  });
};



getErrorLocation = function getErrorLocation () {
  var info = {},
      callstack,
      errLocation,
      err;

  switch ($.env.name) {
  case 'Firefox':
  case 'Safari':
  case 'IE':

    if ($.env.name === 'IE') {
      err = new Error();
    }
    else {
      try {
        window.call.js.is.explody;
      }
      catch(e) { err = e; }
    }

    callstack = err.stack.split('\n');

    
    callstack.shift();
    callstack.shift();

    info.stack = callstack;

    if ($.env.name === 'IE') {
      
      info.stack.shift();

      
      
      info.stack = $.map(callstack, function(call) {
        return call.replace(/^\s+at\s+/g, '');
      });
    }

    errLocation = /@(.+?):([0-9]+)(:([0-9]+))?$/.exec(callstack[0]);
    if (errLocation) {
      info.fileName = errLocation[1];
      info.lineNumber = parseInt(errLocation[2], 10);
      if (errLocation.length > 3) info.columnNumber = parseInt(errLocation[4], 10);
    }
    break;

  case 'Chrome':
  case 'Node':
  case 'Opera':
    var currentPST = Error.prepareStackTrace;
    Error.prepareStackTrace = prepareStackTrace;
    err = new Error();
    info.stack = err.stack;
    Error.prepareStackTrace = currentPST;

    var topFrame = info.stack[0];
    info.lineNumber = topFrame.lineNumber;
    info.columnNumber = topFrame.columnNumber;
    info.fileName = topFrame.fileName;
    if (topFrame.functionName) info.functionName = topFrame.functionName;
    if (topFrame.methodName) info.methodName = topFrame.methodName;
    if (topFrame.self) info.self = topFrame.self;
    break;

  default:
    err = new Error();
    if (err.stack) info.stack = err.stack.split('\n');
    break;
  }

  if (err.message) info.message = err.message;
  return info;
};






















(function() {
  
  var version = -1;

  
  
  var versionGEThan = function versionGEThan (otherVersion) {
    if (otherVersion === version) return true;

    if (typeof(otherVersion) === 'number' && typeof(version) === 'number') {
      return otherVersion > version;
    }

    
    
    
    
    var v1 = otherVersion.split('.'),
        v2 = version.split('.'),
        versionLength = (v1.length > v2.length ? v2 : v1).length;

    for (var i = 0; i < versionLength; ++i) {
      if (parseInt(v1[i], 10) > parseInt(v2[i], 10)) {
        return true;
      }
    }

    
    
    
    if (i < v1.length) {
      return true;
    }

    return false;
  };

  var env = function() {
    if (typeof(process) !== 'undefined' &&
        typeof(process.versions) !== 'undefined' &&
        typeof(process.versions.node) === 'string') {

      version = process.versions.node;
      if (version.substr(1) === 'v') version = version.substr(1);

      
      
      return {
        name: 'Node',
        version: version,
        userAgent: 'Node ' + version,
        iframeNeedsLoad: false,
        versionGreaterThan: versionGEThan
      };
    }

    var userAgent = window.navigator.userAgent.toLowerCase(),
        appName = window.navigator.appName,
        navigatorVendor,
        name = 'unknown';

    if (userAgent.indexOf('opera') > -1 || userAgent.indexOf('opr') > -1) {
      name = 'Opera';

      if (/opr\/([0-9]{1,}[\.0-9]{0,})/.exec(userAgent) !== null) {
        version = parseFloat( RegExp.$1 );
      }

    } else if (userAgent.indexOf('firefox') > -1)   {
      name = 'Firefox';

      if (/firefox\/([0-9]{1,}[\.0-9]{0,})/.exec(userAgent) !== null) {
        version = parseFloat( RegExp.$1 );
      }

    } else if (appName === 'Microsoft Internet Explorer') {
      
      name = 'IE';

      if (/msie ([0-9]{1,}[\.0-9]{0,})/.exec(userAgent) !== null) {
        version = parseFloat( RegExp.$1 );
      }

    } else if (appName === 'Netscape' && userAgent.indexOf('trident') > -1) {
      

      name = 'IE';

      if (/trident\/.*rv:([0-9]{1,}[\.0-9]{0,})/.exec(userAgent) !== null) {
        version = parseFloat( RegExp.$1 );
      }

    } else if (userAgent.indexOf('chrome') > -1) {
      name = 'Chrome';

      if (/chrome\/([0-9]{1,}[\.0-9]{0,})/.exec(userAgent) !== null) {
        version = parseFloat( RegExp.$1 );
      }

    } else if ((navigatorVendor = window.navigator.vendor) &&
      navigatorVendor.toLowerCase().indexOf('apple') > -1) {
      name = 'Safari';

      if (/version\/([0-9]{1,}[\.0-9]{0,})/.exec(userAgent) !== null) {
        version = parseFloat( RegExp.$1 );
      }
    }

    return {
      name: name,
      version: version,
      userAgent: window.navigator.userAgent,
      iframeNeedsLoad: userAgent.indexOf('webkit') < 0,
      versionGreaterThan: versionGEThan
    };
  }();


  OTHelpers.env = env;

  OTHelpers.browser = function() {
    return OTHelpers.env.name;
  };

  OTHelpers.browserVersion = function() {
    return OTHelpers.env;
  };

})();






if (window.OTHelpers.env.name === 'Node') {
  var request = require('request');

  OTHelpers.request = function(url, options, callback) {
    var completion = function(error, response, body) {
      var event = {response: response, body: body};

      
      
      if (!error && response.statusCode >= 200 &&
                  (response.statusCode < 300 || response.statusCode === 304) ) {
        callback(null, event);
      } else {
        callback(error, event);
      }
    };

    if (options.method.toLowerCase() === 'get') {
      request.get(url, completion);
    }
    else {
      request.post(url, options.body, completion);
    }
  };

  OTHelpers.getJSON = function(url, options, callback) {
    var extendedHeaders = require('underscore').extend(
      {
        'Accept': 'application/json'
      },
      options.headers || {}
    );

    request.get({
      url: url,
      headers: extendedHeaders,
      json: true
    }, function(err, response) {
      callback(err, response && response.body);
    });
  };
}





function formatPostData(data) { 
  
  if (typeof(data) === 'string') return data;

  var queryString = [];

  for (var key in data) {
    queryString.push(
      encodeURIComponent(key) + '=' + encodeURIComponent(data[key])
    );
  }

  return queryString.join('&').replace(/\+/g, '%20');
}

if (window.OTHelpers.env.name !== 'Node') {

  OTHelpers.xdomainRequest = function(url, options, callback) {
    
    var xdr = new XDomainRequest(),
        _options = options || {},
        _method = _options.method.toLowerCase();

    if(!_method) {
      callback(new Error('No HTTP method specified in options'));
      return;
    }

    _method = _method.toUpperCase();

    if(!(_method === 'GET' || _method === 'POST')) {
      callback(new Error('HTTP method can only be '));
      return;
    }

    function done(err, event) {
      xdr.onload = xdr.onerror = xdr.ontimeout = function() {};
      xdr = void 0;
      callback(err, event);
    }


    xdr.onload = function() {
      done(null, {
        target: {
          responseText: xdr.responseText,
          headers: {
            'content-type': xdr.contentType
          }
        }
      });
    };

    xdr.onerror = function() {
      done(new Error('XDomainRequest of ' + url + ' failed'));
    };

    xdr.ontimeout = function() {
      done(new Error('XDomainRequest of ' + url + ' timed out'));
    };

    xdr.open(_method, url);
    xdr.send(options.body && formatPostData(options.body));

  };

  OTHelpers.request = function(url, options, callback) {
    var request = new XMLHttpRequest(),
        _options = options || {},
        _method = _options.method;

    if(!_method) {
      callback(new Error('No HTTP method specified in options'));
      return;
    }

    
    
    
    if(callback) {
      OTHelpers.on(request, 'load', function(event) {
        var status = event.target.status;

        
        
        if ( status >= 200 && (status < 300 || status === 304) ) {
          callback(null, event);
        } else {
          callback(event);
        }
      });

      OTHelpers.on(request, 'error', callback);
    }

    request.open(options.method, url, true);

    if (!_options.headers) _options.headers = {};

    for (var name in _options.headers) {
      request.setRequestHeader(name, _options.headers[name]);
    }

    request.send(options.body && formatPostData(options.body));
  };


  OTHelpers.getJSON = function(url, options, callback) {
    options = options || {};

    var done = function(error, event) {
      if(error) {
        callback(error, event && event.target && event.target.responseText);
      } else {
        var response;

        try {
          response = JSON.parse(event.target.responseText);
        } catch(e) {
          
          callback(e, event && event.target && event.target.responseText);
          return;
        }

        callback(null, response, event);
      }
    };

    if(options.xdomainrequest) {
      OTHelpers.xdomainRequest(url, { method: 'GET' }, done);
    } else {
      var extendedHeaders = OTHelpers.extend({
        'Accept': 'application/json'
      }, options.headers || {});

      OTHelpers.get(url, OTHelpers.extend(options || {}, {
        headers: extendedHeaders
      }), done);
    }

  };

}





OTHelpers.useLogHelpers = function(on){

  
  on.DEBUG    = 5;
  on.LOG      = 4;
  on.INFO     = 3;
  on.WARN     = 2;
  on.ERROR    = 1;
  on.NONE     = 0;

  var _logLevel = on.NONE,
      _logs = [],
      _canApplyConsole = true;

  try {
    Function.prototype.bind.call(window.console.log, window.console);
  } catch (err) {
    _canApplyConsole = false;
  }

  
  
  
  var makeLogArgumentsSafe = function(args) { return args; };

  if (OTHelpers.env.name === 'IE') {
    makeLogArgumentsSafe = function(args) {
      return [toDebugString(prototypeSlice.apply(args))];
    };
  }

  
  
  
  
  
  
  
  
  function generateLoggingMethod(method, level, fallback) {
    return function() {
      if (on.shouldLog(level)) {
        var cons = window.console,
            args = makeLogArgumentsSafe(arguments);

        
        
        
        if (cons && cons[method]) {
          
          
          
          if (cons[method].apply || _canApplyConsole) {
            if (!cons[method].apply) {
              cons[method] = Function.prototype.bind.call(cons[method], cons);
            }

            cons[method].apply(cons, args);
          }
          else {
            
            
            cons[method](args);
          }
        }
        else if (fallback) {
          fallback.apply(on, args);

          
          return;
        }

        appendToLogs(method, makeLogArgumentsSafe(arguments));
      }
    };
  }

  on.log = generateLoggingMethod('log', on.LOG);

  
  on.debug = generateLoggingMethod('debug', on.DEBUG, on.log);
  on.info = generateLoggingMethod('info', on.INFO, on.log);
  on.warn = generateLoggingMethod('warn', on.WARN, on.log);
  on.error = generateLoggingMethod('error', on.ERROR, on.log);


  on.setLogLevel = function(level) {
    _logLevel = typeof(level) === 'number' ? level : 0;
    on.debug('TB.setLogLevel(' + _logLevel + ')');
    return _logLevel;
  };

  on.getLogs = function() {
    return _logs;
  };

  
  on.shouldLog = function(level) {
    return _logLevel >= level;
  };

  
  
  function formatDateStamp() {
    var now = new Date();
    return now.toLocaleTimeString() + now.getMilliseconds();
  }

  function toJson(object) {
    try {
      return JSON.stringify(object);
    } catch(e) {
      return object.toString();
    }
  }

  function toDebugString(object) {
    var components = [];

    if (typeof(object) === 'undefined') {
      
    }
    else if (object === null) {
      components.push('NULL');
    }
    else if (OTHelpers.isArray(object)) {
      for (var i=0; i<object.length; ++i) {
        components.push(toJson(object[i]));
      }
    }
    else if (OTHelpers.isObject(object)) {
      for (var key in object) {
        var stringValue;

        if (!OTHelpers.isFunction(object[key])) {
          stringValue = toJson(object[key]);
        }
        else if (object.hasOwnProperty(key)) {
          stringValue = 'function ' + key + '()';
        }

        components.push(key + ': ' + stringValue);
      }
    }
    else if (OTHelpers.isFunction(object)) {
      try {
        components.push(object.toString());
      } catch(e) {
        components.push('function()');
      }
    }
    else  {
      components.push(object.toString());
    }

    return components.join(', ');
  }

  
  function appendToLogs(level, args) {
    if (!args) return;

    var message = toDebugString(args);
    if (message.length <= 2) return;

    _logs.push(
      [level, formatDateStamp(), message]
    );
  }
};

OTHelpers.useLogHelpers(OTHelpers);
OTHelpers.setLogLevel(OTHelpers.ERROR);












ElementCollection.prototype.on = function (eventName, handler) {
  return this.forEach(function(element) {
    if (element.addEventListener) {
      element.addEventListener(eventName, handler, false);
    } else if (element.attachEvent) {
      element.attachEvent('on' + eventName, handler);
    } else {
      var oldHandler = element['on'+eventName];
      element['on'+eventName] = function() {
        handler.apply(this, arguments);
        if (oldHandler) oldHandler.apply(this, arguments);
      };
    }
  });
};


ElementCollection.prototype.off = function (eventName, handler) {
  return this.forEach(function(element) {
    if (element.removeEventListener) {
      element.removeEventListener (eventName, handler,false);
    }
    else if (element.detachEvent) {
      element.detachEvent('on' + eventName, handler);
    }
  });
};

ElementCollection.prototype.once = function (eventName, handler) {
  var removeAfterTrigger = $.bind(function() {
    this.off(eventName, removeAfterTrigger);
    handler.apply(null, arguments);
  }, this);

  return this.on(eventName, removeAfterTrigger);
};


OTHelpers.on = function(element, eventName,  handler) {
  return $(element).on(eventName, handler);
};


OTHelpers.off = function(element, eventName, handler) {
  return $(element).off(eventName, handler);
};


OTHelpers.once = function (element, eventName, handler) {
  return $(element).once(eventName, handler);
};






(function() {

  var _domReady = typeof(document) === 'undefined' ||
                    document.readyState === 'complete' ||
                   (document.readyState === 'interactive' && document.body),

      _loadCallbacks = [],
      _unloadCallbacks = [],
      _domUnloaded = false,

      onDomReady = function() {
        _domReady = true;

        if (typeof(document) !== 'undefined') {
          if ( document.addEventListener ) {
            document.removeEventListener('DOMContentLoaded', onDomReady, false);
            window.removeEventListener('load', onDomReady, false);
          } else {
            document.detachEvent('onreadystatechange', onDomReady);
            window.detachEvent('onload', onDomReady);
          }
        }

        
        
        OTHelpers.on(window, 'unload', onDomUnload);

        OTHelpers.forEach(_loadCallbacks, function(listener) {
          listener[0].call(listener[1]);
        });

        _loadCallbacks = [];
      },

      onDomUnload = function() {
        _domUnloaded = true;

        OTHelpers.forEach(_unloadCallbacks, function(listener) {
          listener[0].call(listener[1]);
        });

        _unloadCallbacks = [];
      };


  OTHelpers.onDOMLoad = function(cb, context) {
    if (OTHelpers.isReady()) {
      cb.call(context);
      return;
    }

    _loadCallbacks.push([cb, context]);
  };

  OTHelpers.onDOMUnload = function(cb, context) {
    if (this.isDOMUnloaded()) {
      cb.call(context);
      return;
    }

    _unloadCallbacks.push([cb, context]);
  };

  OTHelpers.isReady = function() {
    return !_domUnloaded && _domReady;
  };

  OTHelpers.isDOMUnloaded = function() {
    return _domUnloaded;
  };

  if (_domReady) {
    onDomReady();
  } else if(typeof(document) !== 'undefined') {
    if (document.addEventListener) {
      document.addEventListener('DOMContentLoaded', onDomReady, false);

      
      window.addEventListener( 'load', onDomReady, false );

    } else if (document.attachEvent) {
      document.attachEvent('onreadystatechange', function() {
        if (document.readyState === 'complete') onDomReady();
      });

      
      window.attachEvent( 'onload', onDomReady );
    }
  }

})();





OTHelpers.setCookie = function(key, value) {
  try {
    localStorage.setItem(key, value);
  } catch (err) {
    
    var date = new Date();
    date.setTime(date.getTime()+(365*24*60*60*1000));
    var expires = '; expires=' + date.toGMTString();
    document.cookie = key + '=' + value + expires + '; path=/';
  }
};

OTHelpers.getCookie = function(key) {
  var value;

  try {
    value = localStorage.getItem(key);
    return value;
  } catch (err) {
    
    var nameEQ = key + '=';
    var ca = document.cookie.split(';');
    for(var i=0;i < ca.length;i++) {
      var c = ca[i];
      while (c.charAt(0) === ' ') {
        c = c.substring(1,c.length);
      }
      if (c.indexOf(nameEQ) === 0) {
        value = c.substring(nameEQ.length,c.length);
      }
    }

    if (value) {
      return value;
    }
  }

  return null;
};







OTHelpers.Collection = function(idField) {
  var _models = [],
      _byId = {},
      _idField = idField || 'id';

  OTHelpers.eventing(this, true);

  var modelProperty = function(model, property) {
    if(OTHelpers.isFunction(model[property])) {
      return model[property]();
    } else {
      return model[property];
    }
  };

  var onModelUpdate = OTHelpers.bind(function onModelUpdate (event) {
        this.trigger('update', event);
        this.trigger('update:'+event.target.id, event);
      }, this),

      onModelDestroy = OTHelpers.bind(function onModelDestroyed (event) {
        this.remove(event.target, event.reason);
      }, this);


  this.reset = function() {
    
    OTHelpers.forEach(_models, function(model) {
      model.off('updated', onModelUpdate, this);
      model.off('destroyed', onModelDestroy, this);
    }, this);

    _models = [];
    _byId = {};
  };

  this.destroy = function(reason) {
    OTHelpers.forEach(_models, function(model) {
      if(model && typeof model.destroy === 'function') {
        model.destroy(reason, true);
      }
    });

    this.reset();
    this.off();
  };

  this.get = function(id) { return id && _byId[id] !== void 0 ? _models[_byId[id]] : void 0; };
  this.has = function(id) { return id && _byId[id] !== void 0; };

  this.toString = function() { return _models.toString(); };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  this.where = function(attrsOrFilterFn, context) {
    if (OTHelpers.isFunction(attrsOrFilterFn)) {
      return OTHelpers.filter(_models, attrsOrFilterFn, context);
    }

    return OTHelpers.filter(_models, function(model) {
      for (var key in attrsOrFilterFn) {
        if(!attrsOrFilterFn.hasOwnProperty(key)) {
          continue;
        }
        if (modelProperty(model, key) !== attrsOrFilterFn[key]) return false;
      }

      return true;
    });
  };

  
  
  this.find = function(attrsOrFilterFn, context) {
    var filterFn;

    if (OTHelpers.isFunction(attrsOrFilterFn)) {
      filterFn = attrsOrFilterFn;
    }
    else {
      filterFn = function(model) {
        for (var key in attrsOrFilterFn) {
          if(!attrsOrFilterFn.hasOwnProperty(key)) {
            continue;
          }
          if (modelProperty(model, key) !== attrsOrFilterFn[key]) return false;
        }

        return true;
      };
    }

    filterFn = OTHelpers.bind(filterFn, context);

    for (var i=0; i<_models.length; ++i) {
      if (filterFn(_models[i]) === true) return _models[i];
    }

    return null;
  };

  this.add = function(model) {
    var id = modelProperty(model, _idField);

    if (this.has(id)) {
      OTHelpers.warn('Model ' + id + ' is already in the collection', _models);
      return this;
    }

    _byId[id] = _models.push(model) - 1;

    model.on('updated', onModelUpdate, this);
    model.on('destroyed', onModelDestroy, this);

    this.trigger('add', model);
    this.trigger('add:'+id, model);

    return this;
  };

  this.remove = function(model, reason) {
    var id = modelProperty(model, _idField);

    _models.splice(_byId[id], 1);

    
    for (var i=_byId[id]; i<_models.length; ++i) {
      _byId[_models[i][_idField]] = i;
    }

    delete _byId[id];

    model.off('updated', onModelUpdate, this);
    model.off('destroyed', onModelDestroy, this);

    this.trigger('remove', model, reason);
    this.trigger('remove:'+id, model, reason);

    return this;
  };

  
  
  
  this._triggerAddEvents = function() {
    var models = this.where.apply(this, arguments);
    OTHelpers.forEach(models, function(model) {
      this.trigger('add', model);
      this.trigger('add:' + modelProperty(model, _idField), model);
    }, this);
  };

  this.length = function() {
    return _models.length;
  };
};








(function() {

  var _callAsync;

  
  
  
  
  
  var supportsPostMessage = (function () {
    if (window.postMessage) {
      
      
      
      var postMessageIsAsynchronous = true;
      var oldOnMessage = window.onmessage;
      window.onmessage = function() {
        postMessageIsAsynchronous = false;
      };
      window.postMessage('', '*');
      window.onmessage = oldOnMessage;
      return postMessageIsAsynchronous;
    }
  })();

  if (supportsPostMessage) {
    var timeouts = [],
        messageName = 'OTHelpers.' + OTHelpers.uuid.v4() + '.zero-timeout';

    var removeMessageHandler = function() {
      timeouts = [];

      if(window.removeEventListener) {
        window.removeEventListener('message', handleMessage);
      } else if(window.detachEvent) {
        window.detachEvent('onmessage', handleMessage);
      }
    };

    var handleMessage = function(event) {
      if (event.source === window &&
          event.data === messageName) {

        if(OTHelpers.isFunction(event.stopPropagation)) {
          event.stopPropagation();
        }
        event.cancelBubble = true;

        if (!window.___othelpers) {
          removeMessageHandler();
          return;
        }

        if (timeouts.length > 0) {
          var args = timeouts.shift(),
              fn = args.shift();

          fn.apply(null, args);
        }
      }
    };

    
    
    
    OTHelpers.on(window, 'unload', removeMessageHandler);

    if(window.addEventListener) {
      window.addEventListener('message', handleMessage, true);
    } else if(window.attachEvent) {
      window.attachEvent('onmessage', handleMessage);
    }

    _callAsync = function () {
      timeouts.push(prototypeSlice.call(arguments));
      window.postMessage(messageName, '*');
    };
  }
  else {
    _callAsync = function () {
      var args = prototypeSlice.call(arguments),
          fn = args.shift();

      setTimeout(function() {
        fn.apply(null, args);
      }, 0);
    };
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  OTHelpers.callAsync = _callAsync;


  
  
  
  OTHelpers.createAsyncHandler = function(handler) {
    return function() {
      var args = prototypeSlice.call(arguments);

      OTHelpers.callAsync(function() {
        handler.apply(null, args);
      });
    };
  };

})();













OTHelpers.eventing = function(self, syncronous) {
  var _events = {};

  
  function executeDefaultAction(defaultAction, args) {
    if (!defaultAction) return;

    defaultAction.apply(null, args.slice());
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  function executeListenersAsyncronously(name, args, defaultAction) {
    var listeners = _events[name];
    if (!listeners || listeners.length === 0) return;

    var listenerAcks = listeners.length;

    OTHelpers.forEach(listeners, function(listener) { 
      function filterHandlerAndContext(_listener) {
        return _listener.context === listener.context && _listener.handler === listener.handler;
      }

      
      
      OTHelpers.callAsync(function() {
        try {
          
          if (_events[name] && OTHelpers.some(_events[name], filterHandlerAndContext)) {
            (listener.closure || listener.handler).apply(listener.context || null, args);
          }
        }
        finally {
          listenerAcks--;

          if (listenerAcks === 0) {
            executeDefaultAction(defaultAction, args);
          }
        }
      });
    });
  }


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  function executeListenersSyncronously(name, args) { 
    var listeners = _events[name];
    if (!listeners || listeners.length === 0) return;

    OTHelpers.forEach(listeners, function(listener) { 
      (listener.closure || listener.handler).apply(listener.context || null, args);
    });
  }

  var executeListeners = syncronous === true ?
    executeListenersSyncronously : executeListenersAsyncronously;


  var removeAllListenersNamed = function (eventName, context) {
    if (_events[eventName]) {
      if (context) {
        
        
        _events[eventName] = OTHelpers.filter(_events[eventName], function(listener){
          return listener.context !== context;
        });
      }
      else {
        delete _events[eventName];
      }
    }
  };

  var addListeners = OTHelpers.bind(function (eventNames, handler, context, closure) {
    var listener = {handler: handler};
    if (context) listener.context = context;
    if (closure) listener.closure = closure;

    OTHelpers.forEach(eventNames, function(name) {
      if (!_events[name]) _events[name] = [];
      _events[name].push(listener);
      var addedListener = name + ':added';
      if (_events[addedListener]) {
        executeListeners(addedListener, [_events[name].length]);
      }
    });
  }, self);


  var removeListeners = function (eventNames, handler, context) {
    function filterHandlerAndContext(listener) {
      return !(listener.handler === handler && listener.context === context);
    }

    OTHelpers.forEach(eventNames, OTHelpers.bind(function(name) {
      if (_events[name]) {
        _events[name] = OTHelpers.filter(_events[name], filterHandlerAndContext);
        if (_events[name].length === 0) delete _events[name];
        var removedListener = name + ':removed';
        if (_events[ removedListener]) {
          executeListeners(removedListener, [_events[name] ? _events[name].length : 0]);
        }
      }
    }, self));

  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  self.dispatchEvent = function(event, defaultAction) {
    if (!event.type) {
      OTHelpers.error('OTHelpers.Eventing.dispatchEvent: Event has no type');
      OTHelpers.error(event);

      throw new Error('OTHelpers.Eventing.dispatchEvent: Event has no type');
    }

    if (!event.target) {
      event.target = this;
    }

    if (!_events[event.type] || _events[event.type].length === 0) {
      executeDefaultAction(defaultAction, [event]);
      return;
    }

    executeListeners(event.type, [event], defaultAction);

    return this;
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  self.trigger = function(eventName) {
    if (!_events[eventName] || _events[eventName].length === 0) {
      return;
    }

    var args = prototypeSlice.call(arguments);

    
    args.shift();

    executeListeners(eventName, args);

    return this;
  };

 






































































  self.on = function(eventNames, handlerOrContext, context) {
    if (typeof(eventNames) === 'string' && handlerOrContext) {
      addListeners(eventNames.split(' '), handlerOrContext, context);
    }
    else {
      for (var name in eventNames) {
        if (eventNames.hasOwnProperty(name)) {
          addListeners([name], eventNames[name], handlerOrContext);
        }
      }
    }

    return this;
  };

 
























































  self.off = function(eventNames, handlerOrContext, context) {
    if (typeof eventNames === 'string') {
      if (handlerOrContext && OTHelpers.isFunction(handlerOrContext)) {
        removeListeners(eventNames.split(' '), handlerOrContext, context);
      }
      else {
        OTHelpers.forEach(eventNames.split(' '), function(name) {
          removeAllListenersNamed(name, handlerOrContext);
        }, this);
      }

    } else if (!eventNames) {
      
      _events = {};

    } else {
      for (var name in eventNames) {
        if (eventNames.hasOwnProperty(name)) {
          removeListeners([name], eventNames[name], handlerOrContext);
        }
      }
    }

    return this;
  };


 








































































  self.once = function(eventNames, handler, context) {
    var names = eventNames.split(' '),
        fun = OTHelpers.bind(function() {
          var result = handler.apply(context || null, arguments);
          removeListeners(names, handler, context);

          return result;
        }, this);

    addListeners(names, handler, context, fun);
    return this;
  };


  


























  
  
  self.addEventListener = function(eventName, handler, context) {
    OTHelpers.warn('The addEventListener() method is deprecated. Use on() or once() instead.');
    addListeners([eventName], handler, context);
  };


  























  
  
  self.removeEventListener = function(eventName, handler, context) {
    OTHelpers.warn('The removeEventListener() method is deprecated. Use off() instead.');
    removeListeners([eventName], handler, context);
  };


  return self;
};

OTHelpers.eventing.Event = function() {
  return function (type, cancelable) {
    this.type = type;
    this.cancelable = cancelable !== undefined ? cancelable : true;

    var _defaultPrevented = false;

    this.preventDefault = function() {
      if (this.cancelable) {
        _defaultPrevented = true;
      } else {
        OTHelpers.warn('Event.preventDefault :: Trying to preventDefault ' +
          'on an Event that isn\'t cancelable');
      }
    };

    this.isDefaultPrevented = function() {
      return _defaultPrevented;
    };
  };
};







OTHelpers.createElement = function(nodeName, attributes, children, doc) {
  var element = (doc || document).createElement(nodeName);

  if (attributes) {
    for (var name in attributes) {
      if (typeof(attributes[name]) === 'object') {
        if (!element[name]) element[name] = {};

        var subAttrs = attributes[name];
        for (var n in subAttrs) {
          element[name][n] = subAttrs[n];
        }
      }
      else if (name === 'className') {
        element.className = attributes[name];
      }
      else {
        element.setAttribute(name, attributes[name]);
      }
    }
  }

  var setChildren = function(child) {
    if(typeof child === 'string') {
      element.innerHTML = element.innerHTML + child;
    } else {
      element.appendChild(child);
    }
  };

  if($.isArray(children)) {
    $.forEach(children, setChildren);
  } else if(children) {
    setChildren(children);
  }

  return element;
};

OTHelpers.createButton = function(innerHTML, attributes, events) {
  var button = $.createElement('button', attributes, innerHTML);

  if (events) {
    for (var name in events) {
      if (events.hasOwnProperty(name)) {
        $.on(button, name, events[name]);
      }
    }

    button._boundEvents = events;
  }

  return button;
};







ElementCollection.prototype.appendTo = function(parentElement) {
  if (!parentElement) throw new Error('appendTo requires a DOMElement to append to.');

  return this.forEach(parentElement.appendChild.bind(parentElement));
};

ElementCollection.prototype.after = function(prevElement) {
  if (!prevElement) throw new Error('after requires a DOMElement to insert after');

  return this.forEach(function(element) {
    if (element.parentElement) {
      if (prevElement !== element.parentNode.lastChild) {
        element.parentElement.before(element, prevElement);
      }
      else {
        element.parentElement.appendChild(element);
      }
    }
  });
};

ElementCollection.prototype.before = function(nextElement) {
  if (!nextElement) throw new Error('before requires a DOMElement to insert before');

  return this.forEach(function(element) {
    if (element.parentElement) {
      element.parentElement.before(element, nextElement);
    }
  });
};

ElementCollection.prototype.remove = function () {
  return this.forEach(function(element) {
    if (element.parentNode) {
      element.parentNode.removeChild(element);
    }
  });
};

ElementCollection.prototype.empty = function () {
  return this.forEach(function(element) {
    
    
    
    
    
    while (element.firstChild) {
      element.removeChild(element.firstChild);
    }
  });
};




ElementCollection.prototype.isDisplayNone = function() {
  return this.some(function(element) {
    if ( (element.offsetWidth === 0 || element.offsetHeight === 0) &&
                $(element).css('display') === 'none') return true;

    if (element.parentNode && element.parentNode.style) {
      return $(element.parentNode).isDisplayNone();
    }
  });
};

ElementCollection.prototype.findElementWithDisplayNone = function(element) {
  return $.findElementWithDisplayNone(element);
};



OTHelpers.isElementNode = function(node) {
  return node && typeof node === 'object' && node.nodeType === 1;
};



OTHelpers.removeElement = function(element) {
  $(element).remove();
};


OTHelpers.removeElementById = function(elementId) {
  return $('#'+elementId).remove();
};


OTHelpers.removeElementsByType = function(parentElem, type) {
  return $(type, parentElem).remove();
};


OTHelpers.emptyElement = function(element) {
  return $(element).empty();
};






OTHelpers.isDisplayNone = function(element) {
  return $(element).isDisplayNone();
};

OTHelpers.findElementWithDisplayNone = function(element) {
  if ( (element.offsetWidth === 0 || element.offsetHeight === 0) &&
            $.css(element, 'display') === 'none') return element;

  if (element.parentNode && element.parentNode.style) {
    return $.findElementWithDisplayNone(element.parentNode);
  }

  return null;
};








OTHelpers.Modal = function(options) {

  OTHelpers.eventing(this, true);

  var callback = arguments[arguments.length - 1];

  if(!OTHelpers.isFunction(callback)) {
    throw new Error('OTHelpers.Modal2 must be given a callback');
  }

  if(arguments.length < 2) {
    options = {};
  }

  var domElement = document.createElement('iframe');

  domElement.id = options.id || OTHelpers.uuid();
  domElement.style.position = 'absolute';
  domElement.style.position = 'fixed';
  domElement.style.height = '100%';
  domElement.style.width = '100%';
  domElement.style.top = '0px';
  domElement.style.left = '0px';
  domElement.style.right = '0px';
  domElement.style.bottom = '0px';
  domElement.style.zIndex = 1000;
  domElement.style.border = '0';

  try {
    domElement.style.backgroundColor = 'rgba(0,0,0,0.2)';
  } catch (err) {
    
    
    domElement.style.backgroundColor = 'transparent';
    domElement.setAttribute('allowTransparency', 'true');
  }

  domElement.scrolling = 'no';
  domElement.setAttribute('scrolling', 'no');

  
  
  var frameContent = '<!DOCTYPE html><html><head>' +
                    '<meta http-equiv="x-ua-compatible" content="IE=Edge">' +
                    '<meta http-equiv="Content-type" content="text/html; charset=utf-8">' +
                    '<title></title></head><body></body></html>';

  var wrappedCallback = function() {
    var doc = domElement.contentDocument || domElement.contentWindow.document;

    if (OTHelpers.env.iframeNeedsLoad) {
      doc.body.style.backgroundColor = 'transparent';
      doc.body.style.border = 'none';

      if (OTHelpers.env.name !== 'IE') {
        
        
        doc.open();
        doc.write(frameContent);
        doc.close();
      }
    }

    callback(
      domElement.contentWindow,
      doc
    );
  };

  document.body.appendChild(domElement);

  if(OTHelpers.env.iframeNeedsLoad) {
    if (OTHelpers.env.name === 'IE') {
      
      
      
      domElement.contentWindow.contents = frameContent;
      
      domElement.src = 'javascript:window["contents"]';
      
    }

    OTHelpers.on(domElement, 'load', wrappedCallback);
  } else {
    setTimeout(wrappedCallback, 0);
  }

  this.close = function() {
    OTHelpers.removeElement(domElement);
    this.trigger('closed');
    this.element = domElement = null;
    return this;
  };

  this.element = domElement;

};










(function() {

  


  function getPixelSize(element, style, property, fontSize) {
    var sizeWithSuffix = style[property],
        size = parseFloat(sizeWithSuffix),
        suffix = sizeWithSuffix.split(/\d/)[0],
        rootSize;

    fontSize = fontSize != null ?
      fontSize : /%|em/.test(suffix) && element.parentElement ?
        getPixelSize(element.parentElement, element.parentElement.currentStyle, 'fontSize', null) :
        16;
    rootSize = property === 'fontSize' ?
      fontSize : /width/i.test(property) ? element.clientWidth : element.clientHeight;

    return (suffix === 'em') ?
      size * fontSize : (suffix === 'in') ?
        size * 96 : (suffix === 'pt') ?
          size * 96 / 72 : (suffix === '%') ?
            size / 100 * rootSize : size;
  }

  function setShortStyleProperty(style, property) {
    var
    borderSuffix = property === 'border' ? 'Width' : '',
    t = property + 'Top' + borderSuffix,
    r = property + 'Right' + borderSuffix,
    b = property + 'Bottom' + borderSuffix,
    l = property + 'Left' + borderSuffix;

    style[property] = (style[t] === style[r] === style[b] === style[l] ? [style[t]]
    : style[t] === style[b] && style[l] === style[r] ? [style[t], style[r]]
    : style[l] === style[r] ? [style[t], style[r], style[b]]
    : [style[t], style[r], style[b], style[l]]).join(' ');
  }

  function CSSStyleDeclaration(element) {
    var currentStyle = element.currentStyle,
        style = this,
        fontSize = getPixelSize(element, currentStyle, 'fontSize', null),
        property;

    for (property in currentStyle) {
      if (/width|height|margin.|padding.|border.+W/.test(property) && style[property] !== 'auto') {
        style[property] = getPixelSize(element, currentStyle, property, fontSize) + 'px';
      } else if (property === 'styleFloat') {
        
        style['float'] = currentStyle[property];
      } else {
        style[property] = currentStyle[property];
      }
    }

    setShortStyleProperty(style, 'margin');
    setShortStyleProperty(style, 'padding');
    setShortStyleProperty(style, 'border');

    style.fontSize = fontSize + 'px';

    return style;
  }

  CSSStyleDeclaration.prototype = {
    constructor: CSSStyleDeclaration,
    getPropertyPriority: function () {},
    getPropertyValue: function ( prop ) {
      return this[prop] || '';
    },
    item: function () {},
    removeProperty: function () {},
    setProperty: function () {},
    getPropertyCSSValue: function () {}
  };

  function getComputedStyle(element) {
    return new CSSStyleDeclaration(element);
  }


  OTHelpers.getComputedStyle = function(element) {
    if(element &&
        element.ownerDocument &&
        element.ownerDocument.defaultView &&
        element.ownerDocument.defaultView.getComputedStyle) {
      return element.ownerDocument.defaultView.getComputedStyle(element);
    } else {
      return getComputedStyle(element);
    }
  };

})();







var observeStyleChanges = function observeStyleChanges (element, stylesToObserve, onChange) {
  var oldStyles = {};

  var getStyle = function getStyle(style) {
    switch (style) {
    case 'width':
      return $(element).width();

    case 'height':
      return $(element).height();

    default:
      return $(element).css(style);
    }
  };

  
  $.forEach(stylesToObserve, function(style) {
    oldStyles[style] = getStyle(style);
  });

  var observer = new MutationObserver(function(mutations) {
    var changeSet = {};

    $.forEach(mutations, function(mutation) {
      if (mutation.attributeName !== 'style') return;

      var isHidden = $.isDisplayNone(element);

      $.forEach(stylesToObserve, function(style) {
        if(isHidden && (style === 'width' || style === 'height')) return;

        var newValue = getStyle(style);

        if (newValue !== oldStyles[style]) {
          changeSet[style] = [oldStyles[style], newValue];
          oldStyles[style] = newValue;
        }
      });
    });

    if (!$.isEmpty(changeSet)) {
      
      $.callAsync(function() {
        onChange.call(null, changeSet);
      });
    }
  });

  observer.observe(element, {
    attributes:true,
    attributeFilter: ['style'],
    childList:false,
    characterData:false,
    subtree:false
  });

  return observer;
};

var observeNodeOrChildNodeRemoval = function observeNodeOrChildNodeRemoval (element, onChange) {
  var observer = new MutationObserver(function(mutations) {
    var removedNodes = [];

    $.forEach(mutations, function(mutation) {
      if (mutation.removedNodes.length) {
        removedNodes = removedNodes.concat(prototypeSlice.call(mutation.removedNodes));
      }
    });

    if (removedNodes.length) {
      
      $.callAsync(function() {
        onChange($(removedNodes));
      });
    }
  });

  observer.observe(element, {
    attributes:false,
    childList:true,
    characterData:false,
    subtree:true
  });

  return observer;
};

var observeSize = function (element, onChange) {
  var previousSize = {
    width: 0,
    height: 0
  };

  var interval = setInterval(function() {
    var rect = element.getBoundingClientRect();
    if (previousSize.width !== rect.width || previousSize.height !== rect.height) {
      onChange(rect, previousSize);
      previousSize = {
        width: rect.width,
        height: rect.height
      };
    }
  }, 1000 / 5);

  return {
    disconnect: function() {
      clearInterval(interval);
    }
  };
};

























ElementCollection.prototype.observeStyleChanges = function(stylesToObserve, onChange) {
  var observers = [];

  this.forEach(function(element) {
    observers.push(
      observeStyleChanges(element, stylesToObserve, onChange)
    );
  });

  return observers;
};






















ElementCollection.prototype.observeNodeOrChildNodeRemoval = function(onChange) {
  var observers = [];

  this.forEach(function(element) {
    observers.push(
      observeNodeOrChildNodeRemoval(element, onChange)
    );
  });

  return observers;
};

















ElementCollection.prototype.observeSize = function(onChange) {
  var observers = [];

  this.forEach(function(element) {
    observers.push(
      observeSize(element, onChange)
    );
  });

  return observers;
};



OTHelpers.observeStyleChanges = function(element, stylesToObserve, onChange) {
  return $(element).observeStyleChanges(stylesToObserve, onChange)[0];
};


OTHelpers.observeNodeOrChildNodeRemoval = function(element, onChange) {
  return $(element).observeNodeOrChildNodeRemoval(onChange)[0];
};









(function() {

  var displayStateCache = {},
      defaultDisplays = {};

  var defaultDisplayValueForElement = function (element) {
    if (defaultDisplays[element.ownerDocument] &&
      defaultDisplays[element.ownerDocument][element.nodeName]) {
      return defaultDisplays[element.ownerDocument][element.nodeName];
    }

    if (!defaultDisplays[element.ownerDocument]) defaultDisplays[element.ownerDocument] = {};

    
    
    var testNode = element.ownerDocument.createElement(element.nodeName),
        defaultDisplay;

    element.ownerDocument.body.appendChild(testNode);
    defaultDisplay = defaultDisplays[element.ownerDocument][element.nodeName] =
    $(testNode).css('display');

    $(testNode).remove();
    testNode = null;

    return defaultDisplay;
  };

  var isHidden = function (element) {
    var computedStyle = $.getComputedStyle(element);
    return computedStyle.getPropertyValue('display') === 'none';
  };

  var setCssProperties = function (element, hash) {
    var style = element.style;

    for (var cssName in hash) {
      if (hash.hasOwnProperty(cssName)) {
        style[cssName] = hash[cssName];
      }
    }
  };

  var setCssProperty = function (element, name, value) {
    element.style[name] = value;
  };

  var getCssProperty = function (element, unnormalisedName) {
    
    

    var name = unnormalisedName.replace( /([A-Z]|^ms)/g, '-$1' ).toLowerCase(),
        computedStyle = $.getComputedStyle(element),
        currentValue = computedStyle.getPropertyValue(name);

    if (currentValue === '') {
      currentValue = element.style[name];
    }

    return currentValue;
  };

  var applyCSS = function(element, styles, callback) {
    var oldStyles = {},
        name,
        ret;

    
    for (name in styles) {
      if (styles.hasOwnProperty(name)) {
        
        
        
        oldStyles[name] = element.style[name];

        $(element).css(name, styles[name]);
      }
    }

    ret = callback(element);

    
    for (name in styles) {
      if (styles.hasOwnProperty(name)) {
        $(element).css(name, oldStyles[name] || '');
      }
    }

    return ret;
  };

  ElementCollection.prototype.show = function() {
    return this.forEach(function(element) {
      var display = element.style.display;

      if (display === '' || display === 'none') {
        element.style.display = displayStateCache[element] || '';
        delete displayStateCache[element];
      }

      if (isHidden(element)) {
        
        
        displayStateCache[element] = 'none';

        element.style.display = defaultDisplayValueForElement(element);
      }
    });
  };

  ElementCollection.prototype.hide = function() {
    return this.forEach(function(element) {
      if (element.style.display === 'none') return;

      displayStateCache[element] = element.style.display;
      element.style.display = 'none';
    });
  };

  ElementCollection.prototype.css = function(nameOrHash, value) {
    if (this.length === 0) return;

    if (typeof(nameOrHash) !== 'string') {

      return this.forEach(function(element) {
        setCssProperties(element, nameOrHash);
      });

    } else if (value !== undefined) {

      return this.forEach(function(element) {
        setCssProperty(element, nameOrHash, value);
      });

    } else {
      return getCssProperty(this.first, nameOrHash, value);
    }
  };

  
  
  ElementCollection.prototype.applyCSS = function (styles, callback) {
    var results = [];

    this.forEach(function(element) {
      results.push(applyCSS(element, styles, callback));
    });

    return results;
  };


  
  ElementCollection.prototype.makeVisibleAndYield = function (callback) {
    var hiddenVisually = {
        display: 'block',
        visibility: 'hidden'
      },
      results = [];

    this.forEach(function(element) {
      
      
      var targetElement = $.findElementWithDisplayNone(element);
      if (!targetElement) {
        results.push(void 0);
      }
      else {
        results.push(
          applyCSS(targetElement, hiddenVisually, callback)
        );
      }
    });

    return results;
  };


  
  OTHelpers.show = function(element) {
    return $(element).show();
  };

  
  OTHelpers.hide = function(element) {
    return $(element).hide();
  };

  
  OTHelpers.css = function(element, nameOrHash, value) {
    return $(element).css(nameOrHash, value);
  };

  
  OTHelpers.applyCSS = function(element, styles, callback) {
    return $(element).applyCSS(styles, callback);
  };

  
  OTHelpers.makeVisibleAndYield = function(element, callback) {
    return $(element).makeVisibleAndYield(callback);
  };

})();





OTHelpers.castToBoolean = function(value, defaultValue) {
  if (value === undefined) return defaultValue;
  return value === 'true' || value === true;
};

OTHelpers.roundFloat = function(value, places) {
  return Number(value.toFixed(places));
};





(function() {

  var requestAnimationFrame = window.requestAnimationFrame ||
                              window.mozRequestAnimationFrame ||
                              window.webkitRequestAnimationFrame ||
                              window.msRequestAnimationFrame;

  if (requestAnimationFrame) {
    requestAnimationFrame = OTHelpers.bind(requestAnimationFrame, window);
  }
  else {
    var lastTime = 0;
    var startTime = OTHelpers.now();

    requestAnimationFrame = function(callback){
      var currTime = OTHelpers.now();
      var timeToCall = Math.max(0, 16 - (currTime - lastTime));
      var id = window.setTimeout(function() { callback(currTime - startTime); }, timeToCall);
      lastTime = currTime + timeToCall;
      return id;
    };
  }

  OTHelpers.requestAnimationFrame = requestAnimationFrame;
})();




(function() {

  var capabilities = {};

  
  
  
  
  
  
  
  
  OTHelpers.registerCapability = function(name, callback) {
    var _name = name.toLowerCase();

    if (capabilities.hasOwnProperty(_name)) {
      OTHelpers.error('Attempted to register', name, 'capability more than once');
      return;
    }

    if (!OTHelpers.isFunction(callback)) {
      OTHelpers.error('Attempted to register', name,
                              'capability with a callback that isn\' a function');
      return;
    }

    memoriseCapabilityTest(_name, callback);
  };


  
  
  var memoriseCapabilityTest = function (name, callback) {
    capabilities[name] = function() {
      var result = callback();
      capabilities[name] = function() {
        return result;
      };

      return result;
    };
  };

  var testCapability = function (name) {
    return capabilities[name]();
  };


  
  
  
  
  
  OTHelpers.hasCapabilities = function() {
    var capNames = prototypeSlice.call(arguments),
        name;

    for (var i=0; i<capNames.length; ++i) {
      name = capNames[i].toLowerCase();

      if (!capabilities.hasOwnProperty(name)) {
        OTHelpers.error('hasCapabilities was called with an unknown capability: ' + name);
        return false;
      }
      else if (testCapability(name) === false) {
        return false;
      }
    }

    return true;
  };

})();







OTHelpers.registerCapability('websockets', function() {
  return 'WebSocket' in window && window.WebSocket !== void 0;
});







OTHelpers.registerCapability('classList', function() {
  return (typeof document !== 'undefined') && ('classList' in document.createElement('a'));
});


function hasClass (element, className) {
  if (!className) return false;

  if ($.hasCapabilities('classList')) {
    return element.classList.contains(className);
  }

  return element.className.indexOf(className) > -1;
}

function toggleClasses (element, classNames) {
  if (!classNames || classNames.length === 0) return;

  
  if (element.nodeType !== 1) {
    return;
  }

  var numClasses = classNames.length,
      i = 0;

  if ($.hasCapabilities('classList')) {
    for (; i<numClasses; ++i) {
      element.classList.toggle(classNames[i]);
    }

    return;
  }

  var className = (' ' + element.className + ' ').replace(/[\s+]/, ' ');


  for (; i<numClasses; ++i) {
    if (hasClass(element, classNames[i])) {
      className = className.replace(' ' + classNames[i] + ' ', ' ');
    }
    else {
      className += classNames[i] + ' ';
    }
  }

  element.className = $.trim(className);
}

function addClass (element, classNames) {
  if (!classNames || classNames.length === 0) return;

  
  if (element.nodeType !== 1) {
    return;
  }

  var numClasses = classNames.length,
      i = 0;

  if ($.hasCapabilities('classList')) {
    for (; i<numClasses; ++i) {
      element.classList.add(classNames[i]);
    }

    return;
  }

  

  if (!element.className && classNames.length === 1) {
    element.className = classNames.join(' ');
  }
  else {
    var setClass = ' ' + element.className + ' ';

    for (; i<numClasses; ++i) {
      if ( !~setClass.indexOf( ' ' + classNames[i] + ' ')) {
        setClass += classNames[i] + ' ';
      }
    }

    element.className = $.trim(setClass);
  }
}

function removeClass (element, classNames) {
  if (!classNames || classNames.length === 0) return;

  
  if (element.nodeType !== 1) {
    return;
  }

  var numClasses = classNames.length,
      i = 0;

  if ($.hasCapabilities('classList')) {
    for (; i<numClasses; ++i) {
      element.classList.remove(classNames[i]);
    }

    return;
  }

  var className = (' ' + element.className + ' ').replace(/[\s+]/, ' ');

  for (; i<numClasses; ++i) {
    className = className.replace(' ' + classNames[i] + ' ', ' ');
  }

  element.className = $.trim(className);
}

ElementCollection.prototype.addClass = function (value) {
  if (value) {
    var classNames = $.trim(value).split(/\s+/);

    this.forEach(function(element) {
      addClass(element, classNames);
    });
  }

  return this;
};

ElementCollection.prototype.removeClass = function (value) {
  if (value) {
    var classNames = $.trim(value).split(/\s+/);

    this.forEach(function(element) {
      removeClass(element, classNames);
    });
  }

  return this;
};

ElementCollection.prototype.toggleClass = function (value) {
  if (value) {
    var classNames = $.trim(value).split(/\s+/);

    this.forEach(function(element) {
      toggleClasses(element, classNames);
    });
  }

  return this;
};

ElementCollection.prototype.hasClass = function (value) {
  return this.some(function(element) {
    return hasClass(element, value);
  });
};



OTHelpers.addClass = function(element, className) {
  return $(element).addClass(className);
};


OTHelpers.removeClass = function(element, value) {
  return $(element).removeClass(value);
};










ElementCollection.prototype.attr = function (name, value) {
  if (OTHelpers.isObject(name)) {
    for (var key in name) {
      this.first.setAttribute(key, name[key]);
    }
  }
  else if (value === void 0) {
    return this.first.getAttribute(name);
  }
  else {
    this.first.setAttribute(name, value);
  }

  return this;
};





ElementCollection.prototype.html = function (html) {
  if (html !== void 0) {
    this.first.innerHTML = html;
  }

  return this.first.innerHTML;
};




ElementCollection.prototype.center = function (width, height) {
  var $element;

  this.forEach(function(element) {
    $element = $(element);
    if (!width) width = parseInt($element.width(), 10);
    if (!height) height = parseInt($element.height(), 10);

    var marginLeft = -0.5 * width + 'px';
    var marginTop = -0.5 * height + 'px';

    $element.css('margin', marginTop + ' 0 0 ' + marginLeft)
            .addClass('OT_centered');
  });

  return this;
};





OTHelpers.centerElement = function(element, width, height) {
  return $(element).center(width, height);
};

  


(function() {

  var _width = function(element) {
        if (element.offsetWidth > 0) {
          return element.offsetWidth + 'px';
        }

        return $(element).css('width');
      },

      _height = function(element) {
        if (element.offsetHeight > 0) {
          return element.offsetHeight + 'px';
        }

        return $(element).css('height');
      };

  ElementCollection.prototype.width = function (newWidth) {
    if (newWidth) {
      this.css('width', newWidth);
      return this;
    }
    else {
      if (this.isDisplayNone()) {
        return this.makeVisibleAndYield(function(element) {
          return _width(element);
        })[0];
      }
      else {
        return _width(this.get(0));
      }
    }
  };

  ElementCollection.prototype.height = function (newHeight) {
    if (newHeight) {
      this.css('height', newHeight);
      return this;
    }
    else {
      if (this.isDisplayNone()) {
        
        return this.makeVisibleAndYield(function(element) {
          return _height(element);
        })[0];
      }
      else {
        return _height(this.get(0));
      }
    }
  };

  
  OTHelpers.width = function(element, newWidth) {
    var ret = $(element).width(newWidth);
    return newWidth ? OTHelpers : ret;
  };

  
  OTHelpers.height = function(element, newHeight) {
    var ret = $(element).height(newHeight);
    return newHeight ? OTHelpers : ret;
  };

})();



























(function() {

  OTHelpers.setImmediate = (function() {
    if (typeof process === 'undefined' || !(process.nextTick)) {
      if (typeof setImmediate === 'function') {
        return function (fn) {
          
          setImmediate(fn);
        };
      }
      return function (fn) {
        setTimeout(fn, 0);
      };
    }
    if (typeof setImmediate !== 'undefined') {
      return setImmediate;
    }
    return process.nextTick;
  })();

  OTHelpers.iterator = function(tasks) {
    var makeCallback = function (index) {
      var fn = function () {
        if (tasks.length) {
          tasks[index].apply(null, arguments);
        }
        return fn.next();
      };
      fn.next = function () {
        return (index < tasks.length - 1) ? makeCallback(index + 1) : null;
      };
      return fn;
    };
    return makeCallback(0);
  };

  OTHelpers.waterfall = function(array, done) {
    done = done || function () {};
    if (array.constructor !== Array) {
      return done(new Error('First argument to waterfall must be an array of functions'));
    }

    if (!array.length) {
      return done();
    }

    var next = function(iterator) {
      return function (err) {
        if (err) {
          done.apply(null, arguments);
          done = function () {};
        } else {
          var args = prototypeSlice.call(arguments, 1),
              nextFn = iterator.next();
          if (nextFn) {
            args.push(next(nextFn));
          } else {
            args.push(done);
          }
          OTHelpers.setImmediate(function() {
            iterator.apply(null, args);
          });
        }
      };
    };

    next(OTHelpers.iterator(array))();
  };

})();





(function() {

  
  var logQueue = [],
      queueRunning = false;

  OTHelpers.Analytics = function(loggingUrl, debugFn) {

    var endPoint = loggingUrl + '/logging/ClientEvent',
        endPointQos = loggingUrl + '/logging/ClientQos',

        reportedErrors = {},

        send = function(data, isQos, callback) {
          OTHelpers.post((isQos ? endPointQos : endPoint) + '?_=' + OTHelpers.uuid.v4(), {
            body: data,
            xdomainrequest: ($.env.name === 'IE' && $.env.version < 10),
            headers: {
              'Content-Type': 'application/json'
            }
          }, callback);
        },

        throttledPost = function() {
          
          if (!queueRunning && logQueue.length > 0) {
            queueRunning = true;
            var curr = logQueue[0];

            
            var processNextItem = function() {
              logQueue.shift();
              queueRunning = false;
              throttledPost();
            };

            if (curr) {
              send(curr.data, curr.isQos, function(err) {
                if (err) {
                  var debugMsg = 'Failed to send ClientEvent, moving on to the next item.';
                  if (debugFn) {
                    debugFn(debugMsg);
                  } else {
                    console.log(debugMsg);
                  }
                  
                } else {
                  curr.onComplete();
                }
                setTimeout(processNextItem, 50);
              });
            }
          }
        },

        post = function(data, onComplete, isQos) {
          logQueue.push({
            data: data,
            onComplete: onComplete,
            isQos: isQos
          });

          throttledPost();
        },

        shouldThrottleError = function(code, type, partnerId) {
          if (!partnerId) return false;

          var errKey = [partnerId, type, code].join('_'),
          
            msgLimit = 100;
          if (msgLimit === null || msgLimit === undefined) return false;
          return (reportedErrors[errKey] || 0) <= msgLimit;
        };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    this.logError = function(code, type, message, details, options) {
      if (!options) options = {};
      var partnerId = options.partnerId;

      if (shouldThrottleError(code, type, partnerId)) {
        
        
        return;
      }

      var errKey = [partnerId, type, code].join('_'),
      payload =  details ? details : null;

      reportedErrors[errKey] = typeof(reportedErrors[errKey]) !== 'undefined' ?
        reportedErrors[errKey] + 1 : 1;
      this.logEvent(OTHelpers.extend(options, {
        action: type + '.' + code,
        payload: payload
      }), false);
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    this.logEvent = function(data, qos, throttle) {
      if (!qos) qos = false;

      if (throttle && !isNaN(throttle)) {
        if (Math.random() > throttle) {
          return;
        }
      }

      
      for (var key in data) {
        if (data.hasOwnProperty(key) && data[key] === null) {
          delete data[key];
        }
      }

      
      data = JSON.stringify(data);

      var onComplete = function() {
        
        
      };

      post(data, onComplete, qos);
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    this.logQOS = function(options) {
      this.logEvent(options, true);
    };
  };

})();









OTHelpers.get = function(url, options, callback) {
  var _options = OTHelpers.extend(options || {}, {
    method: 'GET'
  });
  OTHelpers.request(url, _options, callback);
};


OTHelpers.post = function(url, options, callback) {
  var _options = OTHelpers.extend(options || {}, {
    method: 'POST'
  });

  if(_options.xdomainrequest) {
    OTHelpers.xdomainRequest(url, _options, callback);
  } else {
    OTHelpers.request(url, _options, callback);
  }
};


})(window, window.OTHelpers);


















(function(scope) {



if (scope.OTPlugin !== void 0) return;







var isSupported = (OTHelpers.env.name === 'IE' && OTHelpers.env.version >= 8 &&
                    OTHelpers.env.userAgent.indexOf('x64') === -1),
    pluginIsReady = false;


var OTPlugin = {
  isSupported: function () { return isSupported; },
  isReady: function() { return pluginIsReady; },
  meta: {
    mimeType: 'application/x-opentokie,version=0.4.0.9',
    activeXName: 'TokBox.OpenTokIE.0.4.0.9',
    version: '0.4.0.9'
  }
};




OTHelpers.useLogHelpers(OTPlugin);

scope.OTPlugin = OTPlugin;



if (!OTPlugin.isSupported()) {
  OTPlugin.isInstalled = function isInstalled () { return false; };
  return;
}







var shim = function shim () {
  if (!Function.prototype.bind) {
    Function.prototype.bind = function (oThis) {
      if (typeof this !== 'function') {
        
        throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
      }

      var aArgs = Array.prototype.slice.call(arguments, 1),
          fToBind = this,
          FNOP = function () {},
          fBound = function () {
            return fToBind.apply(this instanceof FNOP && oThis ?
                          this : oThis, aArgs.concat(Array.prototype.slice.call(arguments)));
          };

      FNOP.prototype = this.prototype;
      fBound.prototype = new FNOP();

      return fBound;
    };
  }

  if(!Array.isArray) {
    Array.isArray = function (vArg) {
      return Object.prototype.toString.call(vArg) === '[object Array]';
    };
  }

  if (!Array.prototype.indexOf) {
    Array.prototype.indexOf = function (searchElement, fromIndex) {
      var i,
          pivot = (fromIndex) ? fromIndex : 0,
          length;

      if (!this) {
        throw new TypeError();
      }

      length = this.length;

      if (length === 0 || pivot >= length) {
        return -1;
      }

      if (pivot < 0) {
        pivot = length - Math.abs(pivot);
      }

      for (i = pivot; i < length; i++) {
        if (this[i] === searchElement) {
          return i;
        }
      }
      return -1;
    };
  }

  if (!Array.prototype.map)
  {
    Array.prototype.map = function(fun )
    {
      'use strict';

      if (this === void 0 || this === null)
        throw new TypeError();

      var t = Object(this);
      var len = t.length >>> 0;
      if (typeof fun !== 'function') {
        throw new TypeError();
      }

      var res = new Array(len);
      var thisArg = arguments.length >= 2 ? arguments[1] : void 0;
      for (var i = 0; i < len; i++)
      {
        
        
        
        
        
        if (i in t)
          res[i] = fun.call(thisArg, t[i], i, t);
      }

      return res;
    };
  }
};





var RumorSocket = function(plugin, server) {
  var connected = false,
      rumorID;

  var _onOpen,
      _onClose;


  try {
    rumorID = plugin._.RumorInit(server, '');
  }
  catch(e) {
    OTPlugin.error('Error creating the Rumor Socket: ', e.message);
  }

  if(!rumorID) {
    throw new Error('Could not initialise OTPlugin rumor connection');
  }

  plugin._.SetOnRumorOpen(rumorID, function() {
    if (_onOpen && OTHelpers.isFunction(_onOpen)) {
      _onOpen.call(null);
    }
  });

  plugin._.SetOnRumorClose(rumorID, function(code) {
    _onClose(code);

    
    plugin.removeRef(this);
  });

  var api = {
    open: function() {
      connected = true;
      plugin._.RumorOpen(rumorID);
    },

    close: function(code, reason) {
      if (connected) {
        connected = false;
        plugin._.RumorClose(rumorID, code, reason);
      }
    },

    destroy: function() {
      this.close();
    },

    send: function(msg) {
      plugin._.RumorSend(rumorID, msg.type, msg.toAddress,
        JSON.parse(JSON.stringify(msg.headers)), msg.data);
    },

    onOpen: function(callback) {
      _onOpen = callback;
    },

    onClose: function(callback) {
      _onClose = callback;
    },

    onError: function(callback) {
      plugin._.SetOnRumorError(rumorID, callback);
    },

    onMessage: function(callback) {
      plugin._.SetOnRumorMessage(rumorID, callback);
    }
  };

  plugin.addRef(api);
  return api;
};











var objectTimeouts = {},
    mediaCaptureObject,
    plugins = {};

var curryCallAsync = function curryCallAsync (fn) {
  return function() {
    var args = Array.prototype.slice.call(arguments);
    args.unshift(fn);
    OTHelpers.callAsync.apply(OTHelpers, args);
  };
};


var clearObjectLoadTimeout = function clearObjectLoadTimeout (callbackId) {
  if (!callbackId) return;

  if (objectTimeouts[callbackId]) {
    clearTimeout(objectTimeouts[callbackId]);
    delete objectTimeouts[callbackId];
  }

  if (scope[callbackId]) {
    try {
      delete scope[callbackId];
    } catch (err) {
      scope[callbackId] = void 0;
    }
  }
};

var removeObjectFromDom = function removeObjectFromDom (object) {
  clearObjectLoadTimeout(object.getAttribute('tbCallbackId'));

  if (mediaCaptureObject && mediaCaptureObject.id === object.id) {
    mediaCaptureObject = null;
  }
  else if (plugins.hasOwnProperty(object.id)) {
    delete plugins[object.id];
  }

  OTHelpers.removeElement(object);
};



var removeAllObjects = function removeAllObjects () {
  if (mediaCaptureObject) mediaCaptureObject.destroy();

  for (var id in plugins) {
    if (plugins.hasOwnProperty(id)) {
      plugins[id].destroy();
    }
  }
};


var PluginProxy = function PluginProxy (plugin) {
  var _plugin = plugin,
      _liveObjects = [];

  this._ = _plugin;

  this.addRef = function(ref) {
    _liveObjects.push(ref);
    return this;
  };

  this.removeRef = function(ref) {
    if (_liveObjects.length === 0) return;

    var index = _liveObjects.indexOf(ref);
    if (index !== -1) {
      _liveObjects.splice(index, 1);
    }

    if (_liveObjects.length === 0) {
      this.destroy();
    }

    return this;
  };

  this.isValid = function() {
    return _plugin.valid;
  };

  

  var eventHandlers = {};

  var onCustomEvent = OTHelpers.bind(curryCallAsync(function onCustomEvent() {
    var args = Array.prototype.slice.call(arguments),
        name = args.shift();

    if (!eventHandlers.hasOwnProperty(name) && eventHandlers[name].length) {
      return;
    }

    OTHelpers.forEach(eventHandlers[name], function(handler) {
      handler[0].apply(handler[1], args);
    });
  }), this);


  this.on = function (name, callback, context) {
    if (!eventHandlers.hasOwnProperty(name)) {
      eventHandlers[name] = [];
    }

    eventHandlers[name].push([callback, context]);
    return this;
  };

  this.off = function (name, callback, context) {
    if (!eventHandlers.hasOwnProperty(name) ||
        eventHandlers[name].length === 0) {
      return;
    }

    OTHelpers.filter(eventHandlers[name], function(listener) {
      return listener[0] === callback &&
              listener[1] === context;
    });

    return this;
  };

  this.once = function (name, callback, context) {
    var fn = function () {
      this.off(name, fn, this);
      return callback.apply(context, arguments);
    };

    this.on(name, fn, this);
    return this;
  };


  this.onReady = function(readyCallback) {
    if (_plugin.on) {
      
      _plugin.on(-1, {customEvent: curryCallAsync(onCustomEvent, this)});
    }

    
    if (_plugin.initialise) {
      this.on('ready', OTHelpers.bind(curryCallAsync(readyCallback), this));
      _plugin.initialise();
    }
    else {
      readyCallback.call(null);
    }
  };

  this.destroy = function() {
    while (_liveObjects.length) {
      _liveObjects.shift().destroy();
    }

    if (_plugin) removeObjectFromDom(_plugin);
    _plugin = null;
  };

  this.setStream = function(stream, completion) {
    if (completion) {
      if (stream.hasVideo()) {
        
        
        var verifyStream = function() {
          if (!_plugin) return;

          if (_plugin.videoWidth > 0) {
            
            setTimeout(completion, 200);
          }
          else {
            setTimeout(verifyStream, 500);
          }
        };

        setTimeout(verifyStream, 500);
      }
      else {
        
        
        completion();
      }
    }
    _plugin.setStream(stream);
  };
};









var VideoContainer = function VideoContainer (plugin, stream) {
  this.domElement = plugin._;
  this.parentElement = plugin._.parentNode;

  plugin.addRef(this);

  this.appendTo = function (parentDomElement) {
    if (parentDomElement && plugin._.parentNode !== parentDomElement) {
      OTPlugin.debug('VideoContainer appendTo', parentDomElement);
      parentDomElement.appendChild(plugin._);
      this.parentElement = parentDomElement;
    }
  };

  this.show = function (completion) {
    OTPlugin.debug('VideoContainer show');
    plugin._.removeAttribute('width');
    plugin._.removeAttribute('height');
    plugin.setStream(stream, completion);
    OTHelpers.show(plugin._);
  };

  this.setWidth = function (width) {
    OTPlugin.debug('VideoContainer setWidth to ' + width);
    plugin._.setAttribute('width', width);
  };

  this.setHeight = function (height) {
    OTPlugin.debug('VideoContainer setHeight to ' + height);
    plugin._.setAttribute('height', height);
  };

  this.setVolume = function (value) {
    
    OTPlugin.debug('VideoContainer setVolume not implemented: called with ' + value);
  };

  this.getVolume = function () {
    
    OTPlugin.debug('VideoContainer getVolume not implemented');
    return 0.5;
  };

  this.getImgData = function () {
    return plugin._.getImgData('image/png');
  };

  this.getVideoWidth = function () {
    return plugin._.videoWidth;
  };

  this.getVideoHeight = function () {
    return plugin._.videoHeight;
  };

  this.destroy = function () {
    plugin._.setStream(null);
    plugin.removeRef(this);
  };
};









var RTCStatsReport = function (reports) {
  this.forEach = function (callback, context) {
    for (var id in reports) {
      callback.call(context, reports[id]);
    }
  };
};

  












var PeerConnection = function PeerConnection (iceServers, options, plugin, ready) {
  var id = OTHelpers.uuid(),
      hasLocalDescription = false,
      hasRemoteDescription = false,
      candidates = [],
      inited = false,
      deferMethods = [],
      events,
      _this = this;

  plugin.addRef(this);

  events = {
    addstream: [],
    removestream: [],
    icecandidate: [],
    signalingstatechange: [],
    iceconnectionstatechange: []
  };

  var onAddIceCandidate = function onAddIceCandidate () {},

      onAddIceCandidateFailed = function onAddIceCandidateFailed (err) {
        OTPlugin.error('Failed to process candidate');
        OTPlugin.error(err);
      },

      processPendingCandidates = function processPendingCandidates () {
        for (var i=0; i<candidates.length; ++i) {
          plugin._.addIceCandidate(id, candidates[i], onAddIceCandidate, onAddIceCandidateFailed);
        }
      },


      deferMethod = function (method) {
        return function() {
          if (inited === true) {
            return method.apply(_this, arguments);
          }

          deferMethods.push([method, arguments]);
        };
      },

      processDeferredMethods = function () {
        var m;
        while ( (m = deferMethods.shift()) ) {
          m[0].apply(_this, m[1]);
        }
      },

      triggerEvent = function () {
        var args = Array.prototype.slice.call(arguments),
            eventName = args.shift();

        if (!events.hasOwnProperty(eventName)) {
          OTPlugin.error('PeerConnection does not have an event called: ' + eventName);
          return;
        }

        OTHelpers.forEach(events[eventName], function(listener) {
          listener.apply(null, args);
        });
      },

      bindAndDelegateEvents = function () {
        plugin._.on(id, {
          addStream: function(streamJson) {
            setTimeout(function() {
              var stream = MediaStream.fromJson(streamJson, plugin),
                  event = {stream: stream, target: _this};

              if (_this.onaddstream && OTHelpers.isFunction(_this.onaddstream)) {
                OTHelpers.callAsync(_this.onaddstream, event);
              }

              triggerEvent('addstream', event);
            }, 3000);
          },

          removeStream: function(streamJson) {
            var stream = MediaStream.fromJson(streamJson, plugin),
                event = {stream: stream, target: _this};

            if (_this.onremovestream && OTHelpers.isFunction(_this.onremovestream)) {
              OTHelpers.callAsync(_this.onremovestream, event);
            }

            triggerEvent('removestream', event);
          },

          iceCandidate: function(candidateSdp, sdpMid, sdpMLineIndex) {
            var candidate = new OTPlugin.RTCIceCandidate({
              candidate: candidateSdp,
              sdpMid: sdpMid,
              sdpMLineIndex: sdpMLineIndex
            });

            var event = {candidate: candidate, target: _this};

            if (_this.onicecandidate && OTHelpers.isFunction(_this.onicecandidate)) {
              OTHelpers.callAsync(_this.onicecandidate, event);
            }

            triggerEvent('icecandidate', event);
          },

          signalingStateChange: function(state) {
            _this.signalingState = state;
            var event = {state: state, target: _this};

            if (_this.onsignalingstatechange &&
                    OTHelpers.isFunction(_this.onsignalingstatechange)) {
              OTHelpers.callAsync(_this.onsignalingstatechange, event);
            }

            triggerEvent('signalingstate', event);
          },

          iceConnectionChange: function(state) {
            _this.iceConnectionState = state;
            var event = {state: state, target: _this};

            if (_this.oniceconnectionstatechange &&
                    OTHelpers.isFunction(_this.oniceconnectionstatechange)) {
              OTHelpers.callAsync(_this.oniceconnectionstatechange, event);
            }

            triggerEvent('iceconnectionstatechange', event);
          }
        });
      };

  this.createOffer = deferMethod(function (success, error, constraints) {
    OTPlugin.debug('createOffer', constraints);
    plugin._.createOffer(id, function(type, sdp) {
      success(new OTPlugin.RTCSessionDescription({
        type: type,
        sdp: sdp
      }));
    }, error, constraints || {});
  });

  this.createAnswer = deferMethod(function (success, error, constraints) {
    OTPlugin.debug('createAnswer', constraints);
    plugin._.createAnswer(id, function(type, sdp) {
      success(new OTPlugin.RTCSessionDescription({
        type: type,
        sdp: sdp
      }));
    }, error, constraints || {});
  });

  this.setLocalDescription = deferMethod( function (description, success, error) {
    OTPlugin.debug('setLocalDescription');

    plugin._.setLocalDescription(id, description, function() {
      hasLocalDescription = true;

      if (hasRemoteDescription) processPendingCandidates();

      if (success) success.call(null);
    }, error);
  });

  this.setRemoteDescription = deferMethod( function (description, success, error) {
    OTPlugin.debug('setRemoteDescription');

    plugin._.setRemoteDescription(id, description, function() {
      hasRemoteDescription = true;

      if (hasLocalDescription) processPendingCandidates();
      if (success) success.call(null);
    }, error);
  });

  this.addIceCandidate = deferMethod( function (candidate) {
    OTPlugin.debug('addIceCandidate');

    if (hasLocalDescription && hasRemoteDescription) {
      plugin._.addIceCandidate(id, candidate, onAddIceCandidate, onAddIceCandidateFailed);
    }
    else {
      candidates.push(candidate);
    }
  });

  this.addStream = deferMethod( function (stream) {
    var constraints = {};
    plugin._.addStream(id, stream, constraints);
  });

  this.removeStream = deferMethod( function (stream) {
    plugin._.removeStream(id, stream);
  });

  this.getRemoteStreams = function () {
    return OTHelpers.map(plugin._.getRemoteStreams(id), function(stream) {
      return MediaStream.fromJson(stream, plugin);
    });
  };

  this.getLocalStreams = function () {
    return OTHelpers.map(plugin._.getLocalStreams(id), function(stream) {
      return MediaStream.fromJson(stream, plugin);
    });
  };

  this.getStreamById = function (streamId) {
    return MediaStream.fromJson(plugin._.getStreamById(id, streamId), plugin);
  };

  this.getStats = deferMethod( function (mediaStreamTrack, success, error) {
    plugin._.getStats(id, mediaStreamTrack || null, function(statsReportJson) {
      var report = new RTCStatsReport(JSON.parse(statsReportJson));
      OTHelpers.callAsync(success, report);
    }, error);
  });

  this.close = function () {
    plugin._.destroyPeerConnection(id);
    plugin.removeRef(this);
  };

  this.destroy = function () {
    this.close();
  };

  this.addEventListener = function  (event, handler ) {
    if (events[event] === void 0) {
      OTPlugin.error('Could not bind invalid event "' + event + '" to PeerConnection. ' +
                      'The valid event types are:');
      OTPlugin.error('\t' + OTHelpers.keys(events).join(', '));
      return;
    }

    events[event].push(handler);
  };

  this.removeEventListener = function  (event, handler ) {
    if (events[event] === void 0) {
      OTPlugin.error('Could not unbind invalid event "' + event + '" to PeerConnection. ' +
                      'The valid event types are:');
      OTPlugin.error('\t' + OTHelpers.keys(events).join(', '));
      return;
    }

    events[event] = OTHelpers.filter(events[event], handler);
  };

  
  
  
  
  this.onaddstream = null;
  this.onremovestream = null;
  this.onicecandidate = null;
  this.onsignalingstatechange = null;
  this.oniceconnectionstatechange = null;

  
  OTHelpers.forEach(iceServers.iceServers, function(iceServer) {
    if (!iceServer.username) iceServer.username = '';
    if (!iceServer.credential) iceServer.credential = '';
  });

  if (!plugin._.initPeerConnection(id, iceServers, options)) {
    OTPlugin.error('Failed to initialise PeerConnection');
    ready(new OTHelpers.error('Failed to initialise PeerConnection'));
    return;
  }

  
  inited = true;
  bindAndDelegateEvents();
  processDeferredMethods();
  ready(void 0, this);
};

PeerConnection.create = function (iceServers, options, plugin, ready) {
  new PeerConnection(iceServers, options, plugin, ready);
};













var MediaStreamTrack = function MediaStreamTrack (mediaStreamId, options, plugin) {
  this.id = options.id;
  this.kind = options.kind;
  this.label = options.label;
  this.enabled = OTHelpers.castToBoolean(options.enabled);
  this.streamId = mediaStreamId;

  this.setEnabled = function (enabled) {
    this.enabled = OTHelpers.castToBoolean(enabled);

    if (this.enabled) {
      plugin._.enableMediaStreamTrack(mediaStreamId, this.id);
    }
    else {
      plugin._.disableMediaStreamTrack(mediaStreamId, this.id);
    }
  };
};

var MediaStream = function MediaStream (options, plugin) {
  var audioTracks = [],
      videoTracks = [];

  this.id = options.id;
  plugin.addRef(this);

  
  
  

  if (options.videoTracks) {
    options.videoTracks.map(function(track) {
      videoTracks.push( new MediaStreamTrack(options.id, track, plugin) );
    });
  }

  if (options.audioTracks) {
    options.audioTracks.map(function(track) {
      audioTracks.push( new MediaStreamTrack(options.id, track, plugin) );
    });
  }

  var hasTracksOfType = function (type) {
    var tracks = type === 'video' ? videoTracks : audioTracks;

    return OTHelpers.some(tracks, function(track) {
      return track.enabled;
    });
  };

  this.getVideoTracks = function () { return videoTracks; };
  this.getAudioTracks = function () { return audioTracks; };

  this.getTrackById = function (id) {
    videoTracks.concat(audioTracks).forEach(function(track) {
      if (track.id === id) return track;
    });

    return null;
  };

  this.hasVideo = function () {
    return hasTracksOfType('video');
  };

  this.hasAudio = function () {
    return hasTracksOfType('audio');
  };

  this.addTrack = function () {
    
  };

  this.removeTrack = function () {
    
  };

  this.stop = function() {
    plugin._.stopMediaStream(this.id);
    plugin.removeRef(this);
  };

  this.destroy = function() {
    this.stop();
  };

  
  this._ = {
    plugin: plugin,

    
    render: OTHelpers.bind(function() {
      return new VideoContainer(plugin, this);
    }, this)
  };
};


MediaStream.fromJson = function (json, plugin) {
  if (!json) return null;
  return new MediaStream( JSON.parse(json), plugin );
};










var MediaConstraints = function(userConstraints) {
  var constraints = OTHelpers.clone(userConstraints);

  this.hasVideo = constraints.video !== void 0 && constraints.video !== false;
  this.hasAudio = constraints.audio !== void 0 && constraints.audio !== false;

  if (constraints.video === true) constraints.video = {};
  if (constraints.audio === true)  constraints.audio = {};

  if (this.hasVideo && !constraints.video.mandatory) {
    constraints.video.mandatory = {};
  }

  if (this.hasAudio && !constraints.audio.mandatory) {
    constraints.audio.mandatory = {};
  }

  this.screenSharing = this.hasVideo &&
                ( constraints.video.mandatory.chromeMediaSource === 'screen' ||
                  constraints.video.mandatory.chromeMediaSource === 'window' );

  this.audio = constraints.audio;
  this.video = constraints.video;

  this.setVideoSource = function(sourceId) {
    if (sourceId !== void 0) constraints.video.mandatory.sourceId =  sourceId;
    else delete constraints.video;
  };

  this.setAudioSource = function(sourceId) {
    if (sourceId !== void 0) constraints.audio.mandatory.sourceId =  sourceId;
    else delete constraints.audio;
  };

  this.toHash = function() {
    return constraints;
  };
};










var objectTimeouts = {};

var lastElementChild = function lastElementChild(parent) {
  var node = parent.lastChild;

  while(node && node.nodeType !== 1) {
    node = node.previousSibling;
  }

  return node;
};

var generateCallbackUUID = function generateCallbackUUID () {
  return 'OTPlugin_loaded_' + OTHelpers.uuid().replace(/\-+/g, '');
};


var clearObjectLoadTimeout = function clearObjectLoadTimeout (callbackId) {
  if (!callbackId) return;

  if (objectTimeouts[callbackId]) {
    clearTimeout(objectTimeouts[callbackId]);
    delete objectTimeouts[callbackId];
  }

  if (scope[callbackId]) {
    try {
      delete scope[callbackId];
    } catch (err) {
      scope[callbackId] = void 0;
    }
  }
};

var createPluginProxy = function createPluginProxy (callbackId, mimeType, params, isVisible) {
  var objBits = [],
      extraAttributes = ['width="0" height="0"'],
      plugin;

  if (isVisible !== true) {
    extraAttributes.push('visibility="hidden"');
  }

  objBits.push('<object type="' + mimeType + '" ' + extraAttributes.join(' ') + '>');

  for (var name in params) {
    if (params.hasOwnProperty(name)) {
      objBits.push('<param name="' + name + '" value="' + params[name] + '" />');
    }
  }

  objBits.push('</object>');

  scope.document.body.insertAdjacentHTML('beforeend', objBits.join(''));
  plugin = new PluginProxy(lastElementChild(scope.document.body));
  plugin._.setAttribute('tbCallbackId', callbackId);

  return plugin;
};



var injectObject = function injectObject (mimeType, isVisible, params, completion) {
  var callbackId = generateCallbackUUID(),
      plugin;

  params.onload = callbackId;
  params.userAgent = OTHelpers.env.userAgent.toLowerCase();

  scope[callbackId] = function() {
    clearObjectLoadTimeout(callbackId);

    plugin._.setAttribute('id', 'tb_plugin_' + plugin._.uuid);

    if (plugin._.removeAttribute !== void 0) {
      plugin._.removeAttribute('tbCallbackId');
    }
    else {
      
      plugin._.tbCallbackId = null;
    }

    plugin.uuid = plugin._.uuid;
    plugin.id = plugin._.id;

    plugin.onReady(function(err) {
      if (err) {
        OTPlugin.error('Error while starting up plugin ' + plugin.uuid + ': ' + err);
        return;
      }

      OTPlugin.debug('Plugin ' + plugin.id + ' is loaded');

      if (completion && OTHelpers.isFunction(completion)) {
        completion.call(OTPlugin, null, plugin);
      }
    });
  };

  plugin = createPluginProxy(callbackId, mimeType, params, isVisible);

  objectTimeouts[callbackId] = setTimeout(function() {
    clearObjectLoadTimeout(callbackId);

    completion.call(OTPlugin, 'The object with the mimeType of ' +
                                mimeType + ' timed out while loading.');

    scope.document.body.removeChild(plugin._);
  }, 3000);

  return plugin;
};














var objectTimeouts = {},
    mediaCaptureObject,
    plugins = {};




var removeAllObjects = function removeAllObjects () {
  if (mediaCaptureObject) mediaCaptureObject.destroy();

  for (var id in plugins) {
    if (plugins.hasOwnProperty(id)) {
      plugins[id].destroy();
    }
  }
};







var createMediaCaptureController = function createMediaCaptureController (completion) {
  if (mediaCaptureObject) {
    throw new Error('OTPlugin.createMediaCaptureController called multiple times!');
  }

  mediaCaptureObject = injectObject(OTPlugin.meta.mimeType, false, {windowless: false}, completion);

  mediaCaptureObject.selectSources = function() {
    return this._.selectSources.apply(this._, arguments);
  };

  return mediaCaptureObject;
};





var createPeerController = function createPeerController (completion) {
  var o = injectObject(OTPlugin.meta.mimeType, true, {windowless: true}, function(err, plugin) {
    if (err) {
      completion.call(OTPlugin, err);
      return;
    }

    plugins[plugin.id] = plugin;
    completion.call(OTPlugin, null, plugin);
  });

  return o;
};











var AutoUpdater;

(function() {

  var autoUpdaterController,
      updaterMimeType,        
      installedVersion = -1;  

  var versionGreaterThan = function versionGreaterThan (version1, version2) {
    if (version1 === version2) return false;
    if (version1 === -1) return version2;
    if (version2 === -1) return version1;

    if (version1.indexOf('.') === -1 && version2.indexOf('.') === -1) {
      return version1 > version2;
    }

    
    
    
    
    var v1 = version1.split('.'),
        v2 = version2.split('.'),
        versionLength = (v1.length > v2.length ? v2 : v1).length;


    for (var i = 0; i < versionLength; ++i) {
      if (parseInt(v1[i], 10) > parseInt(v2[i], 10)) {
        return true;
      }
    }

    
    
    
    if (i < v1.length) {
      return true;
    }

    return false;
  };


  
  
  var findMimeTypeAndVersion = function findMimeTypeAndVersion () {

    if (updaterMimeType !== void 0) {
      return updaterMimeType;
    }

    var activeXControlId = 'TokBox.otiePluginInstaller',
        installPluginName = 'otiePluginInstaller',
        unversionedMimeType = 'application/x-otieplugininstaller',
        plugin = navigator.plugins[activeXControlId] || navigator.plugins[installPluginName];

    installedVersion = -1;

    if (plugin) {
      
      
      
      
      var numMimeTypes = plugin.length,
          extractVersion = new RegExp(unversionedMimeType.replace('-', '\\-') +
                                                      ',version=([0-9a-zA-Z-_.]+)', 'i'),
          mimeType,
          bits;


      for (var i=0; i<numMimeTypes; ++i) {
        mimeType = plugin[i];

        
        
        if (mimeType && mimeType.enabledPlugin &&
            (mimeType.enabledPlugin.name === plugin.name) &&
            mimeType.type.indexOf(unversionedMimeType) !== -1) {

          bits = extractVersion.exec(mimeType.type);

          if (bits !== null && versionGreaterThan(bits[1], installedVersion)) {
            installedVersion = bits[1];
          }
        }
      }
    }
    else if (OTHelpers.env.name === 'IE') {
      
      
      
      
      try {
        plugin = new ActiveXObject(activeXControlId);
        installedVersion = plugin.getMasterVersion();
      } catch(e) {
      }
    }

    updaterMimeType = installedVersion !== -1 ?
                              unversionedMimeType + ',version=' + installedVersion :
                              null;
  };

  var getInstallerMimeType = function getInstallerMimeType () {
    if (updaterMimeType === void 0) {
      findMimeTypeAndVersion();
    }

    return updaterMimeType;
  };

  var getInstalledVersion = function getInstalledVersion () {
    if (installedVersion === void 0) {
      findMimeTypeAndVersion();
    }

    return installedVersion;
  };

  
  
  
  var hasBrokenUpdater = function () {
    var _broken = !versionGreaterThan(getInstalledVersion(), '0.4.0.4');

    hasBrokenUpdater = function() { return _broken; };
    return _broken;
  };


  AutoUpdater = function () {
    var plugin;

    var getControllerCurry = function getControllerFirstCurry (fn) {
      return function() {
        if (plugin) {
          return fn(void 0, arguments);
        }

        injectObject(getInstallerMimeType(), false, {windowless: false}, function(err, p) {
          plugin = p;

          if (err) {
            OTPlugin.error('Error while loading the AutoUpdater: ' + err);
            return;
          }

          return fn.apply(void 0, arguments);
        });
      };
    };

    
    
    this.isOutOfDate = function () {
      return versionGreaterThan(OTPlugin.meta.version, getInstalledVersion());
    };

    this.autoUpdate = getControllerCurry(function () {
      var modal = OT.Dialogs.Plugin.updateInProgress(),
          analytics = new OT.Analytics(),
        payload = {
          ieVersion: OTHelpers.env.version,
          pluginOldVersion: OTPlugin.installedVersion(),
          pluginNewVersion: OTPlugin.version()
        };

      var success = curryCallAsync(function() {
            analytics.logEvent({
              action: 'OTPluginAutoUpdate',
              variation: 'Success',
              partnerId: OT.APIKEY,
              payload: JSON.stringify(payload)
            });

            plugin.destroy();

            modal.close();
            OT.Dialogs.Plugin.updateComplete().on({
              reload: function() {
                window.location.reload();
              }
            });
          }),

          error = curryCallAsync(function(errorCode, errorMessage, systemErrorCode) {
            payload.errorCode = errorCode;
            payload.systemErrorCode = systemErrorCode;

            analytics.logEvent({
              action: 'OTPluginAutoUpdate',
              variation: 'Failure',
              partnerId: OT.APIKEY,
              payload: JSON.stringify(payload)
            });

            plugin.destroy();

            modal.close();
            var updateMessage = errorMessage + ' (' + errorCode +
                                      '). Please restart your browser and try again.';

            modal = OT.Dialogs.Plugin.updateComplete(updateMessage).on({
              'reload': function() {
                modal.close();
              }
            });

            OTPlugin.error('autoUpdate failed: ' + errorMessage + ' (' + errorCode +
                                      '). Please restart your browser and try again.');
            
          }),

          progress = curryCallAsync(function(progress) {
            modal.setUpdateProgress(progress.toFixed());
            
          });

      plugin._.updatePlugin(OTPlugin.pathToInstaller(), success, error, progress);
    });

    this.destroy = function() {
      if (plugin) plugin.destroy();
    };

    
    if (navigator.plugins) {
      navigator.plugins.refresh(false);
    }
  };

  AutoUpdater.get = function (completion) {
    if (!autoUpdaterController) {
      if (!this.isinstalled()) {
        completion.call(null, 'Plugin was not installed');
        return;
      }

      autoUpdaterController = new AutoUpdater();
    }

    completion.call(null, void 0, autoUpdaterController);
  };

  AutoUpdater.isinstalled = function () {
    return getInstallerMimeType() !== null && !hasBrokenUpdater();
  };

  AutoUpdater.installedVersion = function () {
    return getInstalledVersion();
  };

})();


















var readyCallbacks = [];

var 
    destroy = function destroy () {
      removeAllObjects();
    },

    registerReadyListener = function registerReadyListener (callback) {
      readyCallbacks.push(callback);
    },

    notifyReadyListeners = function notifyReadyListeners (err) {
      var callback;

      while ( (callback = readyCallbacks.pop()) && OTHelpers.isFunction(callback) ) {
        callback.call(OTPlugin, err);
      }
    },

    onDomReady = function onDomReady () {
      AutoUpdater.get(function(err, updater) {
        if (err) {
          OTPlugin.error('Error while loading the AutoUpdater: ' + err);
          notifyReadyListeners('Error while loading the AutoUpdater: ' + err);
          return;
        }

        
        
        if (updater.isOutOfDate()) {
          updater.autoUpdate();
          return;
        }

        
        createMediaCaptureController(function(err) {
          if (!err && (mediaCaptureObject && !mediaCaptureObject.isValid())) {
            err = 'The TB Plugin failed to load properly';
          }

          pluginIsReady = true;
          notifyReadyListeners(err);

          OTHelpers.onDOMUnload(destroy);
        });
      });
    };




















OTPlugin.isInstalled = function isInstalled () {
  if (!this.isSupported()) return false;
  return AutoUpdater.isinstalled();
};

OTPlugin.version = function version () {
  return OTPlugin.meta.version;
};

OTPlugin.installedVersion = function installedVersion () {
  return AutoUpdater.installedVersion();
};



OTPlugin.pathToInstaller = function pathToInstaller () {
  return 'https://s3.amazonaws.com/otplugin.tokbox.com/v' +
                    OTPlugin.meta.version + '/otiePluginMain.msi';
};






OTPlugin.ready = function ready (callback) {
  if (OTPlugin.isReady()) {
    var err;

    if (!mediaCaptureObject || !mediaCaptureObject.isValid()) {
      err = 'The TB Plugin failed to load properly';
    }

    callback.call(OTPlugin, err);
  }
  else {
    registerReadyListener(callback);
  }
};


var _getUserMedia = function _getUserMedia(mediaConstraints, success, error) {
  createPeerController(function(err, plugin) {
    if (err) {
      error.call(OTPlugin, err);
      return;
    }

    plugin._.getUserMedia(mediaConstraints.toHash(), function(streamJson) {
      success.call(OTPlugin, MediaStream.fromJson(streamJson, plugin));
    }, error);
  });
};




OTPlugin.getUserMedia = function getUserMedia (userConstraints, success, error) {
  var constraints = new MediaConstraints(userConstraints);

  if (constraints.screenSharing) {
    _getUserMedia(constraints, success, error);
  }
  else {
    var sources = [];
    if (constraints.hasVideo) sources.push('video');
    if (constraints.hasAudio) sources.push('audio');

    mediaCaptureObject.selectSources(sources, function(captureDevices) {
      for (var key in captureDevices) {
        if (captureDevices.hasOwnProperty(key)) {
          OTPlugin.debug(key + ' Capture Device: ' + captureDevices[key]);
        }
      }

      
      constraints.setVideoSource(captureDevices.video);
      constraints.setAudioSource(captureDevices.audio);

      _getUserMedia(constraints, success, error);
    }, error);
  }
};

OTPlugin.initRumorSocket = function(messagingURL, completion) {
  OTPlugin.ready(function(error) {
    if(error) {
      completion(error);
    } else {
      completion(null, new RumorSocket(mediaCaptureObject, messagingURL));
    }
  });
};





OTPlugin.initPeerConnection = function initPeerConnection (iceServers,
                                                           options,
                                                           localStream,
                                                           completion) {

  var gotPeerObject = function(err, plugin) {
    if (err) {
      completion.call(OTPlugin, err);
      return;
    }

    OTPlugin.debug('Got PeerConnection for ' + plugin.id);
    PeerConnection.create(iceServers, options, plugin, function(err, peerConnection) {
      if (err) {
        completion.call(OTPlugin, err);
        return;
      }

      completion.call(OTPlugin, null, peerConnection);
    });
  };

  
  
  
  
  
  
  if (localStream && localStream._.plugin) {
    gotPeerObject(null, localStream._.plugin);
  }
  else {
    createPeerController(gotPeerObject);
  }
};


OTPlugin.RTCSessionDescription = function RTCSessionDescription (options) {
  this.type = options.type;
  this.sdp = options.sdp;
};


OTPlugin.RTCIceCandidate = function RTCIceCandidate (options) {
  this.sdpMid = options.sdpMid;
  this.sdpMLineIndex = parseInt(options.sdpMLineIndex, 10);
  this.candidate = options.candidate;
};





shim();

OTHelpers.onDOMLoad(onDomReady);


})(this);





!(function(window, OT) {










if (location.protocol === 'file:') {
  
  alert('You cannot test a page using WebRTC through the file system due to browser ' +
    'permissions. You must run it over a web server.');
}

var OT = window.OT || {};


OT.APIKEY = (function(){
  
  var scriptSrc = (function(){
    var s = document.getElementsByTagName('script');
    s = s[s.length - 1];
    s = s.getAttribute('src') || s.src;
    return s;
  })();

  var m = scriptSrc.match(/[\?\&]apikey=([^&]+)/i);
  return m ? m[1] : '';
})();


if (!window.OT) window.OT = OT;
if (!window.TB) window.TB = OT;



OT.properties = {
  version: 'v2.5.0',         
  build: '17447b9',    

  
  debug: 'false',
  
  websiteURL: 'http://www.tokbox.com',

  
  cdnURL: 'http://static.opentok.com',
  
  loggingURL: 'http://hlg.tokbox.com/prod',

  
  apiURL: 'http://anvil.opentok.com',

  
  messagingProtocol: 'wss',
  
  messagingPort: 443,

  
  supportSSL: 'true',
  
  cdnURLSSL: 'https://static.opentok.com',
  
  loggingURLSSL: 'https://hlg.tokbox.com/prod',

  
  apiURLSSL: 'https://anvil.opentok.com',

  minimumVersion: {
    firefox: parseFloat('29'),
    chrome: parseFloat('34')
  }
};











OT.$ = window.OTHelpers;


OT.$.eventing(OT);



OT.$.defineGetters = function(self, getters, enumerable) {
  var propsDefinition = {};

  if (enumerable === void 0) enumerable = false;

  for (var key in getters) {
    if(!getters.hasOwnProperty(key)) {
      continue;
    }

    propsDefinition[key] = {
      get: getters[key],
      enumerable: enumerable
    };
  }

  Object.defineProperties(self, propsDefinition);
};




OT.Modal = OT.$.Modal;


OT.$.useLogHelpers(OT);

var _debugHeaderLogged = false,
    _setLogLevel = OT.setLogLevel;


OT.setLogLevel = function(level) {
  
  OT.$.setLogLevel(level);
  var retVal = _setLogLevel.call(OT, level);
  if (OT.shouldLog(OT.DEBUG) && !_debugHeaderLogged) {
    OT.debug('OpenTok JavaScript library ' + OT.properties.version);
    OT.debug('Release notes: ' + OT.properties.websiteURL +
      '/opentok/webrtc/docs/js/release-notes.html');
    OT.debug('Known issues: ' + OT.properties.websiteURL +
      '/opentok/webrtc/docs/js/release-notes.html#knownIssues');
    _debugHeaderLogged = true;
  }
  OT.debug('OT.setLogLevel(' + retVal + ')');
  return retVal;
};

var debugTrue = OT.properties.debug === 'true' || OT.properties.debug === true;
OT.setLogLevel(debugTrue ? OT.DEBUG : OT.ERROR);



if (OTPlugin && OTPlugin.isInstalled()) {
  OT.$.env.userAgent += '; OTPlugin ' + OTPlugin.version();
}


OT.$.userAgent = function() { return OT.$.env.userAgent; };

























































































OT.Rumor = {
  MessageType: {
    
    
    
    SUBSCRIBE: 0,

    
    
    UNSUBSCRIBE: 1,

    
    
    MESSAGE: 2,

    
    
    
    CONNECT: 3,

    
    
    DISCONNECT: 4,

    
    PING: 7,
    PONG: 8,
    STATUS: 9
  }
};








!(function() {

  OT.Rumor.PluginSocket = function(messagingURL, events) {

    var webSocket,
        state = 'initializing';

    OTPlugin.initRumorSocket(messagingURL, OT.$.bind(function(err, rumorSocket) {
      if(err) {
        state = 'closed';
        events.onClose({ code: 4999 });
      } else if(state === 'initializing') {
        webSocket = rumorSocket;

        webSocket.onOpen(function() {
          state = 'open';
          events.onOpen();
        });
        webSocket.onClose(function(error) {
          state = 'closed'; 
          events.onClose({ code: error });
        });
        webSocket.onError(function(error) {
          state = 'closed'; 
          events.onError(error);
          
          events.onClose({ code: error });
        });

        webSocket.onMessage(function(type, addresses, headers, payload) {
          var msg = new OT.Rumor.Message(type, addresses, headers, payload);
          events.onMessage(msg);
        });

        webSocket.open();
      } else {
        this.close();
      }
    }, this));

    this.close = function() {
      if(state === 'initializing' || state === 'closed') {
        state = 'closed';
        return;
      }

      webSocket.close(1000, '');
    };

    this.send = function(msg) {
      if(state === 'open') {
        webSocket.send(msg);
      }
    };

    this.isClosed = function() {
      return state === 'closed';
    };

  };

}(this));


























(function(global) {
  'use strict';

  if(OT.$.env && OT.$.env.name === 'IE' && OT.$.env.version < 10) {
    return; 
  }

  if ( (global.TextEncoder !== void 0) && (global.TextDecoder !== void 0))  {
    
    return;
  }

  

  
  
  

  





  function inRange(a, min, max) {
    return min <= a && a <= max;
  }

  




  function div(n, d) {
    return Math.floor(n / d);
  }


  
  
  
  

  
  
  

  
  
  

  
  var EOF_byte = -1;
  
  var EOF_code_point = -1;

  



  function ByteInputStream(bytes) {
    
    var pos = 0;

    
    this.get = function() {
      return (pos >= bytes.length) ? EOF_byte : Number(bytes[pos]);
    };

    

    this.offset = function(n) {
      pos += n;
      if (pos < 0) {
        throw new Error('Seeking past start of the buffer');
      }
      if (pos > bytes.length) {
        throw new Error('Seeking past EOF');
      }
    };

    




    this.match = function(test) {
      if (test.length > pos + bytes.length) {
        return false;
      }
      var i;
      for (i = 0; i < test.length; i += 1) {
        if (Number(bytes[pos + i]) !== test[i]) {
          return false;
        }
      }
      return true;
    };
  }

  



  function ByteOutputStream(bytes) {
    
    var pos = 0;

    



    this.emit = function(var_args) {
      
      var last = EOF_byte;
      var i;
      for (i = 0; i < arguments.length; ++i) {
        last = Number(arguments[i]);
        bytes[pos++] = last;
      }
      return last;
    };
  }

  



  function CodePointInputStream(string) {
    



    function stringToCodePoints(string) {
      
      var cps = [];
      
      var i = 0, n = string.length;
      while (i < string.length) {
        var c = string.charCodeAt(i);
        if (!inRange(c, 0xD800, 0xDFFF)) {
          cps.push(c);
        } else if (inRange(c, 0xDC00, 0xDFFF)) {
          cps.push(0xFFFD);
        } else { 
          if (i === n - 1) {
            cps.push(0xFFFD);
          } else {
            var d = string.charCodeAt(i + 1);
            if (inRange(d, 0xDC00, 0xDFFF)) {
              var a = c & 0x3FF;
              var b = d & 0x3FF;
              i += 1;
              cps.push(0x10000 + (a << 10) + b);
            } else {
              cps.push(0xFFFD);
            }
          }
        }
        i += 1;
      }
      return cps;
    }

    
    var pos = 0;
    
    var cps = stringToCodePoints(string);

    

    this.offset = function(n) {
      pos += n;
      if (pos < 0) {
        throw new Error('Seeking past start of the buffer');
      }
      if (pos > cps.length) {
        throw new Error('Seeking past EOF');
      }
    };


    
    this.get = function() {
      if (pos >= cps.length) {
        return EOF_code_point;
      }
      return cps[pos];
    };
  }

  


  function CodePointOutputStream() {
    
    var string = '';

    
    this.string = function() {
      return string;
    };

    
    this.emit = function(c) {
      if (c <= 0xFFFF) {
        string += String.fromCharCode(c);
      } else {
        c -= 0x10000;
        string += String.fromCharCode(0xD800 + ((c >> 10) & 0x3ff));
        string += String.fromCharCode(0xDC00 + (c & 0x3ff));
      }
    };
  }

  



  function EncodingError(message) {
    this.name = 'EncodingError';
    this.message = message;
    this.code = 0;
  }
  EncodingError.prototype = Error.prototype;

  




  function decoderError(fatal, opt_code_point) {
    if (fatal) {
      throw new EncodingError('Decoder error');
    }
    return opt_code_point || 0xFFFD;
  }

  


  function encoderError(code_point) {
    throw new EncodingError('The code point ' + code_point +
                            ' could not be encoded.');
  }

  



  function getEncoding(label) {
    label = String(label).trim().toLowerCase();
    if (Object.prototype.hasOwnProperty.call(label_to_encoding, label)) {
      return label_to_encoding[label];
    }
    return null;
  }

  

  var encodings = [
    {
      'encodings': [
        {
          'labels': [
            'unicode-1-1-utf-8',
            'utf-8',
            'utf8'
          ],
          'name': 'utf-8'
        }
      ],
      'heading': 'The Encoding'
    },
    {
      'encodings': [
        {
          'labels': [
            'cp864',
            'ibm864'
          ],
          'name': 'ibm864'
        },
        {
          'labels': [
            'cp866',
            'ibm866'
          ],
          'name': 'ibm866'
        },
        {
          'labels': [
            'csisolatin2',
            'iso-8859-2',
            'iso-ir-101',
            'iso8859-2',
            'iso_8859-2',
            'l2',
            'latin2'
          ],
          'name': 'iso-8859-2'
        },
        {
          'labels': [
            'csisolatin3',
            'iso-8859-3',
            'iso_8859-3',
            'iso-ir-109',
            'l3',
            'latin3'
          ],
          'name': 'iso-8859-3'
        },
        {
          'labels': [
            'csisolatin4',
            'iso-8859-4',
            'iso_8859-4',
            'iso-ir-110',
            'l4',
            'latin4'
          ],
          'name': 'iso-8859-4'
        },
        {
          'labels': [
            'csisolatincyrillic',
            'cyrillic',
            'iso-8859-5',
            'iso_8859-5',
            'iso-ir-144'
          ],
          'name': 'iso-8859-5'
        },
        {
          'labels': [
            'arabic',
            'csisolatinarabic',
            'ecma-114',
            'iso-8859-6',
            'iso_8859-6',
            'iso-ir-127'
          ],
          'name': 'iso-8859-6'
        },
        {
          'labels': [
            'csisolatingreek',
            'ecma-118',
            'elot_928',
            'greek',
            'greek8',
            'iso-8859-7',
            'iso_8859-7',
            'iso-ir-126'
          ],
          'name': 'iso-8859-7'
        },
        {
          'labels': [
            'csisolatinhebrew',
            'hebrew',
            'iso-8859-8',
            'iso-8859-8-i',
            'iso-ir-138',
            'iso_8859-8',
            'visual'
          ],
          'name': 'iso-8859-8'
        },
        {
          'labels': [
            'csisolatin6',
            'iso-8859-10',
            'iso-ir-157',
            'iso8859-10',
            'l6',
            'latin6'
          ],
          'name': 'iso-8859-10'
        },
        {
          'labels': [
            'iso-8859-13'
          ],
          'name': 'iso-8859-13'
        },
        {
          'labels': [
            'iso-8859-14',
            'iso8859-14'
          ],
          'name': 'iso-8859-14'
        },
        {
          'labels': [
            'iso-8859-15',
            'iso_8859-15'
          ],
          'name': 'iso-8859-15'
        },
        {
          'labels': [
            'iso-8859-16'
          ],
          'name': 'iso-8859-16'
        },
        {
          'labels': [
            'koi8-r',
            'koi8_r'
          ],
          'name': 'koi8-r'
        },
        {
          'labels': [
            'koi8-u'
          ],
          'name': 'koi8-u'
        },
        {
          'labels': [
            'csmacintosh',
            'mac',
            'macintosh',
            'x-mac-roman'
          ],
          'name': 'macintosh'
        },
        {
          'labels': [
            'iso-8859-11',
            'tis-620',
            'windows-874'
          ],
          'name': 'windows-874'
        },
        {
          'labels': [
            'windows-1250',
            'x-cp1250'
          ],
          'name': 'windows-1250'
        },
        {
          'labels': [
            'windows-1251',
            'x-cp1251'
          ],
          'name': 'windows-1251'
        },
        {
          'labels': [
            'ascii',
            'ansi_x3.4-1968',
            'csisolatin1',
            'iso-8859-1',
            'iso8859-1',
            'iso_8859-1',
            'l1',
            'latin1',
            'us-ascii',
            'windows-1252'
          ],
          'name': 'windows-1252'
        },
        {
          'labels': [
            'cp1253',
            'windows-1253'
          ],
          'name': 'windows-1253'
        },
        {
          'labels': [
            'csisolatin5',
            'iso-8859-9',
            'iso-ir-148',
            'l5',
            'latin5',
            'windows-1254'
          ],
          'name': 'windows-1254'
        },
        {
          'labels': [
            'cp1255',
            'windows-1255'
          ],
          'name': 'windows-1255'
        },
        {
          'labels': [
            'cp1256',
            'windows-1256'
          ],
          'name': 'windows-1256'
        },
        {
          'labels': [
            'windows-1257'
          ],
          'name': 'windows-1257'
        },
        {
          'labels': [
            'cp1258',
            'windows-1258'
          ],
          'name': 'windows-1258'
        },
        {
          'labels': [
            'x-mac-cyrillic',
            'x-mac-ukrainian'
          ],
          'name': 'x-mac-cyrillic'
        }
      ],
      'heading': 'Legacy single-byte encodings'
    },
    {
      'encodings': [
        {
          'labels': [
            'chinese',
            'csgb2312',
            'csiso58gb231280',
            'gb2312',
            'gbk',
            'gb_2312',
            'gb_2312-80',
            'iso-ir-58',
            'x-gbk'
          ],
          'name': 'gbk'
        },
        {
          'labels': [
            'gb18030'
          ],
          'name': 'gb18030'
        },
        {
          'labels': [
            'hz-gb-2312'
          ],
          'name': 'hz-gb-2312'
        }
      ],
      'heading': 'Legacy multi-byte Chinese (simplified) encodings'
    },
    {
      'encodings': [
        {
          'labels': [
            'big5',
            'big5-hkscs',
            'cn-big5',
            'csbig5',
            'x-x-big5'
          ],
          'name': 'big5'
        }
      ],
      'heading': 'Legacy multi-byte Chinese (traditional) encodings'
    },
    {
      'encodings': [
        {
          'labels': [
            'cseucpkdfmtjapanese',
            'euc-jp',
            'x-euc-jp'
          ],
          'name': 'euc-jp'
        },
        {
          'labels': [
            'csiso2022jp',
            'iso-2022-jp'
          ],
          'name': 'iso-2022-jp'
        },
        {
          'labels': [
            'csshiftjis',
            'ms_kanji',
            'shift-jis',
            'shift_jis',
            'sjis',
            'windows-31j',
            'x-sjis'
          ],
          'name': 'shift_jis'
        }
      ],
      'heading': 'Legacy multi-byte Japanese encodings'
    },
    {
      'encodings': [
        {
          'labels': [
            'cseuckr',
            'csksc56011987',
            'euc-kr',
            'iso-ir-149',
            'korean',
            'ks_c_5601-1987',
            'ks_c_5601-1989',
            'ksc5601',
            'ksc_5601',
            'windows-949'
          ],
          'name': 'euc-kr'
        },
        {
          'labels': [
            'csiso2022kr',
            'iso-2022-kr'
          ],
          'name': 'iso-2022-kr'
        }
      ],
      'heading': 'Legacy multi-byte Korean encodings'
    },
    {
      'encodings': [
        {
          'labels': [
            'utf-16',
            'utf-16le'
          ],
          'name': 'utf-16'
        },
        {
          'labels': [
            'utf-16be'
          ],
          'name': 'utf-16be'
        }
      ],
      'heading': 'Legacy utf-16 encodings'
    }
  ];

  var name_to_encoding = {};
  var label_to_encoding = {};
  encodings.forEach(function(category) {
    category.encodings.forEach(function(encoding) {
      name_to_encoding[encoding.name] = encoding;
      encoding.labels.forEach(function(label) {
        label_to_encoding[label] = encoding;
      });
    });
  });

  
  
  

  





  function indexCodePointFor(pointer, index) {
    return (index || [])[pointer] || null;
  }

  





  function indexPointerFor(code_point, index) {
    var pointer = index.indexOf(code_point);
    return pointer === -1 ? null : pointer;
  }

  
  var indexes = global['encoding-indexes'] || {};


  
  
  

  

  



  function UTF8Decoder(options) {
    var fatal = options.fatal;
    var utf8_code_point = 0,
        utf8_bytes_needed = 0,
        utf8_bytes_seen = 0,
        utf8_lower_boundary = 0;

    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte) {
        if (utf8_bytes_needed !== 0) {
          return decoderError(fatal);
        }
        return EOF_code_point;
      }
      byte_pointer.offset(1);

      if (utf8_bytes_needed === 0) {
        if (inRange(bite, 0x00, 0x7F)) {
          return bite;
        }
        if (inRange(bite, 0xC2, 0xDF)) {
          utf8_bytes_needed = 1;
          utf8_lower_boundary = 0x80;
          utf8_code_point = bite - 0xC0;
        } else if (inRange(bite, 0xE0, 0xEF)) {
          utf8_bytes_needed = 2;
          utf8_lower_boundary = 0x800;
          utf8_code_point = bite - 0xE0;
        } else if (inRange(bite, 0xF0, 0xF4)) {
          utf8_bytes_needed = 3;
          utf8_lower_boundary = 0x10000;
          utf8_code_point = bite - 0xF0;
        } else {
          return decoderError(fatal);
        }
        utf8_code_point = utf8_code_point * Math.pow(64, utf8_bytes_needed);
        return null;
      }
      if (!inRange(bite, 0x80, 0xBF)) {
        utf8_code_point = 0;
        utf8_bytes_needed = 0;
        utf8_bytes_seen = 0;
        utf8_lower_boundary = 0;
        byte_pointer.offset(-1);
        return decoderError(fatal);
      }
      utf8_bytes_seen += 1;
      utf8_code_point = utf8_code_point + (bite - 0x80) *
          Math.pow(64, utf8_bytes_needed - utf8_bytes_seen);
      if (utf8_bytes_seen !== utf8_bytes_needed) {
        return null;
      }
      var code_point = utf8_code_point;
      var lower_boundary = utf8_lower_boundary;
      utf8_code_point = 0;
      utf8_bytes_needed = 0;
      utf8_bytes_seen = 0;
      utf8_lower_boundary = 0;
      if (inRange(code_point, lower_boundary, 0x10FFFF) &&
          !inRange(code_point, 0xD800, 0xDFFF)) {
        return code_point;
      }
      return decoderError(fatal);
    };
  }

  



  function UTF8Encoder(options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0xD800, 0xDFFF)) {
        return encoderError(code_point);
      }
      if (inRange(code_point, 0x0000, 0x007f)) {
        return output_byte_stream.emit(code_point);
      }
      var count, offset;
      if (inRange(code_point, 0x0080, 0x07FF)) {
        count = 1;
        offset = 0xC0;
      } else if (inRange(code_point, 0x0800, 0xFFFF)) {
        count = 2;
        offset = 0xE0;
      } else if (inRange(code_point, 0x10000, 0x10FFFF)) {
        count = 3;
        offset = 0xF0;
      }
      var result = output_byte_stream.emit(
          div(code_point, Math.pow(64, count)) + offset);
      while (count > 0) {
        var temp = div(code_point, Math.pow(64, count - 1));
        result = output_byte_stream.emit(0x80 + (temp % 64));
        count -= 1;
      }
      return result;
    };
  }

  name_to_encoding['utf-8'].getEncoder = function(options) {
    return new UTF8Encoder(options);
  };
  name_to_encoding['utf-8'].getDecoder = function(options) {
    return new UTF8Decoder(options);
  };


  
  



  function detectEncoding(label, input_stream) {
    if (input_stream.match([0xFF, 0xFE])) {
      input_stream.offset(2);
      return 'utf-16';
    }
    if (input_stream.match([0xFE, 0xFF])) {
      input_stream.offset(2);
      return 'utf-16be';
    }
    if (input_stream.match([0xEF, 0xBB, 0xBF])) {
      input_stream.offset(3);
      return 'utf-8';
    }
    return label;
  }

  



  function consumeBOM(label, input_stream) {
    if (input_stream.match([0xFF, 0xFE]) && label === 'utf-16') {
      input_stream.offset(2);
      return;
    }
    if (input_stream.match([0xFE, 0xFF]) && label == 'utf-16be') {
      input_stream.offset(2);
      return;
    }
    if (input_stream.match([0xEF, 0xBB, 0xBF]) && label == 'utf-8') {
      input_stream.offset(3);
      return;
    }
  }

  
  
  

  
  var DEFAULT_ENCODING = 'utf-8';

  





  function TextEncoder(opt_encoding, options) {
    if (!this || this === global) {
      return new TextEncoder(opt_encoding, options);
    }
    opt_encoding = opt_encoding ? String(opt_encoding) : DEFAULT_ENCODING;
    options = Object(options);
    
    this._encoding = getEncoding(opt_encoding);
    if (this._encoding === null || (this._encoding.name !== 'utf-8' &&
                                    this._encoding.name !== 'utf-16' &&
                                    this._encoding.name !== 'utf-16be'))
      throw new TypeError('Unknown encoding: ' + opt_encoding);
    
    this._streaming = false;
    
    this._encoder = null;
    
    this._options = { fatal: Boolean(options.fatal) };

    if (Object.defineProperty) {
      Object.defineProperty(
          this, 'encoding',
          { get: function() { return this._encoding.name; } });
    } else {
      this.encoding = this._encoding.name;
    }

    return this;
  }

  TextEncoder.prototype = {
    



    encode: function encode(opt_string, options) {
      opt_string = opt_string ? String(opt_string) : '';
      options = Object(options);
      
      if (!this._streaming) {
        this._encoder = this._encoding.getEncoder(this._options);
      }
      this._streaming = Boolean(options.stream);

      var bytes = [];
      var output_stream = new ByteOutputStream(bytes);
      var input_stream = new CodePointInputStream(opt_string);
      while (input_stream.get() !== EOF_code_point) {
        this._encoder.encode(output_stream, input_stream);
      }
      if (!this._streaming) {
        var last_byte;
        do {
          last_byte = this._encoder.encode(output_stream, input_stream);
        } while (last_byte !== EOF_byte);
        this._encoder = null;
      }
      return new Uint8Array(bytes);
    }
  };


  





  function TextDecoder(opt_encoding, options) {
    if (!this || this === global) {
      return new TextDecoder(opt_encoding, options);
    }
    opt_encoding = opt_encoding ? String(opt_encoding) : DEFAULT_ENCODING;
    options = Object(options);
    
    this._encoding = getEncoding(opt_encoding);
    if (this._encoding === null)
      throw new TypeError('Unknown encoding: ' + opt_encoding);

    
    this._streaming = false;
    
    this._decoder = null;
    
    this._options = { fatal: Boolean(options.fatal) };

    if (Object.defineProperty) {
      Object.defineProperty(
          this, 'encoding',
          { get: function() { return this._encoding.name; } });
    } else {
      this.encoding = this._encoding.name;
    }

    return this;
  }

  
  
  
  TextDecoder.prototype = {
    



    decode: function decode(opt_view, options) {
      if (opt_view && !('buffer' in opt_view && 'byteOffset' in opt_view &&
                        'byteLength' in opt_view)) {
        throw new TypeError('Expected ArrayBufferView');
      } else if (!opt_view) {
        opt_view = new Uint8Array(0);
      }
      options = Object(options);

      if (!this._streaming) {
        this._decoder = this._encoding.getDecoder(this._options);
      }
      this._streaming = Boolean(options.stream);

      var bytes = new Uint8Array(opt_view.buffer,
                                 opt_view.byteOffset,
                                 opt_view.byteLength);
      var input_stream = new ByteInputStream(bytes);

      if (!this._BOMseen) {
        
        this._BOMseen = true;
        consumeBOM(this._encoding.name, input_stream);
      }

      var output_stream = new CodePointOutputStream(), code_point;
      while (input_stream.get() !== EOF_byte) {
        code_point = this._decoder.decode(input_stream);
        if (code_point !== null && code_point !== EOF_code_point) {
          output_stream.emit(code_point);
        }
      }
      if (!this._streaming) {
        do {
          code_point = this._decoder.decode(input_stream);
          if (code_point !== null && code_point !== EOF_code_point) {
            output_stream.emit(code_point);
          }
        } while (code_point !== EOF_code_point &&
                 input_stream.get() != EOF_byte);
        this._decoder = null;
      }
      return output_stream.string();
    }
  };

  global['TextEncoder'] = global['TextEncoder'] || TextEncoder;
  global['TextDecoder'] = global['TextDecoder'] || TextDecoder;

  

}(this));















OT.Rumor.Message = function (type, toAddress, headers, data) {
  this.type = type;
  this.toAddress = toAddress;
  this.headers = headers;
  this.data = data;

  this.transactionId = this.headers['TRANSACTION-ID'];
  this.status = this.headers.STATUS;
  this.isError = !(this.status && this.status[0] === '2');
};

OT.Rumor.Message.prototype.serialize = function () {
  var offset = 8,
      cBuf = 7,
      address = [],
      headerKey = [],
      headerVal = [],
      strArray,
      dataView,
      i,
      j;

  
  cBuf++;

  
  for (i = 0; i < this.toAddress.length; i++) {
    
    address.push(new TextEncoder('utf-8').encode(this.toAddress[i]));
    cBuf += 2;
    cBuf += address[i].length;
  }

  
  cBuf++;

  
  i = 0;

  for (var key in this.headers) {
    if(!this.headers.hasOwnProperty(key)) {
      continue;
    }
    headerKey.push(new TextEncoder('utf-8').encode(key));
    headerVal.push(new TextEncoder('utf-8').encode(this.headers[key]));
    cBuf += 4;
    cBuf += headerKey[i].length;
    cBuf += headerVal[i].length;

    i++;
  }

  dataView = new TextEncoder('utf-8').encode(this.data);
  cBuf += dataView.length;

  
  var buffer = new ArrayBuffer(cBuf);
  var uint8View = new Uint8Array(buffer, 0, cBuf);

  
  cBuf -= 4;

  
  uint8View[0] = (cBuf & 0xFF000000) >>> 24;
  uint8View[1] = (cBuf & 0x00FF0000) >>> 16;
  uint8View[2] = (cBuf & 0x0000FF00) >>>  8;
  uint8View[3] = (cBuf & 0x000000FF) >>>  0;

  
  uint8View[4] = 0;
  uint8View[5] = 0;

  
  uint8View[6] = this.type;
  uint8View[7] = this.toAddress.length;

  
  for (i = 0; i < address.length; i++) {
    strArray = address[i];
    uint8View[offset++] = strArray.length >> 8 & 0xFF;
    uint8View[offset++] = strArray.length >> 0 & 0xFF;
    for (j = 0; j < strArray.length; j++) {
      uint8View[offset++] = strArray[j];
    }
  }

  uint8View[offset++] = headerKey.length;

  
  for (i = 0; i < headerKey.length; i++) {
    strArray = headerKey[i];
    uint8View[offset++] = strArray.length >> 8 & 0xFF;
    uint8View[offset++] = strArray.length >> 0 & 0xFF;
    for (j = 0; j < strArray.length; j++) {
      uint8View[offset++] = strArray[j];
    }

    strArray = headerVal[i];
    uint8View[offset++] = strArray.length >> 8 & 0xFF;
    uint8View[offset++] = strArray.length >> 0 & 0xFF;
    for (j = 0; j < strArray.length; j++) {
      uint8View[offset++] = strArray[j];
    }
  }

  
  for (i = 0; i < dataView.length; i++) {
    uint8View[offset++] = dataView[i];
  }

  return buffer;
};

function toArrayBuffer(buffer) {
  var ab = new ArrayBuffer(buffer.length);
  var view = new Uint8Array(ab);
  for (var i = 0; i < buffer.length; ++i) {
    view[i] = buffer[i];
  }
  return ab;
}

OT.Rumor.Message.deserialize = function (buffer) {

  if(typeof Buffer !== 'undefined' &&
    Buffer.isBuffer(buffer)) {
    buffer = toArrayBuffer(buffer);
  }
  var cBuf = 0,
      type,
      offset = 8,
      uint8View = new Uint8Array(buffer),
      strView,
      headerlen,
      headers,
      keyStr,
      valStr,
      length,
      i;

  
  cBuf += uint8View[0] << 24;
  cBuf += uint8View[1] << 16;
  cBuf += uint8View[2] <<  8;
  cBuf += uint8View[3] <<  0;

  type = uint8View[6];
  var address = [];

  for (i = 0; i < uint8View[7]; i++) {
    length = uint8View[offset++] << 8;
    length += uint8View[offset++];
    strView = new Uint8Array(buffer, offset, length);
    
    address[i] = new TextDecoder('utf-8').decode(strView);
    offset += length;
  }

  headerlen = uint8View[offset++];
  headers = {};

  for (i = 0; i < headerlen; i++) {
    length = uint8View[offset++] << 8;
    length += uint8View[offset++];
    strView = new Uint8Array(buffer, offset, length);
    keyStr = new TextDecoder('utf-8').decode(strView);
    offset += length;

    length = uint8View[offset++] << 8;
    length += uint8View[offset++];
    strView = new Uint8Array(buffer, offset, length);
    valStr = new TextDecoder('utf-8').decode(strView);
    headers[keyStr] = valStr;
    offset += length;
  }

  var dataView = new Uint8Array(buffer, offset);
  var data = new TextDecoder('utf-8').decode(dataView);

  return new OT.Rumor.Message(type, address, headers, data);
};


OT.Rumor.Message.Connect = function (uniqueId, notifyDisconnectAddress) {
  var headers = {
    uniqueId: uniqueId,
    notifyDisconnectAddress: notifyDisconnectAddress
  };

  return new OT.Rumor.Message(OT.Rumor.MessageType.CONNECT, [], headers, '');
};

OT.Rumor.Message.Disconnect = function () {
  return new OT.Rumor.Message(OT.Rumor.MessageType.DISCONNECT, [], {}, '');
};

OT.Rumor.Message.Subscribe = function(topics) {
  return new OT.Rumor.Message(OT.Rumor.MessageType.SUBSCRIBE, topics, {}, '');
};

OT.Rumor.Message.Unsubscribe = function(topics) {
  return new OT.Rumor.Message(OT.Rumor.MessageType.UNSUBSCRIBE, topics, {}, '');
};

OT.Rumor.Message.Publish = function(topics, message, headers) {
  return new OT.Rumor.Message(OT.Rumor.MessageType.MESSAGE, topics, headers||{}, message || '');
};





OT.Rumor.Message.Ping = function() {
  return new OT.Rumor.Message(OT.Rumor.MessageType.PING, [], {}, '');
};









!(function() {

  var BUFFER_DRAIN_INTERVAL = 100,
      
      BUFFER_DRAIN_MAX_RETRIES = 10;

  OT.Rumor.NativeSocket = function(TheWebSocket, messagingURL, events) {

    var webSocket,
        disconnectWhenSendBufferIsDrained,
        bufferDrainTimeout,           
        close;

    webSocket = new TheWebSocket(messagingURL);
    webSocket.binaryType = 'arraybuffer';

    webSocket.onopen = events.onOpen;
    webSocket.onclose = events.onClose;
    webSocket.onerror = events.onError;

    webSocket.onmessage = function(message) {
      if (!OT) {
        
        
        return;
      }

      var msg = OT.Rumor.Message.deserialize(message.data);
      events.onMessage(msg);
    };

    
    
    
    disconnectWhenSendBufferIsDrained =
      function disconnectWhenSendBufferIsDrained (bufferDrainRetries) {
      if (!webSocket) return;

      if (bufferDrainRetries === void 0) bufferDrainRetries = 0;
      if (bufferDrainTimeout) clearTimeout(bufferDrainTimeout);

      if (webSocket.bufferedAmount > 0 &&
        (bufferDrainRetries + 1) <= BUFFER_DRAIN_MAX_RETRIES) {
        bufferDrainTimeout = setTimeout(disconnectWhenSendBufferIsDrained,
          BUFFER_DRAIN_INTERVAL, bufferDrainRetries+1);

      } else {
        close();
      }
    };

    close = function close() {
      webSocket.close();
    };

    this.close = function(drainBuffer) {
      if (drainBuffer) {
        disconnectWhenSendBufferIsDrained();
      } else {
        close();
      }
    };

    this.send = function(msg) {
      webSocket.send(msg.serialize());
    };

    this.isClosed = function() {
      return webSocket.readyState === 3;
    };

  };


}(this));










var WEB_SOCKET_KEEP_ALIVE_INTERVAL = 9000,

    
    
    
    WEB_SOCKET_CONNECTIVITY_TIMEOUT = 5*WEB_SOCKET_KEEP_ALIVE_INTERVAL - 100,

    wsCloseErrorCodes;



wsCloseErrorCodes = {
  1002:  'The endpoint is terminating the connection due to a protocol error. ' +
    '(CLOSE_PROTOCOL_ERROR)',
  1003:  'The connection is being terminated because the endpoint received data of ' +
    'a type it cannot accept (for example, a text-only endpoint received binary data). ' +
    '(CLOSE_UNSUPPORTED)',
  1004:  'The endpoint is terminating the connection because a data frame was received ' +
    'that is too large. (CLOSE_TOO_LARGE)',
  1005:  'Indicates that no status code was provided even though one was expected. ' +
  '(CLOSE_NO_STATUS)',
  1006:  'Used to indicate that a connection was closed abnormally (that is, with no ' +
    'close frame being sent) when a status code is expected. (CLOSE_ABNORMAL)',
  1007: 'Indicates that an endpoint is terminating the connection because it has received ' +
    'data within a message that was not consistent with the type of the message (e.g., ' +
    'non-UTF-8 [RFC3629] data within a text message)',
  1008: 'Indicates that an endpoint is terminating the connection because it has received a ' +
    'message that violates its policy.  This is a generic status code that can be returned ' +
    'when there is no other more suitable status code (e.g., 1003 or 1009) or if there is a ' +
    'need to hide specific details about the policy',
  1009: 'Indicates that an endpoint is terminating the connection because it has received a ' +
    'message that is too big for it to process',
  1011: 'Indicates that a server is terminating the connection because it encountered an ' +
    'unexpected condition that prevented it from fulfilling the request',

  
  4001:   'Connectivity loss was detected as it was too long since the socket received the ' +
    'last PONG message'
};

OT.Rumor.SocketError = function(code, message) {
  this.code = code;
  this.message = message;
};



OT.Rumor.Socket = function(messagingURL, notifyDisconnectAddress, NativeSocket) {

  var states = ['disconnected',  'error', 'connected', 'connecting', 'disconnecting'],
      webSocket,
      id,
      onOpen,
      onError,
      onClose,
      onMessage,
      connectCallback,
      connectTimeout,
      lastMessageTimestamp,         
      keepAliveTimer;               


  
  var stateChanged = function(newState) {
        switch (newState) {
          case 'disconnected':
          case 'error':
            webSocket = null;
            if (onClose) {
              var error;
              if(hasLostConnectivity()) {
                error = new Error(wsCloseErrorCodes[4001]);
                error.code = 4001;
              }
              onClose(error);
            }
            break;
        }
      },

      setState = OT.$.statable(this, states, 'disconnected', stateChanged),

      validateCallback = function validateCallback (name, callback) {
        if (callback === null || !OT.$.isFunction(callback) ) {
          throw new Error('The Rumor.Socket ' + name +
            ' callback must be a valid function or null');
        }
      },

      error = OT.$.bind(function error (errorMessage) {
        OT.error('Rumor.Socket: ' + errorMessage);

        var socketError = new OT.Rumor.SocketError(null, errorMessage || 'Unknown Socket Error');

        if (connectTimeout) clearTimeout(connectTimeout);

        setState('error');

        if (this.previousState === 'connecting' && connectCallback) {
          connectCallback(socketError, void 0);
          connectCallback = null;
        }

        if (onError) onError(socketError);
      }, this),

      hasLostConnectivity = function hasLostConnectivity () {
        if (!lastMessageTimestamp) return false;

        return (OT.$.now() - lastMessageTimestamp) >= WEB_SOCKET_CONNECTIVITY_TIMEOUT;
      },

      sendKeepAlive = OT.$.bind(function() {
        if (!this.is('connected')) return;

        if ( hasLostConnectivity() ) {
          webSocketDisconnected({code: 4001});
        }
        else  {
          webSocket.send(OT.Rumor.Message.Ping());
          keepAliveTimer = setTimeout(sendKeepAlive, WEB_SOCKET_KEEP_ALIVE_INTERVAL);
        }
      }, this),

      
      
      
      isDOMUnloaded = function isDOMUnloaded () {
        return !window.OT;
      };


  
  var webSocketConnected = OT.$.bind(function webSocketConnected () {
        if (connectTimeout) clearTimeout(connectTimeout);
        if (this.isNot('connecting')) {
          OT.debug('webSocketConnected reached in state other than connecting');
          return;
        }

        
        
        
        
        webSocket.send(OT.Rumor.Message.Connect(id, notifyDisconnectAddress));

        setState('connected');
        if (connectCallback) {
          connectCallback(void 0, id);
          connectCallback = null;
        }

        if (onOpen) onOpen(id);

        keepAliveTimer = setTimeout(function() {
          lastMessageTimestamp = OT.$.now();
          sendKeepAlive();
        }, WEB_SOCKET_KEEP_ALIVE_INTERVAL);
      }, this),

      webSocketConnectTimedOut = function webSocketConnectTimedOut () {
        var webSocketWas = webSocket;
        error('Timed out while waiting for the Rumor socket to connect.');
        
        
        
        
        try {
          webSocketWas.close();
        } catch(x) {}
      },

      webSocketError = function webSocketError () {},
        
        

        
        
        
        
        
        

      webSocketDisconnected = OT.$.bind(function webSocketDisconnected (closeEvent) {
        if (connectTimeout) clearTimeout(connectTimeout);
        if (keepAliveTimer) clearTimeout(keepAliveTimer);

        if (isDOMUnloaded()) {
          
          
          
          
          return;
        }

        if (closeEvent.code !== 1000 && closeEvent.code !== 1001) {
          var reason = closeEvent.reason || closeEvent.message;
          if (!reason && wsCloseErrorCodes.hasOwnProperty(closeEvent.code)) {
            reason = wsCloseErrorCodes[closeEvent.code];
          }

          error('Rumor Socket Disconnected: ' + reason);
        }

        if (this.isNot('error')) setState('disconnected');
      }, this),

      webSocketReceivedMessage = function webSocketReceivedMessage (msg) {
        lastMessageTimestamp = OT.$.now();

        if (onMessage) {
          if (msg.type !== OT.Rumor.MessageType.PONG) {
            onMessage(msg);
          }
        }
      };


  

  this.publish = function (topics, message, headers) {
    webSocket.send(OT.Rumor.Message.Publish(topics, message, headers));
  };

  this.subscribe = function(topics) {
    webSocket.send(OT.Rumor.Message.Subscribe(topics));
  };

  this.unsubscribe = function(topics) {
    webSocket.send(OT.Rumor.Message.Unsubscribe(topics));
  };

  this.connect = function (connectionId, complete) {
    if (this.is('connecting', 'connected')) {
      complete(new OT.Rumor.SocketError(null,
          'Rumor.Socket cannot connect when it is already connecting or connected.'));
      return;
    }

    id = connectionId;
    connectCallback = complete;

    setState('connecting');

    var TheWebSocket = NativeSocket || window.WebSocket;

    var events = {
      onOpen:    webSocketConnected,
      onClose:   webSocketDisconnected,
      onError:   webSocketError,
      onMessage: webSocketReceivedMessage
    };

    try {
      if(typeof TheWebSocket !== 'undefined') {
        webSocket = new OT.Rumor.NativeSocket(TheWebSocket, messagingURL, events);
      } else {
        webSocket = new OT.Rumor.PluginSocket(messagingURL, events);
      }

      connectTimeout = setTimeout(webSocketConnectTimedOut, OT.Rumor.Socket.CONNECT_TIMEOUT);
    }
    catch(e) {
      OT.error(e);

      
      error('Could not connect to the Rumor socket, possibly because of a blocked port.');
    }
  };

  this.disconnect = function(drainSocketBuffer) {
    if (connectTimeout) clearTimeout(connectTimeout);
    if (keepAliveTimer) clearTimeout(keepAliveTimer);

    if (!webSocket) {
      if (this.isNot('error')) setState('disconnected');
      return;
    }

    if (webSocket.isClosed()) {
      if (this.isNot('error')) setState('disconnected');
    }
    else {
      if (this.is('connected')) {
        
        webSocket.send(OT.Rumor.Message.Disconnect());
      }

      
      webSocket.close(drainSocketBuffer);
    }
  };



  OT.$.defineProperties(this, {
    id: {
      get: function() { return id; }
    },

    onOpen: {
      set: function(callback) {
        validateCallback('onOpen', callback);
        onOpen = callback;
      },

      get: function() { return onOpen; }
    },

    onError: {
      set: function(callback) {
        validateCallback('onError', callback);
        onError = callback;
      },

      get: function() { return onError; }
    },

    onClose: {
      set: function(callback) {
        validateCallback('onClose', callback);
        onClose = callback;
      },

      get: function() { return onClose; }
    },

    onMessage: {
      set: function(callback) {
        validateCallback('onMessage', callback);
        onMessage = callback;
      },

      get: function() { return onMessage; }
    }
  });
};


OT.Rumor.Socket.CONNECT_TIMEOUT = 15000;





































OT.Raptor = {
  Actions: {
    
    CONNECT: 100,
    CREATE: 101,
    UPDATE: 102,
    DELETE: 103,
    STATE: 104,

    
    FORCE_DISCONNECT: 105,
    FORCE_UNPUBLISH: 106,
    SIGNAL: 107,

    
    CREATE_ARCHIVE: 108,
    CLOSE_ARCHIVE: 109,
    START_RECORDING_SESSION: 110,
    STOP_RECORDING_SESSION: 111,
    START_RECORDING_STREAM: 112,
    STOP_RECORDING_STREAM: 113,
    LOAD_ARCHIVE: 114,
    START_PLAYBACK: 115,
    STOP_PLAYBACK: 116,

    
    APPSTATE_PUT: 117,
    APPSTATE_DELETE: 118,

    
    OFFER: 119,
    ANSWER: 120,
    PRANSWER: 121,
    CANDIDATE: 122,
    SUBSCRIBE: 123,
    UNSUBSCRIBE: 124,
    QUERY: 125,
    SDP_ANSWER: 126,

    
    PONG: 127,
    REGISTER: 128, 

    QUALITY_CHANGED: 129
  },

  Types: {
    
    RPC_REQUEST: 100,
    RPC_RESPONSE: 101,

    
    STREAM: 102,
    ARCHIVE: 103,
    CONNECTION: 104,
    APPSTATE: 105,
    CONNECTIONCOUNT: 106,
    MODERATION: 107,
    SIGNAL: 108,
    SUBSCRIBER: 110,

    
    JSEP: 109
  }
};








OT.Raptor.serializeMessage = function (message) {
  return JSON.stringify(message);
};











OT.Raptor.deserializeMessage = function (msg) {
  if (msg.length === 0) return {};

  var message = JSON.parse(msg),
      bits = message.uri.substr(1).split('/');

  
  bits.shift();
  if (bits[bits.length-1] === '') bits.pop();

  message.params = {};
  for (var i=0, numBits=bits.length ; i<numBits-1; i+=2) {
    message.params[bits[i]] = bits[i+1];
  }

  
  
  
  if (bits.length % 2 === 0) {
    if (bits[bits.length-2] === 'channel' && bits.length > 6) {
      message.resource = bits[bits.length-4] + '_' + bits[bits.length-2];
    } else {
      message.resource = bits[bits.length-2];
    }
  }
  else {
    if (bits[bits.length-1] === 'channel' && bits.length > 5) {
      message.resource = bits[bits.length-3] + '_' + bits[bits.length-1];
    } else {
      message.resource = bits[bits.length-1];
    }
  }

  message.signature = message.resource + '#' + message.method;
  return message;
};

OT.Raptor.unboxFromRumorMessage = function (rumorMessage) {
  var message = OT.Raptor.deserializeMessage(rumorMessage.data);
  message.transactionId = rumorMessage.transactionId;
  message.fromAddress = rumorMessage.headers['X-TB-FROM-ADDRESS'];

  return message;
};

OT.Raptor.parseIceServers = function (message) {
  try {
    return JSON.parse(message.data).content.iceServers;
  } catch (e) {
    return [];
  }
};

OT.Raptor.Message = {};


OT.Raptor.Message.offer = function (uri, offerSdp) {
  return OT.Raptor.serializeMessage({
    method: 'offer',
    uri: uri,
    content: {
      sdp: offerSdp
    }
  });
};


OT.Raptor.Message.connections = {};

OT.Raptor.Message.connections.create = function (apiKey, sessionId, connectionId) {
  return OT.Raptor.serializeMessage({
    method: 'create',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/connection/' + connectionId,
    content: {
      userAgent: OT.$.env.userAgent
    }
  });
};

OT.Raptor.Message.connections.destroy = function (apiKey, sessionId, connectionId) {
  return OT.Raptor.serializeMessage({
    method: 'delete',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/connection/' + connectionId,
    content: {}
  });
};


OT.Raptor.Message.sessions = {};

OT.Raptor.Message.sessions.get = function (apiKey, sessionId) {
  return OT.Raptor.serializeMessage({
    method: 'read',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId,
    content: {}
  });
};


OT.Raptor.Message.streams = {};

OT.Raptor.Message.streams.get = function (apiKey, sessionId, streamId) {
  return OT.Raptor.serializeMessage({
    method: 'read',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/stream/' + streamId,
    content: {}
  });
};

OT.Raptor.Message.streams.channelFromOTChannel = function(channel) {
  var raptorChannel = {
    id: channel.id,
    type: channel.type,
    active: channel.active
  };

  if (channel.type === 'video') {
    raptorChannel.width = channel.width;
    raptorChannel.height = channel.height;
    raptorChannel.orientation = channel.orientation;
    raptorChannel.frameRate = channel.frameRate;
    if (channel.source !== 'default') {
      raptorChannel.source = channel.source;
    }
    raptorChannel.fitMode = channel.fitMode;
  }

  return raptorChannel;
};

OT.Raptor.Message.streams.create = function (apiKey, sessionId, streamId, name,
  audioFallbackEnabled, channels, minBitrate, maxBitrate) {
  var messageContent = {
    id: streamId,
    name: name,
    audioFallbackEnabled: audioFallbackEnabled,
    channel: OT.$.map(channels, function(channel) {
      return OT.Raptor.Message.streams.channelFromOTChannel(channel);
    })
  };

  if (minBitrate) messageContent.minBitrate = minBitrate;
  if (maxBitrate) messageContent.maxBitrate = maxBitrate;

  return OT.Raptor.serializeMessage({
    method: 'create',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/stream/' + streamId,
    content: messageContent
  });
};

OT.Raptor.Message.streams.destroy = function (apiKey, sessionId, streamId) {
  return OT.Raptor.serializeMessage({
    method: 'delete',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/stream/' + streamId,
    content: {}
  });
};


OT.Raptor.Message.streams.answer = function (apiKey, sessionId, streamId, answerSdp) {
  return OT.Raptor.serializeMessage({
    method: 'answer',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/stream/' + streamId,
    content: {
      sdp: answerSdp
    }
  });
};

OT.Raptor.Message.streams.candidate = function (apiKey, sessionId, streamId, candidate) {
  return OT.Raptor.serializeMessage({
    method: 'candidate',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/stream/' + streamId,
    content: candidate
  });
};

OT.Raptor.Message.streamChannels = {};
OT.Raptor.Message.streamChannels.update =
  function (apiKey, sessionId, streamId, channelId, attributes) {
  return OT.Raptor.serializeMessage({
    method: 'update',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId + '/stream/' +
      streamId + '/channel/' + channelId,
    content: attributes
  });
};


OT.Raptor.Message.subscribers = {};

OT.Raptor.Message.subscribers.create =
  function (apiKey, sessionId, streamId, subscriberId, connectionId, channelsToSubscribeTo) {
  var content = {
    id: subscriberId,
    connection: connectionId,
    keyManagementMethod: OT.$.supportedCryptoScheme(),
    bundleSupport: OT.$.hasCapabilities('bundle'),
    rtcpMuxSupport: OT.$.hasCapabilities('RTCPMux')
  };
  if (channelsToSubscribeTo) content.channel = channelsToSubscribeTo;

  return OT.Raptor.serializeMessage({
    method: 'create',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId +
      '/stream/' + streamId + '/subscriber/' + subscriberId,
    content: content
  });
};

OT.Raptor.Message.subscribers.destroy = function (apiKey, sessionId, streamId, subscriberId) {
  return OT.Raptor.serializeMessage({
    method: 'delete',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId +
      '/stream/' + streamId + '/subscriber/' + subscriberId,
    content: {}
  });
};

OT.Raptor.Message.subscribers.update =
  function (apiKey, sessionId, streamId, subscriberId, attributes) {
  return OT.Raptor.serializeMessage({
    method: 'update',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId +
    '/stream/' + streamId + '/subscriber/' + subscriberId,
    content: attributes
  });
};


OT.Raptor.Message.subscribers.candidate =
  function (apiKey, sessionId, streamId, subscriberId, candidate) {
  return OT.Raptor.serializeMessage({
    method: 'candidate',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId +
      '/stream/' + streamId + '/subscriber/' + subscriberId,
    content: candidate
  });
};


OT.Raptor.Message.subscribers.answer =
  function (apiKey, sessionId, streamId, subscriberId, answerSdp) {
  return OT.Raptor.serializeMessage({
    method: 'answer',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId +
    '/stream/' + streamId + '/subscriber/' + subscriberId,
    content: {
      sdp: answerSdp
    }
  });
};


OT.Raptor.Message.subscriberChannels = {};

OT.Raptor.Message.subscriberChannels.update =
  function (apiKey, sessionId, streamId, subscriberId, channelId, attributes) {
  return OT.Raptor.serializeMessage({
    method: 'update',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId +
    '/stream/' + streamId + '/subscriber/' + subscriberId + '/channel/' + channelId,
    content: attributes
  });
};


OT.Raptor.Message.signals = {};

OT.Raptor.Message.signals.create = function (apiKey, sessionId, toAddress, type, data) {
  var content = {};
  if (type !== void 0) content.type = type;
  if (data !== void 0) content.data = data;

  return OT.Raptor.serializeMessage({
    method: 'signal',
    uri: '/v2/partner/' + apiKey + '/session/' + sessionId +
      (toAddress !== void 0 ? '/connection/' + toAddress : '') + '/signal/' + OT.$.uuid(),
    content: content
  });
};





!(function() {
  

  

  
  var connectErrorReasons;

  connectErrorReasons = {
    409: 'This P2P session already has 2 participants.',
    410: 'The session already has four participants.',
    1004: 'The token passed is invalid.'
  };


  OT.Raptor.Dispatcher = function () {

    if(OT.$.env.name === 'Node') {
      EventEmitter.call(this);
    } else {
      OT.$.eventing(this, true);
      this.emit = this.trigger;
    }

    this.callbacks = {};
  };

  if(OT.$.env.name === 'Node') {
    util.inherits(OT.Raptor.Dispatcher, EventEmitter);
  }

  OT.Raptor.Dispatcher.prototype.registerCallback = function (transactionId, completion) {
    this.callbacks[transactionId] = completion;
  };

  OT.Raptor.Dispatcher.prototype.triggerCallback = function (transactionId) {
    
    if (!transactionId) return;

    var completion = this.callbacks[transactionId];

    if (completion && OT.$.isFunction(completion)) {
      var args = Array.prototype.slice.call(arguments);
      args.shift();

      completion.apply(null, args);
    }

    delete this.callbacks[transactionId];
  };

  OT.Raptor.Dispatcher.prototype.onClose = function(reason) {
    this.emit('close', reason);
  };


  OT.Raptor.Dispatcher.prototype.dispatch = function(rumorMessage) {
    
    

    if (rumorMessage.type === OT.Rumor.MessageType.STATUS) {
      OT.debug('OT.Raptor.dispatch: STATUS');
      OT.debug(rumorMessage);

      var error;

      if (rumorMessage.isError) {
        error = new OT.Error(rumorMessage.status);
      }

      this.triggerCallback(rumorMessage.transactionId, error, rumorMessage);

      return;
    }

    var message = OT.Raptor.unboxFromRumorMessage(rumorMessage);
    OT.debug('OT.Raptor.dispatch ' + message.signature);
    OT.debug(rumorMessage.data);

    switch(message.resource) {
      case 'session':
        this.dispatchSession(message);
        break;

      case 'connection':
        this.dispatchConnection(message);
        break;

      case 'stream':
        this.dispatchStream(message);
        break;

      case 'stream_channel':
        this.dispatchStreamChannel(message);
        break;

      case 'subscriber':
        this.dispatchSubscriber(message);
        break;

      case 'subscriber_channel':
        this.dispatchSubscriberChannel(message);
        break;

      case 'signal':
        this.dispatchSignal(message);
        break;

      case 'archive':
        this.dispatchArchive(message);
        break;

      default:
        OT.warn('OT.Raptor.dispatch: Type ' + message.resource + ' is not currently implemented');
    }
  };

  OT.Raptor.Dispatcher.prototype.dispatchSession = function (message) {
    switch (message.method) {
      case 'read':
        this.emit('session#read', message.content, message.transactionId);
        break;


      default:
        OT.warn('OT.Raptor.dispatch: ' + message.signature + ' is not currently implemented');
    }
  };

  OT.Raptor.Dispatcher.prototype.dispatchConnection = function (message) {

    switch (message.method) {
      case 'created':
        this.emit('connection#created', message.content);
        break;


      case 'deleted':
        this.emit('connection#deleted', message.params.connection, message.reason);
        break;

      default:
        OT.warn('OT.Raptor.dispatch: ' + message.signature + ' is not currently implemented');
    }
  };

  OT.Raptor.Dispatcher.prototype.dispatchStream = function (message) {

    switch (message.method) {
      case 'created':
        this.emit('stream#created', message.content, message.transactionId);
        break;

      case 'deleted':
        this.emit('stream#deleted', message.params.stream,
          message.reason);
        break;


      case 'updated':
        this.emit('stream#updated', message.params.stream,
          message.content);
        break;


      
      case 'generateoffer':
      case 'answer':
      case 'pranswer':
      case 'offer':
      case 'candidate':
        this.dispatchJsep(message.method, message);
        break;

      default:
        OT.warn('OT.Raptor.dispatch: ' + message.signature + ' is not currently implemented');
    }
  };

  OT.Raptor.Dispatcher.prototype.dispatchStreamChannel = function (message) {
    switch (message.method) {
      case 'updated':
        this.emit('streamChannel#updated', message.params.stream,
          message.params.channel, message.content);
        break;

      default:
        OT.warn('OT.Raptor.dispatch: ' + message.signature + ' is not currently implemented');
    }
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  OT.Raptor.Dispatcher.prototype.dispatchJsep = function (method, message) {
    this.emit('jsep#' + method, message.params.stream, message.fromAddress, message);
  };


  OT.Raptor.Dispatcher.prototype.dispatchSubscriberChannel = function (message) {
    switch (message.method) {
      case 'updated':
        this.emit('subscriberChannel#updated', message.params.stream,
          message.params.channel, message.content);
        break;


      case 'update': 
        this.emit('subscriberChannel#update', message.params.subscriber,
          message.params.stream, message.content);
        break;


      default:
        OT.warn('OT.Raptor.dispatch: ' + message.signature + ' is not currently implemented');
    }
  };

  OT.Raptor.Dispatcher.prototype.dispatchSubscriber = function (message) {
    switch (message.method) {
      case 'created':
        this.emit('subscriber#created', message.params.stream, message.fromAddress,
          message.content.id);
        break;


      case 'deleted':
        this.dispatchJsep('unsubscribe', message);
        this.emit('subscriber#deleted', message.params.stream,
          message.fromAddress);
        break;


      
      case 'generateoffer':
      case 'answer':
      case 'pranswer':
      case 'offer':
      case 'candidate':
        this.dispatchJsep(message.method, message);
        break;


      default:
        OT.warn('OT.Raptor.dispatch: ' + message.signature + ' is not currently implemented');
    }
  };

  OT.Raptor.Dispatcher.prototype.dispatchSignal = function (message) {
    if (message.method !== 'signal') {
      OT.warn('OT.Raptor.dispatch: ' + message.signature + ' is not currently implemented');
      return;
    }
    this.emit('signal', message.fromAddress, message.content.type,
      message.content.data);
  };

  OT.Raptor.Dispatcher.prototype.dispatchArchive = function (message) {
    switch (message.method) {
      case 'created':
        this.emit('archive#created', message.content);
        break;

      case 'updated':
        this.emit('archive#updated', message.params.archive, message.content);
        break;
    }
  };

}(this));









(function(window) {

  
  OT.publishers = new OT.$.Collection('guid');          
  OT.subscribers = new OT.$.Collection('widgetId');     
  OT.sessions = new OT.$.Collection();

  function parseStream(dict, session) {
    var channel = dict.channel.map(function(channel) {
      return new OT.StreamChannel(channel);
    });

    var connectionId = dict.connectionId ? dict.connectionId : dict.connection.id;

    return  new OT.Stream(  dict.id,
                            dict.name,
                            dict.creationTime,
                            session.connections.get(connectionId),
                            session,
                            channel );
  }

  function parseAndAddStreamToSession(dict, session) {
    if (session.streams.has(dict.id)) return;

    var stream = parseStream(dict, session);
    session.streams.add( stream );

    return stream;
  }

  function parseArchive(dict) {
    return new OT.Archive( dict.id,
                           dict.name,
                           dict.status );
  }

  function parseAndAddArchiveToSession(dict, session) {
    if (session.archives.has(dict.id)) return;

    var archive = parseArchive(dict);
    session.archives.add(archive);

    return archive;
  }

  var DelayedEventQueue = function DelayedEventQueue (eventDispatcher) {
    var queue = [];

    this.enqueue = function enqueue () {
      queue.push( Array.prototype.slice.call(arguments) );
    };

    this.triggerAll = function triggerAll () {
      var event;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      while( (event = queue.shift()) ) {
        eventDispatcher.trigger.apply(eventDispatcher, event);
      }
    };
  };

  var DelayedSessionEvents = function(dispatcher) {
    var eventQueues = {};

    this.enqueue = function enqueue () {
      var key = arguments[0];
      var eventArgs = Array.prototype.slice.call(arguments, 1);
      if (!eventQueues[key]) {
        eventQueues[key] = new DelayedEventQueue(dispatcher);
      }
      eventQueues[key].enqueue.apply(eventQueues[key], eventArgs);
    };

    this.triggerConnectionCreated = function triggerConnectionCreated (connection) {
      if (eventQueues['connectionCreated' + connection.id]) {
        eventQueues['connectionCreated' + connection.id].triggerAll();
      }
    };

    this.triggerSessionConnected = function triggerSessionConnected (connections) {
      if (eventQueues.sessionConnected) {
        eventQueues.sessionConnected.triggerAll();
      }

      OT.$.forEach(connections, function(connection) {
        this.triggerConnectionCreated(connection);
      });
    };
  };

  var unconnectedStreams = {};

  window.OT.SessionDispatcher = function(session) {

    var dispatcher = new OT.Raptor.Dispatcher(),
        sessionStateReceived = false,
        delayedSessionEvents = new DelayedSessionEvents(dispatcher);

    dispatcher.on('close', function(reason) {

      var connection = session.connection;

      if (!connection) {
        return;
      }

      if (connection.destroyedReason()) {
        OT.debug('OT.Raptor.Socket: Socket was closed but the connection had already ' +
          'been destroyed. Reason: ' + connection.destroyedReason());
        return;
      }

      connection.destroy( reason );
    });
    
    
    
    
    var addConnection = function (connection, sessionRead) {
      connection = OT.Connection.fromHash(connection);
      if (sessionRead || session.connection && connection.id !== session.connection.id) {
        session.connections.add( connection );
        delayedSessionEvents.triggerConnectionCreated(connection);
      }

      OT.$.forEach(OT.$.keys(unconnectedStreams), function(streamId) {
        var stream = unconnectedStreams[streamId];
        if (stream && connection.id === stream.connection.id) {
          
          parseAndAddStreamToSession(stream, session);
          delete unconnectedStreams[stream.id];
          
          var payload = {
            debug: sessionRead ? 'connection came in session#read' :
              'connection came in connection#created',
            streamId : stream.id,
            connectionId : connection.id
          };
          session.logEvent('streamCreated', 'warning', payload);
        }
      });
      
      return connection;
    };

    dispatcher.on('session#read', function(content, transactionId) {

      var state = {},
          connection;

      state.streams = [];
      state.connections = [];
      state.archives = [];

      OT.$.forEach(content.connection, function(connectionParams) {
        connection = addConnection(connectionParams, true);
        state.connections.push(connection);
      });

      OT.$.forEach(content.stream, function(streamParams) {
        state.streams.push( parseAndAddStreamToSession(streamParams, session) );
      });

      OT.$.forEach(content.archive || content.archives, function(archiveParams) {
        state.archives.push( parseAndAddArchiveToSession(archiveParams, session) );
      });

      session._.subscriberMap = {};

      dispatcher.triggerCallback(transactionId, null, state);

      sessionStateReceived = true;
      delayedSessionEvents.triggerSessionConnected(session.connections);
    });

    dispatcher.on('connection#created', function(connection) {
      addConnection(connection);
    });

    dispatcher.on('connection#deleted', function(connection, reason) {
      connection = session.connections.get(connection);
      connection.destroy(reason);
    });

    dispatcher.on('stream#created', function(stream, transactionId) {
      var connectionId = stream.connectionId ? stream.connectionId : stream.connection.id;
      if (session.connections.has(connectionId)) {
        stream = parseAndAddStreamToSession(stream, session);
      } else {
        unconnectedStreams[stream.id] = stream;

        var payload = {
          debug : 'eventOrderError -- streamCreated event before connectionCreated',
          streamId : stream.id,
        };
        session.logEvent('streamCreated', 'warning', payload);
      }

      if (stream.publisher) {
        stream.publisher.setStream(stream);
      }

      dispatcher.triggerCallback(transactionId, null, stream);
    });

    dispatcher.on('stream#deleted', function(streamId, reason) {
      var stream = session.streams.get(streamId);

      if (!stream) {
        OT.error('OT.Raptor.dispatch: A stream does not exist with the id of ' +
          streamId + ', for stream#deleted message!');
        
        return;
      }

      stream.destroy(reason);
    });

    dispatcher.on('stream#updated', function(streamId, content) {
      var stream = session.streams.get(streamId);

      if (!stream) {
        OT.error('OT.Raptor.dispatch: A stream does not exist with the id of ' +
          streamId + ', for stream#updated message!');
        
        return;
      }

      stream._.update(content);

    });

    dispatcher.on('streamChannel#updated', function(streamId, channelId, content) {
      var stream;
      if (!(streamId && (stream = session.streams.get(streamId)))) {
        OT.error('OT.Raptor.dispatch: Unable to determine streamId, or the stream does not ' +
          'exist, for streamChannel message!');
        
        return;
      }
      stream._.updateChannel(channelId, content);
    });

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var jsepHandler = function(method, streamId, fromAddress, message) {

      var fromConnection,
          actors;

      switch (method) {
        
        case 'offer':
          actors = [];
          var subscriber = OT.subscribers.find({streamId: streamId});
          if (subscriber) actors.push(subscriber);
          break;


        
        case 'answer':
        case 'pranswer':
        case 'generateoffer':
        case 'unsubscribe':
          actors = OT.publishers.where({streamId: streamId});
          break;


        
        case 'candidate':
          
          
          actors = OT.publishers.where({streamId: streamId})
            .concat(OT.subscribers.where({streamId: streamId}));
          break;


        default:
          OT.warn('OT.Raptor.dispatch: jsep#' + method +
            ' is not currently implemented');
          return;
      }

      if (actors.length === 0) return;

      
      
      
      fromConnection = actors[0].session.connections.get(fromAddress);
      if(!fromConnection && fromAddress.match(/^symphony\./)) {
        fromConnection = OT.Connection.fromHash({
          id: fromAddress,
          creationTime: Math.floor(OT.$.now())
        });

        actors[0].session.connections.add(fromConnection);
      } else if(!fromConnection) {
        OT.warn('OT.Raptor.dispatch: Messsage comes from a connection (' +
          fromAddress + ') that we do not know about. The message was ignored.');
        return;
      }

      OT.$.forEach(actors, function(actor) {
        actor.processMessage(method, fromConnection, message);
      });
    };

    dispatcher.on('jsep#offer', OT.$.bind(jsepHandler, null, 'offer'));
    dispatcher.on('jsep#answer', OT.$.bind(jsepHandler, null, 'answer'));
    dispatcher.on('jsep#pranswer', OT.$.bind(jsepHandler, null, 'pranswer'));
    dispatcher.on('jsep#generateoffer', OT.$.bind(jsepHandler, null, 'generateoffer'));
    dispatcher.on('jsep#unsubscribe', OT.$.bind(jsepHandler, null, 'unsubscribe'));
    dispatcher.on('jsep#candidate', OT.$.bind(jsepHandler, null, 'candidate'));

    dispatcher.on('subscriberChannel#updated', function(streamId, channelId, content) {

      if (!streamId || !session.streams.has(streamId)) {
        OT.error('OT.Raptor.dispatch: Unable to determine streamId, or the stream does not ' +
          'exist, for subscriberChannel#updated message!');
        
        return;
      }

      session.streams.get(streamId)._
        .updateChannel(channelId, content);

    });

    dispatcher.on('subscriberChannel#update', function(subscriberId, streamId, content) {

      if (!streamId || !session.streams.has(streamId)) {
        OT.error('OT.Raptor.dispatch: Unable to determine streamId, or the stream does not ' +
          'exist, for subscriberChannel#update message!');
        
        return;
      }

      
      if (!OT.subscribers.has(subscriberId)) {
        OT.error('OT.Raptor.dispatch: Unable to determine subscriberId, or the subscriber ' +
          'does not exist, for subscriberChannel#update message!');
        
        return;
      }

      
      
      OT.subscribers.get(subscriberId).disableVideo(content.active);

    });

    dispatcher.on('subscriber#created', function(streamId, fromAddress, subscriberId) {

      var stream = streamId ? session.streams.get(streamId) : null;

      if (!stream) {
        OT.error('OT.Raptor.dispatch: Unable to determine streamId, or the stream does ' +
          'not exist, for subscriber#created message!');
        
        return;
      }

      session._.subscriberMap[fromAddress + '_' + stream.id] = subscriberId;
    });

    dispatcher.on('subscriber#deleted', function(streamId, fromAddress) {
      var stream = streamId ? session.streams.get(streamId) : null;

      if (!stream) {
        OT.error('OT.Raptor.dispatch: Unable to determine streamId, or the stream does ' +
          'not exist, for subscriber#created message!');
        
        return;
      }

      delete session._.subscriberMap[fromAddress + '_' + stream.id];
    });

    dispatcher.on('signal', function(fromAddress, signalType, data) {
      var fromConnection = session.connections.get(fromAddress);
      if (session.connection && fromAddress === session.connection.connectionId) {
        if (sessionStateReceived) {
          session._.dispatchSignal(fromConnection, signalType, data);
        } else {
          delayedSessionEvents.enqueue('sessionConnected',
            'signal', fromAddress, signalType, data);
        }
      } else {
        if (session.connections.get(fromAddress)) {
          session._.dispatchSignal(fromConnection, signalType, data);
        } else {
          delayedSessionEvents.enqueue('connectionCreated' + fromAddress,
            'signal', fromAddress, signalType, data);
        }
      }
    });

    dispatcher.on('archive#created', function(archive) {
      parseAndAddArchiveToSession(archive, session);
    });

    dispatcher.on('archive#updated', function(archiveId, update) {
      var archive = session.archives.get(archiveId);

      if (!archive) {
        OT.error('OT.Raptor.dispatch: An archive does not exist with the id of ' +
          archiveId + ', for archive#updated message!');
        
        return;
      }

      archive._.update(update);
    });

    return dispatcher;

  };

})(window);





function httpTest(config) {

  function otRequest(url, options, callback) {
    var request = new XMLHttpRequest(),
        _options = options || {},
        _method = _options.method;

    if (!_method) {
      callback(new OT.$.Error('No HTTP method specified in options'));
      return;
    }

    
    
    
    if (callback) {
      OTHelpers.on(request, 'load', function(event) {
        var status = event.target.status;

        
        
        if (status >= 200 && (status < 300 || status === 304)) {
          callback(null, event);
        } else {
          callback(event);
        }
      });

      OTHelpers.on(request, 'error', callback);
    }

    request.open(options.method, url, true);

    if (!_options.headers) _options.headers = {};

    for (var name in _options.headers) {
      request.setRequestHeader(name, _options.headers[name]);
    }

    return request;
  }


  var _httpConfig = config.httpConfig;

  function timeout(delay) {
    return new Promise(function(resolve) {
      setTimeout(function() {
        resolve();
      }, delay);
    });
  }

  function generateRandom10DigitNumber() {
    var min = 1000000000;
    var max = 9999999999;
    return min + Math.floor(Math.random() * (max - min));
  }

  




  function doDownload() {

    var xhr;
    var startTs;
    var loadedLength = 0;
    var downloadPromise = new Promise(function(resolve, reject) {
      xhr = otRequest([_httpConfig.downloadUrl, '?x=', generateRandom10DigitNumber()].join(''),
        {method: 'get'}, function(error) {
        if (error) {
          reject(new OT.$.Error('Connection to the HTTP server failed (' +
          error.target.status + ')', 1006));
        } else {
          resolve();
        }
      });

      xhr.addEventListener('loadstart', function() {
        startTs = OT.$.now();
      });
      xhr.addEventListener('progress', function(evt) {
        loadedLength = evt.loaded;
      });

      xhr.send();
    });

    return Promise.race([
      downloadPromise,
      timeout(_httpConfig.duration * 1000)
    ])
      .then(function() {
        xhr.abort();
        return {
          byteDownloaded: loadedLength,
          duration: OT.$.now() - startTs
        };
      });
  }

  function doUpload() {
    var payload = new Array(_httpConfig.uploadSize * _httpConfig.uploadCount).join('a');

    var xhr;
    var startTs;
    var loadedLength = 0;
    var uploadPromise = new Promise(function(resolve, reject) {
      xhr = otRequest(_httpConfig.uploadUrl, {method: 'post'}, function(error) {
        if (error) {
          reject(new OT.$.Error('Connection to the HTTP server failed (' +
          error.target.status + ')', 1006));
        } else {
          resolve();
        }
      });

      xhr.upload.addEventListener('loadstart', function() {
        startTs = OT.$.now();
      });
      xhr.upload.addEventListener('progress', function(evt) {
        loadedLength = evt.loaded;
      });

      xhr.send(payload);
    });

    return Promise.race([
      uploadPromise,
      timeout(_httpConfig.duration * 1000)
    ])
      .then(function() {
        xhr.abort();
        return {
          byteUploaded: loadedLength,
          duration: OT.$.now() - startTs
        };
      });
  }

  return Promise.all([doDownload(), doUpload()])
    .then(function(values) {
      var downloadStats = values[0];
      var uploadStats = values[1];

      return {
        downloadBandwidth: 1000 * (downloadStats.byteDownloaded * 8) / downloadStats.duration,
        uploadBandwidth: 1000 * (uploadStats.byteUploaded * 8) / uploadStats.duration
      };
    });
}

OT.httpTest = httpTest;















var SDPHelpers = {
  
  getMLineIndex: function getMLineIndex(sdpLines, mediaType) {
    var targetMLine = 'm=' + mediaType;

    
    return OT.$.findIndex(sdpLines, function(line) {
      if (line.indexOf(targetMLine) !== -1) {
        return true;
      }

      return false;
    });
  },

  
  
  getMLinePayloadTypes: function getMLinePayloadTypes (mediaLine, mediaType) {
    var mLineSelector = new RegExp('^m=' + mediaType +
                          ' \\d+(/\\d+)? [a-zA-Z0-9/]+(( [a-zA-Z0-9/]+)+)$', 'i');

    
    var payloadTypes = mediaLine.match(mLineSelector);
    if (!payloadTypes || payloadTypes.length < 2) {
      
      return [];
    }

    return OT.$.trim(payloadTypes[2]).split(' ');
  },

  removeTypesFromMLine: function removeTypesFromMLine (mediaLine, payloadTypes) {
    return OT.$.trim(
              mediaLine.replace(new RegExp(' ' + payloadTypes.join(' |'), 'ig') , ' ')
                       .replace(/\s+/g, ' ') );
  },


  
  
  removeMediaEncoding: function removeMediaEncoding (sdp, mediaType, encodingName) {
    var sdpLines = sdp.split('\r\n'),
        mLineIndex = SDPHelpers.getMLineIndex(sdpLines, mediaType),
        mLine = mLineIndex > -1 ? sdpLines[mLineIndex] : void 0,
        typesToRemove = [],
        payloadTypes,
        match;

    if (mLineIndex === -1) {
      
      return sdpLines.join('\r\n');
    }

    
    payloadTypes = SDPHelpers.getMLinePayloadTypes(mLine, mediaType);
    if (payloadTypes.length === 0) {
      
      return sdpLines.join('\r\n');
    }

    
    
    var matcher = new RegExp('a=rtpmap:(' + payloadTypes.join('|') + ') ' +
                                          encodingName + '\\/\\d+', 'i');

    sdpLines = OT.$.filter(sdpLines, function(line, index) {
      match = line.match(matcher);
      if (match === null) return true;

      typesToRemove.push(match[1]);

      if (index < mLineIndex) {
        
        mLineIndex--;
      }

      
      return false;
    });

    if (typesToRemove.length > 0 && mLineIndex > -1) {
      
      sdpLines[mLineIndex] = SDPHelpers.removeTypesFromMLine(mLine, typesToRemove);
    }

    return sdpLines.join('\r\n');
  },

  
  
  
  
  removeComfortNoise: function removeComfortNoise (sdp) {
    return SDPHelpers.removeMediaEncoding(sdp, 'audio', 'CN');
  },

  removeVideoCodec: function removeVideoCodec (sdp, codec) {
    return SDPHelpers.removeMediaEncoding(sdp, 'video', codec);
  }
};





function isVideoStat(stat) {
  
  return stat.hasOwnProperty('googFrameWidthReceived') ||
    stat.hasOwnProperty('googFrameWidthInput') ||
    stat.mediaType === 'video';
}

function isAudioStat(stat) {
  
  return stat.hasOwnProperty('audioInputLevel') ||
    stat.hasOwnProperty('audioOutputLevel') ||
    stat.mediaType === 'audio';
}

function isInboundStat(stat) {
  return stat.hasOwnProperty('bytesReceived');
}

function parseStatCategory(stat) {
  var statCategory = {
    packetsLost: 0,
    packetsReceived: 0,
    bytesReceived: 0
  };

  if (stat.hasOwnProperty('packetsReceived')) {
    statCategory.packetsReceived = parseInt(stat.packetsReceived, 10);
  }
  if (stat.hasOwnProperty('packetsLost')) {
    statCategory.packetsLost = parseInt(stat.packetsLost, 10);
  }
  if (stat.hasOwnProperty('bytesReceived')) {
    statCategory.bytesReceived += parseInt(stat.bytesReceived, 10);
  }

  return statCategory;
}

function normalizeTimestamp(timestamp) {
  if (OT.$.isObject(timestamp) && 'getTime' in timestamp) {
    
    
    return timestamp.getTime();
  } else {
    return timestamp;
  }
}

var getStatsHelpers = {};
getStatsHelpers.isVideoStat = isVideoStat;
getStatsHelpers.isAudioStat = isAudioStat;
getStatsHelpers.isInboundStat = isInboundStat;
getStatsHelpers.parseStatCategory = parseStatCategory;
getStatsHelpers.normalizeTimestamp = normalizeTimestamp;

OT.getStatsHelpers = getStatsHelpers;








function getStatsAdapter() {

  



  function getStatsOldAPI(peerConnection, completion) {

    peerConnection.getStats(function(rtcStatsReport) {

      var stats = [];
      rtcStatsReport.result().forEach(function(rtcStat) {

        var stat = {};

        rtcStat.names().forEach(function(name) {
          stat[name] = rtcStat.stat(name);
        });

        
        stat.id = rtcStat.id;
        stat.type = rtcStat.type;
        stat.timestamp = rtcStat.timestamp;
        stats.push(stat);
      });

      completion(null, stats);
    });
  }




  function getStatsNewAPI(peerConnection, completion) {

    peerConnection.getStats(null, function(rtcStatsReport) {

      var stats = [];
      rtcStatsReport.forEach(function(rtcStats) {
        stats.push(rtcStats);
      });

      completion(null, stats);
    }, completion);
  }

  if (OT.$.browserVersion().name === 'Firefox' || OTPlugin.isInstalled()) {
    return getStatsNewAPI;
  } else {
    return getStatsOldAPI;
  }
}

OT.getStatsAdpater = getStatsAdapter;










function webrtcTest(config) {

  var _getStats = OT.getStatsAdpater();
  var _mediaConfig = config.mediaConfig;
  var _localStream = config.localStream;

  
  
  var NativeRTCSessionDescription,
      NativeRTCIceCandidate;
  if (!OTPlugin.isInstalled()) {
    
    NativeRTCSessionDescription = (window.mozRTCSessionDescription ||
    window.RTCSessionDescription);
    NativeRTCIceCandidate = (window.mozRTCIceCandidate || window.RTCIceCandidate);
  }
  else {
    NativeRTCSessionDescription = OTPlugin.RTCSessionDescription;
    NativeRTCIceCandidate = OTPlugin.RTCIceCandidate;
  }


  function isCandidateRelay(candidate) {
    return candidate.candidate.indexOf('relay') !== -1;
  }

  




  function createVideoElementForTest() {
    var videoElement = new OT.VideoElement({attributes: {muted: true}});
    videoElement.domElement().style.position = 'absolute';
    videoElement.domElement().style.top = '-9999%';
    videoElement.appendTo(document.body);
    return videoElement;
  }

  function createPeerConnectionForTest() {
    return new Promise(function(resolve, reject) {
      OT.$.createPeerConnection({
          iceServers: _mediaConfig.iceServers
        }, {},
        null,
        function(error, pc) {
          if (error) {
            reject(new OT.$.Error('createPeerConnection failed', 1600, error));
          } else {
            resolve(pc);
          }
        }
      );
    });
  }

  function createOffer(pc) {
    return new Promise(function(resolve, reject) {
      pc.createOffer(resolve, reject);
    });
  }

  function attachMediaStream(videoElement, webRtcStream) {
    return new Promise(function(resolve, reject) {
      videoElement.bindToStream(webRtcStream, function(error) {
        if (error) {
          reject(new OT.$.Error('bindToStream failed', 1600, error));
        } else {
          resolve();
        }
      });
    });
  }

  function addIceCandidate(pc, candidate) {
    return new Promise(function(resolve, reject) {
      pc.addIceCandidate(new NativeRTCIceCandidate({
        sdpMLineIndex: candidate.sdpMLineIndex,
        candidate: candidate.candidate
      }), resolve, reject);
    });
  }

  function setLocalDescription(pc, offer) {
    return new Promise(function(resolve, reject) {
      pc.setLocalDescription(offer, resolve, function(error) {
        reject(new OT.$.Error('setLocalDescription failed', 1600, error));
      });
    });
  }

  function setRemoteDescription(pc, offer) {
    return new Promise(function(resolve, reject) {
      pc.setRemoteDescription(offer, resolve, function(error) {
        reject(new OT.$.Error('setRemoteDescription failed', 1600, error));
      });
    });
  }

  function createAnswer(pc) {
    return new Promise(function(resolve, reject) {
      pc.createAnswer(resolve, function(error) {
        reject(new OT.$.Error('createAnswer failed', 1600, error));
      });
    });
  }

  function getStats(pc) {
    return new Promise(function(resolve, reject) {
      _getStats(pc, function(error, stats) {
        if (error) {
          reject(new OT.$.Error('geStats failed', 1600, error));
        } else {
          resolve(stats);
        }
      });
    });
  }

  function createOnIceCandidateListener(pc) {
    return function(event) {
      if (event.candidate && isCandidateRelay(event.candidate)) {
        addIceCandidate(pc, event.candidate)['catch'](function() {
          OT.warn('An error occurred while adding a ICE candidate during webrtc test');
        });
      }
    };
  }

  


  function collectPeerConnectionStats(localPc, remotePc) {

    var SAMPLING_DELAY = 1000;

    return new Promise(function(resolve) {

      var collectionActive = true;

      var statsSamples = {
        startTs: OT.$.now(),
        packetLostRatioSamplesCount: 0,
        packetLostRatio: 0,
        roundTripTimeSamplesCount: 0,
        roundTripTime: 0,
        bytesReceived: 0
      };

      function calculateBandwidth() {
        return 1000 * statsSamples.bytesReceived * 8 / (OT.$.now() - statsSamples.startTs);
      }

      function sample() {

        Promise.all([
          getStats(localPc).then(function(stats) {
            OT.$.forEach(stats, function(stat) {
              if (OT.getStatsHelpers.isVideoStat(stat)) {
                var rtt = null;

                if (stat.hasOwnProperty('googRtt')) {
                  rtt = parseInt(stat.googRtt, 10);
                } else if (stat.hasOwnProperty('mozRtt')) {
                  rtt = stat.mozRtt;
                }

                if (rtt !== null && rtt > -1) {
                  statsSamples.roundTripTimeSamplesCount++;
                  statsSamples.roundTripTime += rtt;
                }
              }
            });
          }),

          getStats(remotePc).then(function(stats) {
            OT.$.forEach(stats, function(stat) {
              if (OT.getStatsHelpers.isVideoStat(stat)) {
                if (stat.hasOwnProperty('packetsReceived') &&
                  stat.hasOwnProperty('packetsLost')) {

                  var packetLost = parseInt(stat.packetsLost, 10);
                  var packetsReceived = parseInt(stat.packetsReceived, 10);
                  if (packetLost >= 0 && packetsReceived > 0) {
                    statsSamples.packetLostRatioSamplesCount++;
                    statsSamples.packetLostRatio += packetLost * 100 / packetsReceived;
                  }
                }

                if (stat.hasOwnProperty('bytesReceived')) {
                  statsSamples.bytesReceived += parseInt(stat.bytesReceived, 10);
                }
              }
            });
          })
        ])
          .then(function() {
            
            setTimeout(function() {
              if (collectionActive) {
                sample();
              }
            }, SAMPLING_DELAY);
          });
      }

      
      sample();

      function stopCollectStats() {
        collectionActive = false;

        var pcStats = {
          packetLostRatio: statsSamples.packetLostRatioSamplesCount > 0 ?
            statsSamples.packetLostRatio /= statsSamples.packetLostRatioSamplesCount * 100 : null,
          roundTripTime: statsSamples.roundTripTimeSamplesCount > 0 ?
            statsSamples.roundTripTime /= statsSamples.roundTripTimeSamplesCount : null,
          bandwidth: calculateBandwidth()
        };

        resolve(pcStats);
      }

      
      
      setTimeout(function() {

        if (calculateBandwidth() < _mediaConfig.thresholdBitsPerSecond) {
          
          setTimeout(stopCollectStats, _mediaConfig.extendedDuration * 1000);
        } else {
          stopCollectStats();
        }

      }, _mediaConfig.duration * 1000);
    });
  }

  return Promise
    .all([createPeerConnectionForTest(), createPeerConnectionForTest()])
    .then(function(pcs) {

      var localPc = pcs[0],
          remotePc = pcs[1];

      var localVideo = createVideoElementForTest(),
          remoteVideo = createVideoElementForTest();

      attachMediaStream(localVideo, _localStream);
      localPc.addStream(_localStream);

      var remoteStream;
      remotePc.onaddstream = function(evt) {
        remoteStream = evt.stream;
        attachMediaStream(remoteVideo, remoteStream);
      };

      localPc.onicecandidate = createOnIceCandidateListener(remotePc);
      remotePc.onicecandidate = createOnIceCandidateListener(localPc);

      function dispose() {
        localVideo.destroy();
        remoteVideo.destroy();
        localPc.close();
        remotePc.close();
      }

      return createOffer(localPc)
        .then(function(offer) {
          return Promise.all([
            setLocalDescription(localPc, offer),
            setRemoteDescription(remotePc, offer)
          ]);
        })
        .then(function() {
          return createAnswer(remotePc);
        })
        .then(function(answer) {
          return Promise.all([
            setLocalDescription(remotePc, answer),
            setRemoteDescription(localPc, answer)
          ]);
        })
        .then(function() {
          return collectPeerConnectionStats(localPc, remotePc);
        })
        .then(function(value) {
          dispose();
          return value;
        }, function(error) {
          dispose();
          throw error;
        });
    });
}

OT.webrtcTest = webrtcTest;








OT.Chrome = function(properties) {
  var _visible = false,
      _widgets = {},

      
      _set = function(name, widget) {
        widget.parent = this;
        widget.appendTo(properties.parent);

        _widgets[name] = widget;

        this[name] = widget;
      };

  if (!properties.parent) {
    
    return;
  }

  OT.$.eventing(this);

  this.destroy = function() {
    this.off();
    this.hideWhileLoading();

    for (var name in _widgets) {
      _widgets[name].destroy();
    }
  };

  this.showAfterLoading = function() {
    _visible = true;

    for (var name in _widgets) {
      _widgets[name].showAfterLoading();
    }
  };

  this.hideWhileLoading = function() {
    _visible = false;

    for (var name in _widgets) {
      _widgets[name].hideWhileLoading();
    }
  };


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  this.set = function(widgetName, widget) {
    if (typeof(widgetName) === 'string' && widget) {
      _set.call(this, widgetName, widget);

    } else {
      for (var name in widgetName) {
        if (widgetName.hasOwnProperty(name)) {
          _set.call(this, name, widgetName[name]);
        }
      }
    }
    return this;
  };

};









if (!OT.Chrome.Behaviour) OT.Chrome.Behaviour = {};





OT.Chrome.Behaviour.Widget = function(widget, options) {
  var _options = options || {},
      _mode,
      _previousOnMode = 'auto',
      _loadingMode;

  
  
  
  
  widget.setDisplayMode = function(mode) {
    var newMode = mode || 'auto';
    if (_mode === newMode) return;

    OT.$.removeClass(this.domElement, 'OT_mode-' + _mode);
    OT.$.addClass(this.domElement, 'OT_mode-' + newMode);

    if (newMode === 'off') {
      _previousOnMode = _mode;
    }
    _mode = newMode;
  };

  widget.getDisplayMode = function() {
    return _mode;
  };

  widget.show = function() {
    if (_mode !== _previousOnMode) {
      this.setDisplayMode(_previousOnMode);
      if (_options.onShow) _options.onShow();
    }
    return this;
  };

  widget.showAfterLoading = function() {
    this.setDisplayMode(_loadingMode);
  };

  widget.hide = function() {
    if (_mode !== 'off') {
      this.setDisplayMode('off');
      if (_options.onHide) _options.onHide();
    }
    return this;
  };

  widget.hideWhileLoading = function() {
    _loadingMode = _mode;
    this.setDisplayMode('off');
  };

  widget.destroy = function() {
    if (_options.onDestroy) _options.onDestroy(this.domElement);
    if (this.domElement) OT.$.removeElement(this.domElement);

    return widget;
  };

  widget.appendTo = function(parent) {
    
    this.domElement = OT.$.createElement(_options.nodeName || 'div',
                                        _options.htmlAttributes,
                                        _options.htmlContent);

    if (_options.onCreate) _options.onCreate(this.domElement);

    widget.setDisplayMode(_options.mode);

    if (_options.mode === 'auto') {
      
      
      OT.$.addClass(widget.domElement, 'OT_mode-on-hold');
      setTimeout(function() {
        OT.$.removeClass(widget.domElement, 'OT_mode-on-hold');
      }, 2000);
    }


    
    parent.appendChild(this.domElement);

    return widget;
  };
};








OT.Chrome.VideoDisabledIndicator = function(options) {
  var _videoDisabled = false,
      _warning = false,
      updateClasses;

  updateClasses = OT.$.bind(function(domElement) {
    if (_videoDisabled) {
      OT.$.addClass(domElement, 'OT_video-disabled');
    } else {
      OT.$.removeClass(domElement, 'OT_video-disabled');
    }
    if(_warning) {
      OT.$.addClass(domElement, 'OT_video-disabled-warning');
    } else {
      OT.$.removeClass(domElement, 'OT_video-disabled-warning');
    }
    if ((_videoDisabled || _warning) &&
        (this.getDisplayMode() === 'auto' || this.getDisplayMode() === 'on')) {
      OT.$.addClass(domElement, 'OT_active');
    } else {
      OT.$.removeClass(domElement, 'OT_active');
    }
  }, this);

  this.disableVideo = function(value) {
    _videoDisabled = value;
    if(value === true) {
      _warning = false;
    }
    updateClasses(this.domElement);
  };

  this.setWarning = function(value) {
    _warning = value;
    updateClasses(this.domElement);
  };

  
  OT.Chrome.Behaviour.Widget(this, {
    mode: options.mode || 'auto',
    nodeName: 'div',
    htmlAttributes: {
      className: 'OT_video-disabled-indicator'
    }
  });

  var parentSetDisplayMode = OT.$.bind(this.setDisplayMode, this);
  this.setDisplayMode = function(mode) {
    parentSetDisplayMode(mode);
    updateClasses(this.domElement);
  };
};


















OT.Chrome.NamePanel = function(options) {
  var _name = options.name;

  if (!_name || OT.$.trim(_name).length === '') {
    _name = null;

    
    options.mode = 'off';
  }

  this.setName = OT.$.bind(function(name) {
    if (!_name) this.setDisplayMode('auto');
    _name = name;
    this.domElement.innerHTML = _name;
  });

  
  OT.Chrome.Behaviour.Widget(this, {
    mode: options.mode,
    nodeName: 'h1',
    htmlContent: _name,
    htmlAttributes: {
      className: 'OT_name OT_edge-bar-item'
    }
  });
};








OT.Chrome.MuteButton = function(options) {
  var _onClickCb,
      _muted = options.muted || false,
      updateClasses,
      attachEvents,
      detachEvents,
      onClick;

  updateClasses = OT.$.bind(function() {
    if (_muted) {
      OT.$.addClass(this.domElement, 'OT_active');
    } else {
      OT.$.removeClass(this.domElement, 'OT_active ');
    }
  }, this);

  
  attachEvents = function(elem) {
    _onClickCb = OT.$.bind(onClick, this);
    OT.$.on(elem, 'click', _onClickCb);
  };

  detachEvents = function(elem) {
    _onClickCb = null;
    OT.$.off(elem, 'click', _onClickCb);
  };

  onClick = function() {
    _muted = !_muted;

    updateClasses();

    if (_muted) {
      this.parent.trigger('muted', this);
    } else {
      this.parent.trigger('unmuted', this);
    }

    return false;
  };

  OT.$.defineProperties(this, {
    muted: {
      get: function() { return _muted; },
      set: function(muted) {
        _muted = muted;
        updateClasses();
      }
    }
  });

  
  var classNames = _muted ? 'OT_edge-bar-item OT_mute OT_active' : 'OT_edge-bar-item OT_mute';
  OT.Chrome.Behaviour.Widget(this, {
    mode: options.mode,
    nodeName: 'button',
    htmlContent: 'Mute',
    htmlAttributes: {
      className: classNames
    },
    onCreate: OT.$.bind(attachEvents, this),
    onDestroy: OT.$.bind(detachEvents, this)
  });
};

























OT.Chrome.BackingBar = function(options) {
  var _nameMode = options.nameMode,
      _muteMode = options.muteMode;

  function getDisplayMode() {
    if(_nameMode === 'on' || _muteMode === 'on') {
      return 'on';
    } else if(_nameMode === 'mini' || _muteMode === 'mini') {
      return 'mini';
    } else if(_nameMode === 'mini-auto' || _muteMode === 'mini-auto') {
      return 'mini-auto';
    } else if(_nameMode === 'auto' || _muteMode === 'auto') {
      return 'auto';
    } else {
      return 'off';
    }
  }

  
  OT.Chrome.Behaviour.Widget(this, {
    mode: getDisplayMode(),
    nodeName: 'div',
    htmlContent: '',
    htmlAttributes: {
      className: 'OT_bar OT_edge-bar-item'
    }
  });

  this.setNameMode = function(nameMode) {
    _nameMode = nameMode;
    this.setDisplayMode(getDisplayMode());
  };

  this.setMuteMode = function(muteMode) {
    _muteMode = muteMode;
    this.setDisplayMode(getDisplayMode());
  };

};










OT.Chrome.AudioLevelMeter = function(options) {

  var widget = this,
      _meterBarElement,
      _voiceOnlyIconElement,
      _meterValueElement,
      _value,
      _maxValue = options.maxValue || 1,
      _minValue = options.minValue || 0;

  function onCreate() {
    _meterBarElement = OT.$.createElement('div', {
      className: 'OT_audio-level-meter__bar'
    }, '');
    _meterValueElement = OT.$.createElement('div', {
      className: 'OT_audio-level-meter__value'
    }, '');
    _voiceOnlyIconElement = OT.$.createElement('div', {
      className: 'OT_audio-level-meter__audio-only-img'
    }, '');

    widget.domElement.appendChild(_meterBarElement);
    widget.domElement.appendChild(_voiceOnlyIconElement);
    widget.domElement.appendChild(_meterValueElement);
  }

  function updateView() {
    var percentSize = _value * 100 / (_maxValue - _minValue);
    _meterValueElement.style.width = _meterValueElement.style.height = 2 * percentSize + '%';
    _meterValueElement.style.top = _meterValueElement.style.right = -percentSize + '%';
  }

  
  var widgetOptions = {
    mode: options ? options.mode : 'auto',
    nodeName: 'div',
    htmlAttributes: {
      className: 'OT_audio-level-meter'
    },
    onCreate: onCreate
  };

  OT.Chrome.Behaviour.Widget(this, widgetOptions);

  
  var _setDisplayMode = OT.$.bind(widget.setDisplayMode, widget);
  widget.setDisplayMode = function(mode) {
    _setDisplayMode(mode);
    if (mode === 'off') {
      if (options.onPassivate) options.onPassivate();
    } else {
      if (options.onActivate) options.onActivate();
    }
  };

  widget.setValue = function(value) {
    _value = value;
    updateView();
  };
};






















OT.Chrome.Archiving = function(options) {
  var _archiving = options.archiving,
      _archivingStarted = options.archivingStarted || 'Archiving on',
      _archivingEnded = options.archivingEnded || 'Archiving off',
      _initialState = true,
      _lightBox,
      _light,
      _text,
      _textNode,
      renderStageDelayedAction,
      renderText,
      renderStage;

  renderText = function(text) {
    _textNode.nodeValue = text;
    _lightBox.setAttribute('title', text);
  };

  renderStage = OT.$.bind(function() {
    if(renderStageDelayedAction) {
      clearTimeout(renderStageDelayedAction);
      renderStageDelayedAction = null;
    }

    if(_archiving) {
      OT.$.addClass(_light, 'OT_active');
    } else {
      OT.$.removeClass(_light, 'OT_active');
    }

    OT.$.removeClass(this.domElement, 'OT_archiving-' + (!_archiving ? 'on' : 'off'));
    OT.$.addClass(this.domElement, 'OT_archiving-' + (_archiving ? 'on' : 'off'));
    if(options.show && _archiving) {
      renderText(_archivingStarted);
      OT.$.addClass(_text, 'OT_mode-on');
      OT.$.removeClass(_text, 'OT_mode-auto');
      this.setDisplayMode('on');
      renderStageDelayedAction = setTimeout(function() {
        OT.$.addClass(_text, 'OT_mode-auto');
        OT.$.removeClass(_text, 'OT_mode-on');
      }, 5000);
    } else if(options.show && !_initialState) {
      OT.$.addClass(_text, 'OT_mode-on');
      OT.$.removeClass(_text, 'OT_mode-auto');
      this.setDisplayMode('on');
      renderText(_archivingEnded);
      renderStageDelayedAction = setTimeout(OT.$.bind(function() {
        this.setDisplayMode('off');
      }, this), 5000);
    } else {
      this.setDisplayMode('off');
    }
  }, this);

  
  OT.Chrome.Behaviour.Widget(this, {
    mode: _archiving && options.show && 'on' || 'off',
    nodeName: 'h1',
    htmlAttributes: {className: 'OT_archiving OT_edge-bar-item OT_edge-bottom'},
    onCreate: OT.$.bind(function() {
      _lightBox = OT.$.createElement('div', {
        className: 'OT_archiving-light-box'
      }, '');
      _light = OT.$.createElement('div', {
        className: 'OT_archiving-light'
      }, '');
      _lightBox.appendChild(_light);
      _text = OT.$.createElement('div', {
        className: 'OT_archiving-status OT_mode-on OT_edge-bar-item OT_edge-bottom'
      }, '');
      _textNode = document.createTextNode('');
      _text.appendChild(_textNode);
      this.domElement.appendChild(_lightBox);
      this.domElement.appendChild(_text);
      renderStage();
    }, this)
  });

  this.setShowArchiveStatus = OT.$.bind(function(show) {
    options.show = show;
    if(this.domElement) {
      renderStage.call(this);
    }
  }, this);

  this.setArchiving = OT.$.bind(function(status) {
    _archiving = status;
    _initialState = false;
    if(this.domElement) {
      renderStage.call(this);
    }
  }, this);

};








!(function(window) {
  
  if (window && typeof(navigator) !== 'undefined') {
    var NativeRTCPeerConnection = (window.webkitRTCPeerConnection ||
                                   window.mozRTCPeerConnection);

    if (navigator.webkitGetUserMedia) {
      
      
      if (!webkitMediaStream.prototype.getVideoTracks) {
        webkitMediaStream.prototype.getVideoTracks = function() {
          return this.videoTracks;
        };
      }

      
      if (!webkitMediaStream.prototype.getAudioTracks) {
        webkitMediaStream.prototype.getAudioTracks = function() {
          return this.audioTracks;
        };
      }

      if (!webkitRTCPeerConnection.prototype.getLocalStreams) {
        webkitRTCPeerConnection.prototype.getLocalStreams = function() {
          return this.localStreams;
        };
      }

      if (!webkitRTCPeerConnection.prototype.getRemoteStreams) {
        webkitRTCPeerConnection.prototype.getRemoteStreams = function() {
          return this.remoteStreams;
        };
      }

    } else if (navigator.mozGetUserMedia) {
      
      
      if (!MediaStream.prototype.getVideoTracks) {
        MediaStream.prototype.getVideoTracks = function() {
          return [];
        };
      }

      if (!MediaStream.prototype.getAudioTracks) {
        MediaStream.prototype.getAudioTracks = function() {
          return [];
        };
      }

      
      
      
      
      
      
      

      
      
      
      
      
      
      
    }

    
    
    
    if (typeof window.MediaStreamTrack !== 'undefined') {
      if (!window.MediaStreamTrack.prototype.setEnabled) {
        window.MediaStreamTrack.prototype.setEnabled = function (enabled) {
          this.enabled = OT.$.castToBoolean(enabled);
        };
      }
    }

    if (!window.URL && window.webkitURL) {
      window.URL = window.webkitURL;
    }

    OT.$.createPeerConnection = function (config, options, publishersWebRtcStream, completion) {
      if (OTPlugin.isInstalled()) {
        OTPlugin.initPeerConnection(config, options,
                                    publishersWebRtcStream, completion);
      }
      else {
        var pc;

        try {
          pc = new NativeRTCPeerConnection(config, options);
        } catch(e) {
          completion(e.message);
          return;
        }

        completion(null, pc);
      }
    };
  }

  
  
  
  
  
  
  
  
  OT.$.supportedCryptoScheme = function() {
    return OT.$.env.name === 'Chrome' && OT.$.env.version < 25 ? 'SDES_SRTP' : 'DTLS_SRTP';
  };

})(window);














var subscribeProcessor = function(peerConnection, success, failure) {
  var constraints,
      generateErrorCallback,
      setLocalDescription;

  constraints = {
    mandatory: {},
    optional: []
  },

  generateErrorCallback = function(message, prefix) {
    return function(errorReason) {
      OT.error(message);
      OT.error(errorReason);

      if (failure) failure(message, errorReason, prefix);
    };
  };

  setLocalDescription = function(offer) {
    offer.sdp = SDPHelpers.removeComfortNoise(offer.sdp);
    offer.sdp = SDPHelpers.removeVideoCodec(offer.sdp, 'ulpfec');
    offer.sdp = SDPHelpers.removeVideoCodec(offer.sdp, 'red');

    peerConnection.setLocalDescription(
      offer,

      
      function() {
        success(offer);
      },

      
      generateErrorCallback('Error while setting LocalDescription', 'SetLocalDescription')
    );
  };

  
  if (navigator.mozGetUserMedia) {
    constraints.mandatory.MozDontOfferDataChannel = true;
  }

  peerConnection.createOffer(
    
    setLocalDescription,

    
    generateErrorCallback('Error while creating Offer', 'CreateOffer'),

    constraints
  );
};














var offerProcessor = function(peerConnection, offer, success, failure) {
  var generateErrorCallback,
      setLocalDescription,
      createAnswer;

  generateErrorCallback = function(message, prefix) {
    return function(errorReason) {
      OT.error(message);
      OT.error(errorReason);

      if (failure) failure(message, errorReason, prefix);
    };
  };

  setLocalDescription = function(answer) {
    answer.sdp = SDPHelpers.removeComfortNoise(answer.sdp);
    answer.sdp = SDPHelpers.removeVideoCodec(answer.sdp, 'ulpfec');
    answer.sdp = SDPHelpers.removeVideoCodec(answer.sdp, 'red');

    peerConnection.setLocalDescription(
      answer,

      
      function() {
        success(answer);
      },

      
      generateErrorCallback('Error while setting LocalDescription', 'SetLocalDescription')
    );
  };

  createAnswer = function() {
    peerConnection.createAnswer(
      
      setLocalDescription,

      
      generateErrorCallback('Error while setting createAnswer', 'CreateAnswer'),

      null, 
      false 
    );
  };

  
  
  if (offer.sdp.indexOf('a=crypto') === -1) {
    var cryptoLine = 'a=crypto:1 AES_CM_128_HMAC_SHA1_80 ' +
      'inline:FakeFakeFakeFakeFakeFakeFakeFakeFakeFake\\r\\n';

      
    offer.sdp = offer.sdp.replace(/^c=IN(.*)$/gmi, 'c=IN$1\r\n'+cryptoLine);
  }

  if (offer.sdp.indexOf('a=rtcp-fb') === -1) {
    var rtcpFbLine = 'a=rtcp-fb:* ccm fir\r\na=rtcp-fb:* nack ';

    
    offer.sdp = offer.sdp.replace(/^m=video(.*)$/gmi, 'm=video$1\r\n'+rtcpFbLine);
  }

  peerConnection.setRemoteDescription(
    offer,

    
    createAnswer,

    
    generateErrorCallback('Error while setting RemoteDescription', 'SetRemoteDescription')
  );

};






var NativeRTCIceCandidate;

if (!OTPlugin.isInstalled()) {
  NativeRTCIceCandidate = (window.mozRTCIceCandidate || window.RTCIceCandidate);
}
else {
  NativeRTCIceCandidate = OTPlugin.RTCIceCandidate;
}
















var IceCandidateProcessor = function() {
  var _pendingIceCandidates = [],
      _peerConnection = null;

  this.setPeerConnection = function(peerConnection) {
    _peerConnection = peerConnection;
  };

  this.process = function(message) {
    var iceCandidate = new NativeRTCIceCandidate(message.content);

    if (_peerConnection) {
      _peerConnection.addIceCandidate(iceCandidate);
    } else {
      _pendingIceCandidates.push(iceCandidate);
    }
  };

  this.processPending = function() {
    while(_pendingIceCandidates.length) {
      _peerConnection.addIceCandidate(_pendingIceCandidates.shift());
    }
  };
};






























var connectionStateLogger = function(pc) {
  var startTime = OT.$.now(),
      finishTime,
      suceeded,
      states = [];

  var trackState = function() {
    var now = OT.$.now(),
        lastState = states[states.length-1],
        state = {delta: finishTime ? now - finishTime : 0};

    if (!lastState || lastState.iceConnection !== pc.iceConnectionState) {
      state.iceConnectionState = pc.iceConnectionState;
    }

    if (!lastState || lastState.signalingState !== pc.signalingState) {
      state.signalingState = pc.signalingState;
    }

    if (!lastState || lastState.iceGatheringState !== pc.iceGatheringState) {
      state.iceGathering = pc.iceGatheringState;
    }
    OT.debug(state);
    states.push(state);
    finishTime = now;
  };

  pc.addEventListener('iceconnectionstatechange', trackState, false);
  pc.addEventListener('signalingstatechange', trackState, false);
  pc.addEventListener('icegatheringstatechange', trackState, false);

  return {
    stop: function () {
      pc.removeEventListener('iceconnectionstatechange', trackState, false);
      pc.removeEventListener('signalingstatechange', trackState, false);
      pc.removeEventListener('icegatheringstatechange', trackState, false);

      

      
      suceeded = true;

      var payload = {
        type: 'PeerConnectionWorkflow',
        success: suceeded,
        startTime: startTime,
        finishTime: finishTime,
        states: states
      };

      
      OT.debug(payload);
    }
  };
};












var NativeRTCSessionDescription;

if (!OTPlugin.isInstalled()) {
  
  NativeRTCSessionDescription = (window.mozRTCSessionDescription ||
                                 window.RTCSessionDescription);
}
else {
  NativeRTCSessionDescription = OTPlugin.RTCSessionDescription;
}



var iceCandidateForwarder = function(messageDelegate) {
  return function(event) {
    if (event.candidate) {
      messageDelegate(OT.Raptor.Actions.CANDIDATE, event.candidate);
    } else {
      OT.debug('IceCandidateForwarder: No more ICE candidates.');
    }
  };
};










OT.PeerConnection = function(config) {
  var _peerConnection,
      _peerConnectionCompletionHandlers = [],
      _iceProcessor = new IceCandidateProcessor(),
      _getStatsAdapter = OT.getStatsAdpater(),
      _stateLogger,
      _offer,
      _answer,
      _state = 'new',
      _messageDelegates = [];


  OT.$.eventing(this);

  
  
  
  if (!config.iceServers) config.iceServers = [];

  
  var delegateMessage = OT.$.bind(function(type, messagePayload, uri) {
        if (_messageDelegates.length) {
          
          
          
          
          
          _messageDelegates[0](type, messagePayload, uri);
        }
      }, this),

      
      
      
      
      
      
      
      
      
      
      
      
      createPeerConnection = OT.$.bind(function (completion, localWebRtcStream) {
        if (_peerConnection) {
          completion.call(null, null, _peerConnection);
          return;
        }

        _peerConnectionCompletionHandlers.push(completion);

        if (_peerConnectionCompletionHandlers.length > 1) {
          
          
          return;
        }

        var pcConstraints = {
          optional: [
            {DtlsSrtpKeyAgreement: true}
          ]
        };

        OT.debug('Creating peer connection config "' + JSON.stringify(config) + '".');

        if (!config.iceServers || config.iceServers.length === 0) {
          
          OT.error('No ice servers present');
        }

        OT.$.createPeerConnection(config, pcConstraints, localWebRtcStream,
                                        attachEventsToPeerConnection);
      }, this),

      
      
      
      
      
      
      attachEventsToPeerConnection = OT.$.bind(function(err, pc) {
        if (err) {
          triggerError('Failed to create PeerConnection, exception: ' +
              err.toString(), 'NewPeerConnection');

          _peerConnectionCompletionHandlers = [];
          return;
        }

        OT.debug('OT attachEventsToPeerConnection');
        _peerConnection = pc;
        _stateLogger = connectionStateLogger(_peerConnection);

        _peerConnection.addEventListener('icecandidate',
                                    iceCandidateForwarder(delegateMessage), false);
        _peerConnection.addEventListener('addstream', onRemoteStreamAdded, false);
        _peerConnection.addEventListener('removestream', onRemoteStreamRemoved, false);
        _peerConnection.addEventListener('signalingstatechange', routeStateChanged, false);

        if (_peerConnection.oniceconnectionstatechange !== void 0) {
          var failedStateTimer;
          _peerConnection.addEventListener('iceconnectionstatechange', function (event) {
            if (event.target.iceConnectionState === 'failed') {
              if (failedStateTimer) {
                clearTimeout(failedStateTimer);
              }

              
              
              
              failedStateTimer = setTimeout(function () {
                if (event.target.iceConnectionState === 'failed') {
                  triggerError('The stream was unable to connect due to a network error.' +
                   ' Make sure your connection isn\'t blocked by a firewall.', 'ICEWorkflow');
                }
              }, 5000);
            }
          }, false);
        }

        triggerPeerConnectionCompletion(null);
      }, this),

      triggerPeerConnectionCompletion = function () {
        while (_peerConnectionCompletionHandlers.length) {
          _peerConnectionCompletionHandlers.shift().call(null);
        }
      },

      
      
      
      tearDownPeerConnection = function() {
        
        if (_iceProcessor) _iceProcessor.setPeerConnection(null);
        if (_stateLogger) _stateLogger.stop();

        qos.stopCollecting();

        if (_peerConnection !== null) {
          if (_peerConnection.destroy) {
            
            
            _peerConnection.destroy();
          }

          _peerConnection = null;
          this.trigger('close');
        }
      },

      routeStateChanged = OT.$.bind(function() {
        var newState = _peerConnection.signalingState;

        if (newState && newState !== _state) {
          _state = newState;
          OT.debug('PeerConnection.stateChange: ' + _state);

          switch(_state) {
            case 'closed':
              tearDownPeerConnection.call(this);
              break;
          }
        }
      }, this),

      qosCallback = OT.$.bind(function(parsedStats) {
        this.trigger('qos', parsedStats);
      }, this),

      getRemoteStreams = function() {
        var streams;

        if (_peerConnection.getRemoteStreams) {
          streams = _peerConnection.getRemoteStreams();
        } else if (_peerConnection.remoteStreams) {
          streams = _peerConnection.remoteStreams;
        } else {
          throw new Error('Invalid Peer Connection object implements no ' +
            'method for retrieving remote streams');
        }

        
        
        
        return Array.prototype.slice.call(streams);
      },

      
      onRemoteStreamAdded = OT.$.bind(function(event) {
        this.trigger('streamAdded', event.stream);
      }, this),

      onRemoteStreamRemoved = OT.$.bind(function(event) {
        this.trigger('streamRemoved', event.stream);
      }, this),

      


      
      
      relaySDP = function(messageType, sdp, uri) {
        delegateMessage(messageType, sdp, uri);
      },


      
      processOffer = function(message) {
        var offer = new NativeRTCSessionDescription({type: 'offer', sdp: message.content.sdp}),

            
            relayAnswer = function(answer) {
              _iceProcessor.setPeerConnection(_peerConnection);
              _iceProcessor.processPending();
              relaySDP(OT.Raptor.Actions.ANSWER, answer);

              qos.startCollecting(_peerConnection);
            },

            reportError = function(message, errorReason, prefix) {
              triggerError('PeerConnection.offerProcessor ' + message + ': ' +
                errorReason, prefix);
            };

        createPeerConnection(function() {
          offerProcessor(
            _peerConnection,
            offer,
            relayAnswer,
            reportError
          );
        });
      },

      processAnswer = function(message) {
        if (!message.content.sdp) {
          OT.error('PeerConnection.processMessage: Weird answer message, no SDP.');
          return;
        }

        _answer = new NativeRTCSessionDescription({type: 'answer', sdp: message.content.sdp});

        _peerConnection.setRemoteDescription(_answer,
            function () {
              OT.debug('setRemoteDescription Success');
            }, function (errorReason) {
              triggerError('Error while setting RemoteDescription ' + errorReason,
                'SetRemoteDescription');
            });

        _iceProcessor.setPeerConnection(_peerConnection);
        _iceProcessor.processPending();

        qos.startCollecting(_peerConnection);
      },

      processSubscribe = function(message) {
        OT.debug('PeerConnection.processSubscribe: Sending offer to subscriber.');

        if (!_peerConnection) {
          
          
          
          throw new Error('PeerConnection broke!');
        }

        createPeerConnection(function() {
          subscribeProcessor(
            _peerConnection,

            
            function(offer) {
              _offer = offer;
              relaySDP(OT.Raptor.Actions.OFFER, _offer, message.uri);
            },

            
            function(message, errorReason, prefix) {
              triggerError('PeerConnection.subscribeProcessor ' + message + ': ' +
                errorReason, prefix);
            }
          );
        });
      },

      triggerError = OT.$.bind(function(errorReason, prefix) {
        OT.error(errorReason);
        this.trigger('error', errorReason, prefix);
      }, this);

  this.addLocalStream = function(webRTCStream) {
    createPeerConnection(function() {
      _peerConnection.addStream(webRTCStream);
    }, webRTCStream);
  };

  this.getSenders = function() {
    return _peerConnection.getSenders();
  };

  this.disconnect = function() {
    _iceProcessor = null;

    if (_peerConnection &&
        _peerConnection.signalingState &&
        _peerConnection.signalingState.toLowerCase() !== 'closed') {

      _peerConnection.close();

      if (OT.$.env.name === 'Firefox') {
        
        
        
        
        
        
        OT.$.callAsync(OT.$.bind(tearDownPeerConnection, this));
      }
    }

    this.off();
  };

  this.processMessage = function(type, message) {
    OT.debug('PeerConnection.processMessage: Received ' +
      type + ' from ' + message.fromAddress);

    OT.debug(message);

    switch(type) {
      case 'generateoffer':
        processSubscribe.call(this, message);
        break;

      case 'offer':
        processOffer.call(this, message);
        break;

      case 'answer':
      case 'pranswer':
        processAnswer.call(this, message);
        break;

      case 'candidate':
        _iceProcessor.process(message);
        break;

      default:
        OT.debug('PeerConnection.processMessage: Received an unexpected message of type ' +
          type + ' from ' + message.fromAddress + ': ' + JSON.stringify(message));
    }

    return this;
  };

  this.setIceServers = function (iceServers) {
    if (iceServers) {
      config.iceServers = iceServers;
    }
  };

  this.registerMessageDelegate = function(delegateFn) {
    return _messageDelegates.push(delegateFn);
  };

  this.unregisterMessageDelegate = function(delegateFn) {
    var index = OT.$.arrayIndexOf(_messageDelegates, delegateFn);

    if ( index !== -1 ) {
      _messageDelegates.splice(index, 1);
    }
    return _messageDelegates.length;
  };

  this.remoteStreams = function() {
    return _peerConnection ? getRemoteStreams() : [];
  };

  this.getStats = function(callback) {
    createPeerConnection(function() {
      _getStatsAdapter(_peerConnection, callback);
    });
  };

  var qos = new OT.PeerConnection.QOS(qosCallback);
};





















!(function() {

  
  
  
  
  var parseStatsOldAPI = function parseStatsOldAPI (peerConnection,
                                                    prevStats,
                                                    currentStats,
                                                    completion) {

    
    var parseVideoStats = function (result) {
          if (result.stat('googFrameRateSent')) {
            currentStats.videoSentBytes = Number(result.stat('bytesSent'));
            currentStats.videoSentPackets = Number(result.stat('packetsSent'));
            currentStats.videoSentPacketsLost = Number(result.stat('packetsLost'));
            currentStats.videoRtt = Number(result.stat('googRtt'));
            currentStats.videoFrameRate = Number(result.stat('googFrameRateInput'));
            currentStats.videoWidth = Number(result.stat('googFrameWidthSent'));
            currentStats.videoHeight = Number(result.stat('googFrameHeightSent'));
            currentStats.videoCodec = result.stat('googCodecName');
          } else if (result.stat('googFrameRateReceived')) {
            currentStats.videoRecvBytes = Number(result.stat('bytesReceived'));
            currentStats.videoRecvPackets = Number(result.stat('packetsReceived'));
            currentStats.videoRecvPacketsLost = Number(result.stat('packetsLost'));
            currentStats.videoFrameRate = Number(result.stat('googFrameRateOutput'));
            currentStats.videoWidth = Number(result.stat('googFrameWidthReceived'));
            currentStats.videoHeight = Number(result.stat('googFrameHeightReceived'));
            currentStats.videoCodec = result.stat('googCodecName');
          }
          return null;
        },

        parseAudioStats = function (result) {
          if (result.stat('audioInputLevel')) {
            currentStats.audioSentPackets = Number(result.stat('packetsSent'));
            currentStats.audioSentPacketsLost = Number(result.stat('packetsLost'));
            currentStats.audioSentBytes =  Number(result.stat('bytesSent'));
            currentStats.audioCodec = result.stat('googCodecName');
            currentStats.audioRtt = Number(result.stat('googRtt'));
          } else if (result.stat('audioOutputLevel')) {
            currentStats.audioRecvPackets = Number(result.stat('packetsReceived'));
            currentStats.audioRecvPacketsLost = Number(result.stat('packetsLost'));
            currentStats.audioRecvBytes =  Number(result.stat('bytesReceived'));
            currentStats.audioCodec = result.stat('googCodecName');
          }
        },

        parseStatsReports = function (stats) {
          if (stats.result) {
            var resultList = stats.result();
            for (var resultIndex = 0; resultIndex < resultList.length; resultIndex++) {
              var result = resultList[resultIndex];

              if (result.stat) {

                if(result.stat('googActiveConnection') === 'true') {
                  currentStats.localCandidateType = result.stat('googLocalCandidateType');
                  currentStats.remoteCandidateType = result.stat('googRemoteCandidateType');
                  currentStats.transportType = result.stat('googTransportType');
                }

                parseAudioStats(result);
                parseVideoStats(result);
              }
            }
          }

          completion(null, currentStats);
        };

    peerConnection.getStats(parseStatsReports);
  };

  
  
  
  
  var parseStatsOTPlugin = function parseStatsOTPlugin (peerConnection,
                                                    prevStats,
                                                    currentStats,
                                                    completion) {

    var onStatsError = function onStatsError (error) {
          completion(error);
        },

        
        
        
        
        
        parseAudioStats = function (statsReport) {
          var lastBytesSent = prevStats.audioBytesTransferred || 0,
              transferDelta;

          if (statsReport.audioInputLevel) {
            currentStats.audioSentBytes = Number(statsReport.bytesSent);
            currentStats.audioSentPackets = Number(statsReport.packetsSent);
            currentStats.audioSentPacketsLost = Number(statsReport.packetsLost);
            currentStats.audioRtt = Number(statsReport.googRtt);
            currentStats.audioCodec = statsReport.googCodecName;
          }
          else if (statsReport.audioOutputLevel) {
            currentStats.audioBytesTransferred = Number(statsReport.bytesReceived);
            currentStats.audioCodec = statsReport.googCodecName;
          }

          if (currentStats.audioBytesTransferred) {
            transferDelta = currentStats.audioBytesTransferred - lastBytesSent;
            currentStats.avgAudioBitrate = Math.round(transferDelta * 8 / currentStats.period);
          }
        },

        
        
        
        
        
        
        parseVideoStats = function (statsReport) {

          var lastBytesSent = prevStats.videoBytesTransferred || 0,
              transferDelta;

          if (statsReport.googFrameHeightSent) {
            currentStats.videoSentBytes = Number(statsReport.bytesSent);
            currentStats.videoSentPackets = Number(statsReport.packetsSent);
            currentStats.videoSentPacketsLost = Number(statsReport.packetsLost);
            currentStats.videoRtt = Number(statsReport.googRtt);
            currentStats.videoCodec = statsReport.googCodecName;
            currentStats.videoWidth = Number(statsReport.googFrameWidthSent);
            currentStats.videoHeight = Number(statsReport.googFrameHeightSent);
          }
          else if (statsReport.googFrameHeightReceived) {
            currentStats.videoRecvBytes =   Number(statsReport.bytesReceived);
            currentStats.videoRecvPackets = Number(statsReport.packetsReceived);
            currentStats.videoRecvPacketsLost = Number(statsReport.packetsLost);
            currentStats.videoRtt = Number(statsReport.googRtt);
            currentStats.videoCodec = statsReport.googCodecName;
            currentStats.videoWidth = Number(statsReport.googFrameWidthReceived);
            currentStats.videoHeight = Number(statsReport.googFrameHeightReceived);
          }

          if (currentStats.videoBytesTransferred) {
            transferDelta = currentStats.videoBytesTransferred - lastBytesSent;
            currentStats.avgVideoBitrate = Math.round(transferDelta * 8 / currentStats.period);
          }

          if (statsReport.googFrameRateSent) {
            currentStats.videoFrameRate = Number(statsReport.googFrameRateSent);
          } else if (statsReport.googFrameRateReceived) {
            currentStats.videoFrameRate = Number(statsReport.googFrameRateReceived);
          }
        },

        isStatsForVideoTrack = function(statsReport) {
          return statsReport.googFrameHeightSent !== void 0 ||
                  statsReport.googFrameHeightReceived !== void 0 ||
                  currentStats.videoBytesTransferred !== void 0 ||
                  statsReport.googFrameRateSent !== void 0;
        },

        isStatsForIceCandidate = function(statsReport) {
          return statsReport.googActiveConnection === 'true';
        };

    peerConnection.getStats(null, function(statsReports) {
      statsReports.forEach(function(statsReport) {
        if (isStatsForIceCandidate(statsReport)) {
          currentStats.localCandidateType = statsReport.googLocalCandidateType;
          currentStats.remoteCandidateType = statsReport.googRemoteCandidateType;
          currentStats.transportType = statsReport.googTransportType;
        }
        else if (isStatsForVideoTrack(statsReport)) {
          parseVideoStats(statsReport);
        }
        else {
          parseAudioStats(statsReport);
        }
      });

      completion(null, currentStats);
    }, onStatsError);
  };


  
  
  
  var parseStatsNewAPI = function parseStatsNewAPI (peerConnection,
                                                    prevStats,
                                                    currentStats,
                                                    completion) {

    var onStatsError = function onStatsError (error) {
          completion(error);
        },

        parseAudioStats = function (result) {
          if (result.type==='outboundrtp') {
            currentStats.audioSentPackets = result.packetsSent;
            currentStats.audioSentPacketsLost = result.packetsLost;
            currentStats.audioSentBytes =  result.bytesSent;
          } else if (result.type==='inboundrtp') {
            currentStats.audioRecvPackets = result.packetsReceived;
            currentStats.audioRecvPacketsLost = result.packetsLost;
            currentStats.audioRecvBytes =  result.bytesReceived;
          }
        },

        parseVideoStats = function (result) {
          if (result.type==='outboundrtp') {
            currentStats.videoSentPackets = result.packetsSent;
            currentStats.videoSentPacketsLost = result.packetsLost;
            currentStats.videoSentBytes =  result.bytesSent;
          } else if (result.type==='inboundrtp') {
            currentStats.videoRecvPackets = result.packetsReceived;
            currentStats.videoRecvPacketsLost = result.packetsLost;
            currentStats.videoRecvBytes =  result.bytesReceived;
          }
        };

    peerConnection.getStats(null, function(stats) {

      for (var key in stats) {
        if (stats.hasOwnProperty(key) &&
          (stats[key].type === 'outboundrtp' || stats[key].type === 'inboundrtp')) {
          var res = stats[key];

          if (res.id.indexOf('audio') !== -1) {
            parseAudioStats(res);
          } else if (res.id.indexOf('video') !== -1) {
            parseVideoStats(res);
          }
        }
      }

      completion(null, currentStats);
    }, onStatsError);
  };


  var parseQOS = function (peerConnection, prevStats, currentStats, completion) {
    if (OTPlugin.isInstalled()) {
      parseQOS = parseStatsOTPlugin;
      return parseStatsOTPlugin(peerConnection, prevStats, currentStats, completion);
    }
    else if (OT.$.env.name === 'Firefox' && OT.$.env.version >= 27) {
      parseQOS = parseStatsNewAPI;
      return parseStatsNewAPI(peerConnection, prevStats, currentStats, completion);
    }
    else {
      parseQOS = parseStatsOldAPI;
      return parseStatsOldAPI(peerConnection, prevStats, currentStats, completion);
    }
  };

  OT.PeerConnection.QOS = function (qosCallback) {
    var _creationTime = OT.$.now(),
        _peerConnection;

    var calculateQOS = OT.$.bind(function calculateQOS (prevStats) {
      if (!_peerConnection) {
        
        
        return;
      }

      var now = OT.$.now();

      var currentStats = {
        timeStamp: now,
        duration: Math.round(now - _creationTime),
        period: (now - prevStats.timeStamp) / 1000
      };

      var onParsedStats = function (err, parsedStats) {
        if (err) {
          OT.error('Failed to Parse QOS Stats: ' + JSON.stringify(err));
          return;
        }

        qosCallback(parsedStats, prevStats);

        
        setTimeout(OT.$.bind(calculateQOS, null, parsedStats), OT.PeerConnection.QOS.INTERVAL);
      };

      parseQOS(_peerConnection, prevStats, currentStats, onParsedStats);
    }, this);


    this.startCollecting = function (peerConnection) {
      if (!peerConnection || !peerConnection.getStats) {
        
        
        return;
      }

      _peerConnection = peerConnection;

      calculateQOS({
        timeStamp: OT.$.now()
      });
    };

    this.stopCollecting = function () {
      _peerConnection = null;
    };
  };

  
  OT.PeerConnection.QOS.INTERVAL = 30000;
})();




OT.PeerConnections = (function() {
  var _peerConnections = {};

  return {
    add: function(remoteConnection, streamId, config) {
      var key = remoteConnection.id + '_' + streamId,
          ref = _peerConnections[key];

      if (!ref) {
        ref = _peerConnections[key] = {
          count: 0,
          pc: new OT.PeerConnection(config)
        };
      }

      
      ref.count += 1;

      return ref.pc;
    },

    remove: function(remoteConnection, streamId) {
      var key = remoteConnection.id + '_' + streamId,
          ref = _peerConnections[key];

      if (ref) {
        ref.count -= 1;

        if (ref.count === 0) {
          ref.pc.disconnect();
          delete _peerConnections[key];
        }
      }
    }
  };
})();




























OT.SubscriberPeerConnection = function(remoteConnection, session, stream,
  subscriber, properties) {
  var _peerConnection,
      _destroyed = false,
      _hasRelayCandidates = false,
      _onPeerClosed,
      _onRemoteStreamAdded,
      _onRemoteStreamRemoved,
      _onPeerError,
      _relayMessageToPeer,
      _setEnabledOnStreamTracksCurry,
      _onQOS;

  
  _onPeerClosed = function() {
    this.destroy();
    this.trigger('disconnected', this);
  };

  _onRemoteStreamAdded = function(remoteRTCStream) {
    this.trigger('remoteStreamAdded', remoteRTCStream, this);
  };

  _onRemoteStreamRemoved = function(remoteRTCStream) {
    this.trigger('remoteStreamRemoved', remoteRTCStream, this);
  };

  
  _onPeerError = function(errorReason, prefix) {
    this.trigger('error', null, errorReason, this, prefix);
  };

  _relayMessageToPeer = OT.$.bind(function(type, payload) {
    if (!_hasRelayCandidates){
      var extractCandidates = type === OT.Raptor.Actions.CANDIDATE ||
                              type === OT.Raptor.Actions.OFFER ||
                              type === OT.Raptor.Actions.ANSWER ||
                              type === OT.Raptor.Actions.PRANSWER ;

      if (extractCandidates) {
        var message = (type === OT.Raptor.Actions.CANDIDATE) ? payload.candidate : payload.sdp;
        _hasRelayCandidates = message.indexOf('typ relay') !== -1;
      }
    }

    switch(type) {
      case OT.Raptor.Actions.ANSWER:
      case OT.Raptor.Actions.PRANSWER:
        this.trigger('connected');

        session._.jsepAnswerP2p(stream.id, subscriber.widgetId, payload.sdp);
        break;

      case OT.Raptor.Actions.OFFER:
        session._.jsepOfferP2p(stream.id, subscriber.widgetId, payload.sdp);
        break;

      case OT.Raptor.Actions.CANDIDATE:
        session._.jsepCandidateP2p(stream.id, subscriber.widgetId, payload);
        break;
    }
  }, this);

  
  _setEnabledOnStreamTracksCurry = function(isVideo) {
    var method = 'get' + (isVideo ? 'Video' : 'Audio') + 'Tracks';

    return function(enabled) {
      var remoteStreams = _peerConnection.remoteStreams(),
          tracks,
          stream;

      if (remoteStreams.length === 0 || !remoteStreams[0][method]) {
        
        
        return;
      }

      for (var i=0, num=remoteStreams.length; i<num; ++i) {
        stream = remoteStreams[i];
        tracks = stream[method]();

        for (var k=0, numTracks=tracks.length; k < numTracks; ++k){
          
          
          if (tracks[k].enabled !== enabled) tracks[k].enabled=enabled;
        }
      }
    };
  };

  _onQOS = OT.$.bind(function _onQOS (parsedStats, prevStats) {
    this.trigger('qos', parsedStats, prevStats);
  }, this);

  OT.$.eventing(this);

  
  this.destroy = function() {
    if (_destroyed) return;
    _destroyed = true;

    if (_peerConnection) {
      var numDelegates = _peerConnection.unregisterMessageDelegate(_relayMessageToPeer);

      
      if (numDelegates === 0) {
        
        if (session && session.isConnected() && stream && !stream.destroyed) {
            
          session._.subscriberDestroy(stream, subscriber);
        }

        
        this.subscribeToAudio(false);
      }
      OT.PeerConnections.remove(remoteConnection, stream.id);
    }
    _peerConnection = null;
    this.off();
  };

  this.processMessage = function(type, message) {
    _peerConnection.processMessage(type, message);
  };

  this.getStats = function(callback) {
    _peerConnection.getStats(callback);
  };

  this.subscribeToAudio = _setEnabledOnStreamTracksCurry(false);
  this.subscribeToVideo = _setEnabledOnStreamTracksCurry(true);

  this.hasRelayCandidates = function() {
    return _hasRelayCandidates;
  };

  
  this.init = function() {
    _peerConnection = OT.PeerConnections.add(remoteConnection, stream.streamId, {});

    _peerConnection.on({
      close: _onPeerClosed,
      streamAdded: _onRemoteStreamAdded,
      streamRemoved: _onRemoteStreamRemoved,
      error: _onPeerError,
      qos: _onQOS
    }, this);

    var numDelegates = _peerConnection.registerMessageDelegate(_relayMessageToPeer);

      
    if (_peerConnection.remoteStreams().length > 0) {
      OT.$.forEach(_peerConnection.remoteStreams(), _onRemoteStreamAdded, this);
    } else if (numDelegates === 1) {
      
      

      var channelsToSubscribeTo;

      if (properties.subscribeToVideo || properties.subscribeToAudio) {
        var audio = stream.getChannelsOfType('audio'),
            video = stream.getChannelsOfType('video');

        channelsToSubscribeTo = OT.$.map(audio, function(channel) {
          return {
            id: channel.id,
            type: channel.type,
            active: properties.subscribeToAudio
          };
        }).concat(OT.$.map(video, function(channel) {
          return {
            id: channel.id,
            type: channel.type,
            active: properties.subscribeToVideo,
            restrictFrameRate: properties.restrictFrameRate !== void 0 ?
              properties.restrictFrameRate : false
          };
        }));
      }

      session._.subscriberCreate(stream, subscriber, channelsToSubscribeTo,
        OT.$.bind(function(err, message) {
          if (err) {
            this.trigger('error', err.message, this, 'Subscribe');
          }
          if (_peerConnection) {
            _peerConnection.setIceServers(OT.Raptor.parseIceServers(message));
          }
        }, this));
    }
  };
};























OT.PublisherPeerConnection = function(remoteConnection, session, streamId, webRTCStream) {
  var _peerConnection,
      _hasRelayCandidates = false,
      _subscriberId = session._.subscriberMap[remoteConnection.id + '_' + streamId],
      _onPeerClosed,
      _onPeerError,
      _relayMessageToPeer,
      _onQOS;

  
  _onPeerClosed = function() {
    this.destroy();
    this.trigger('disconnected', this);
  };

  
  _onPeerError = function(errorReason, prefix) {
    this.trigger('error', null, errorReason, this, prefix);
    this.destroy();
  };

  _relayMessageToPeer = OT.$.bind(function(type, payload, uri) {
    if (!_hasRelayCandidates){
      var extractCandidates = type === OT.Raptor.Actions.CANDIDATE ||
                              type === OT.Raptor.Actions.OFFER ||
                              type === OT.Raptor.Actions.ANSWER ||
                              type === OT.Raptor.Actions.PRANSWER ;

      if (extractCandidates) {
        var message = (type === OT.Raptor.Actions.CANDIDATE) ? payload.candidate : payload.sdp;
        _hasRelayCandidates = message.indexOf('typ relay') !== -1;
      }
    }

    switch(type) {
      case OT.Raptor.Actions.ANSWER:
      case OT.Raptor.Actions.PRANSWER:
        if (session.sessionInfo.p2pEnabled) {
          session._.jsepAnswerP2p(streamId, _subscriberId, payload.sdp);
        } else {
          session._.jsepAnswer(streamId, payload.sdp);
        }

        break;

      case OT.Raptor.Actions.OFFER:
        this.trigger('connected');
        session._.jsepOffer(uri, payload.sdp);

        break;

      case OT.Raptor.Actions.CANDIDATE:
        if (session.sessionInfo.p2pEnabled) {
          session._.jsepCandidateP2p(streamId, _subscriberId, payload);

        } else {
          session._.jsepCandidate(streamId, payload);
        }
    }
  }, this);

  _onQOS = OT.$.bind(function _onQOS (parsedStats, prevStats) {
    this.trigger('qos', remoteConnection, parsedStats, prevStats);
  }, this);

  OT.$.eventing(this);

  
  this.destroy = function() {
    
    if (_peerConnection) {
      _peerConnection.off();
      OT.PeerConnections.remove(remoteConnection, streamId);
    }

    _peerConnection = null;
  };

  this.processMessage = function(type, message) {
    _peerConnection.processMessage(type, message);
  };

  
  this.init = function(iceServers) {
    _peerConnection = OT.PeerConnections.add(remoteConnection, streamId, {
      iceServers: iceServers
    });

    _peerConnection.on({
      close: _onPeerClosed,
      error: _onPeerError,
      qos: _onQOS
    }, this);

    _peerConnection.registerMessageDelegate(_relayMessageToPeer);
    _peerConnection.addLocalStream(webRTCStream);

    this.remoteConnection = function() {
      return remoteConnection;
    };

    this.hasRelayCandidates = function() {
      return _hasRelayCandidates;
    };

  };

  this.getSenders = function() {
    return _peerConnection.getSenders();
  };
};









var videoContentResizesMixin = function(self, domElement) {

  var width = domElement.videoWidth,
      height = domElement.videoHeight,
      stopped = true;

  function actor() {
    if (stopped) {
      return;
    }
    if (width !== domElement.videoWidth || height !== domElement.videoHeight) {
      self.trigger('videoDimensionsChanged',
        { width: width, height: height },
        { width: domElement.videoWidth, height: domElement.videoHeight }
      );
      width = domElement.videoWidth;
      height = domElement.videoHeight;
    }
    waiter();
  }

  function waiter() {
    self.whenTimeIncrements(function() {
      window.requestAnimationFrame(actor);
    });
  }

  self.startObservingSize = function() {
    stopped = false;
    waiter();
  };

  self.stopObservingSize = function() {
    stopped = true;
  };

};

(function(window) {

  var VideoOrientationTransforms = {
    0: 'rotate(0deg)',
    270: 'rotate(90deg)',
    90: 'rotate(-90deg)',
    180: 'rotate(180deg)'
  };

  OT.VideoOrientation = {
    ROTATED_NORMAL: 0,
    ROTATED_LEFT: 270,
    ROTATED_RIGHT: 90,
    ROTATED_UPSIDE_DOWN: 180
  };

  var DefaultAudioVolume = 50;

  var DEGREE_TO_RADIANS = Math.PI * 2 / 360;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  OT.VideoElement = function( options) {
    var _options = OT.$.defaults( options && !OT.$.isFunction(options) ? options : {}, {
        fallbackText: 'Sorry, Web RTC is not available in your browser'
      }),

      errorHandler = OT.$.isFunction(arguments[arguments.length-1]) ?
                                    arguments[arguments.length-1] : void 0,

      orientationHandler = OT.$.bind(function(orientation) {
        this.trigger('orientationChanged', orientation);
      }, this),

      _videoElement = OTPlugin.isInstalled() ?
                            new PluginVideoElement(_options, errorHandler, orientationHandler) :
                            new NativeDOMVideoElement(_options, errorHandler, orientationHandler),
      _streamBound = false,
      _stream,
      _preInitialisedVolue;

    OT.$.eventing(this);

    _videoElement.on('videoDimensionsChanged', OT.$.bind(function(oldValue, newValue) {
      this.trigger('videoDimensionsChanged', oldValue, newValue);
    }, this));

    _videoElement.on('mediaStopped', OT.$.bind(function() {
      this.trigger('mediaStopped');
    }, this));

    
    OT.$.defineProperties(this, {

      domElement: {
        get: function() {
          return _videoElement.domElement();
        }
      },

      videoWidth: {
        get: function() {
          return _videoElement['video' + (this.isRotated() ? 'Height' : 'Width')]();
        }
      },

      videoHeight: {
        get: function() {
          return _videoElement['video' + (this.isRotated() ? 'Width' : 'Height')]();
        }
      },

      aspectRatio: {
        get: function() {
          return (this.videoWidth() + 0.0) / this.videoHeight();
        }
      },

      isRotated: {
        get: function() {
          return _videoElement.isRotated();
        }
      },

      orientation: {
        get: function() {
          return _videoElement.orientation();
        },
        set: function(orientation) {
          _videoElement.orientation(orientation);
        }
      },

      audioChannelType: {
        get: function() {
          return _videoElement.audioChannelType();
        },
        set: function(type) {
          _videoElement.audioChannelType(type);
        }
      }
    });

    

    this.imgData = function() {
      return _videoElement.imgData();
    };

    this.appendTo = function(parentDomElement) {
      _videoElement.appendTo(parentDomElement);
      return this;
    };

    this.bindToStream = function(webRtcStream, completion) {
      _streamBound = false;
      _stream = webRtcStream;

      _videoElement.bindToStream(webRtcStream, OT.$.bind(function(err) {
        if (err) {
          completion(err);
          return;
        }

        _streamBound = true;

        if (_preInitialisedVolue) {
          this.setAudioVolume(_preInitialisedVolue);
          _preInitialisedVolue = null;
        }

        _videoElement.on('aspectRatioAvailable',
          OT.$.bind(this.trigger, this, 'aspectRatioAvailable'));

        completion(null);
      }, this));

      return this;
    };

    this.unbindStream = function() {
      if (!_stream) return this;

      _stream = null;
      _videoElement.unbindStream();
      return this;
    };

    this.setAudioVolume = function (value) {
      if (_streamBound) _videoElement.setAudioVolume( OT.$.roundFloat(value / 100, 2) );
      else _preInitialisedVolue = value;

      return this;
    };

    this.getAudioVolume = function () {
      if (_streamBound) return parseInt(_videoElement.getAudioVolume() * 100, 10);
      else return _preInitialisedVolue || 50;
    };


    this.whenTimeIncrements = function (callback, context) {
      _videoElement.whenTimeIncrements(callback, context);
      return this;
    };

    this.onRatioAvailable = function(callabck) {
      _videoElement.onRatioAvailable(callabck) ;
      return this;
    };

    this.destroy = function () {
      
      this.off();

      _videoElement.destroy();
      return void 0;
    };
  };

  var PluginVideoElement = function PluginVideoElement (options,
                                                        errorHandler,
                                                        orientationChangedHandler) {
    var _videoProxy,
        _parentDomElement,
        _ratioAvailable = false,
        _ratioAvailableListeners = [];

    OT.$.eventing(this);

    canBeOrientatedMixin(this,
                          function() { return _videoProxy.domElement; },
                          orientationChangedHandler);

    

    this.domElement = function() {
      return _videoProxy ? _videoProxy.domElement : void 0;
    };

    this.videoWidth = function() {
      return _videoProxy ? _videoProxy.getVideoWidth() : void 0;
    };

    this.videoHeight = function() {
      return _videoProxy ? _videoProxy.getVideoHeight() : void 0;
    };

    this.imgData = function() {
      return _videoProxy ? _videoProxy.getImgData() : null;
    };

    
    this.appendTo = function(parentDomElement) {
      _parentDomElement = parentDomElement;
      return this;
    };

    
    this.bindToStream = function(webRtcStream, completion) {
      if (!_parentDomElement) {
        completion('The VideoElement must attached to a DOM node before a stream can be bound');
        return;
      }

      _videoProxy = webRtcStream._.render();
      _videoProxy.appendTo(_parentDomElement);
      _videoProxy.show(function(error) {

        if (!error) {
          _ratioAvailable = true;
          var listener;
          while ((listener = _ratioAvailableListeners.shift())) {
            listener();
          }
        }

        completion(error);
      });

      return this;
    };

    
    this.unbindStream = function() {
      

      if (_videoProxy) {
        _videoProxy.destroy();
        _parentDomElement = null;
        _videoProxy = null;
      }

      return this;
    };

    this.setAudioVolume = function(value) {
      if (_videoProxy) _videoProxy.setVolume(value);
    };

    this.getAudioVolume = function() {
      
      if (_videoProxy) return _videoProxy.getVolume();
      return DefaultAudioVolume;
    };

    
    
    this.audioChannelType = function() {
      return 'unknown';
    };

    this.whenTimeIncrements = function(callback, context) {
      
      OT.$.callAsync(OT.$.bind(callback, context));
    };

    this.onRatioAvailable = function(callback) {
      if(_ratioAvailable) {
        callback();
      } else {
        _ratioAvailableListeners.push(callback);
      }
    };

    this.destroy = function() {
      this.unbindStream();

      return void 0;
    };
  };


  var NativeDOMVideoElement = function NativeDOMVideoElement (options,
                                                              errorHandler,
                                                              orientationChangedHandler) {
    var _domElement,
        _videoElementMovedWarning = false;

    OT.$.eventing(this);

    
    var _onVideoError = OT.$.bind(function(event) {
          var reason = 'There was an unexpected problem with the Video Stream: ' +
                        videoElementErrorCodeToStr(event.target.error.code);
          errorHandler(reason, this, 'VideoElement');
        }, this),

        
        
        
        _playVideoOnPause = function() {
          if(!_videoElementMovedWarning) {
            OT.warn('Video element paused, auto-resuming. If you intended to do this, ' +
                      'use publishVideo(false) or subscribeToVideo(false) instead.');

            _videoElementMovedWarning = true;
          }

          _domElement.play();
        };


    _domElement = createNativeVideoElement(options.fallbackText, options.attributes);

    
    
    var ratioAvailable = false;
    var ratioAvailableListeners = [];
    _domElement.addEventListener('timeupdate', function timeupdateHandler() {
      var aspectRatio = _domElement.videoWidth / _domElement.videoHeight;
      if (!isNaN(aspectRatio)) {
        _domElement.removeEventListener('timeupdate', timeupdateHandler);
        ratioAvailable = true;
        var listener;
        while ((listener = ratioAvailableListeners.shift())) {
          listener();
        }
      }
    });

    _domElement.addEventListener('pause', _playVideoOnPause);

    videoContentResizesMixin(this, _domElement);

    canBeOrientatedMixin(this, function() { return _domElement; }, orientationChangedHandler);

    

    this.domElement = function() {
      return _domElement;
    };

    this.videoWidth = function() {
      return _domElement.videoWidth;
    };

    this.videoHeight = function() {
      return _domElement.videoHeight;
    };

    this.imgData = function() {
      var canvas = OT.$.createElement('canvas', {
        width: _domElement.videoWidth,
        height: _domElement.videoHeight,
        style: { display: 'none' }
      });

      document.body.appendChild(canvas);
      try {
        canvas.getContext('2d').drawImage(_domElement, 0, 0, canvas.width, canvas.height);
      } catch(err) {
        OT.warn('Cannot get image data yet');
        return null;
      }
      var imgData = canvas.toDataURL('image/png');

      OT.$.removeElement(canvas);

      return OT.$.trim(imgData.replace('data:image/png;base64,', ''));
    };

    
    this.appendTo = function(parentDomElement) {
      parentDomElement.appendChild(_domElement);
      return this;
    };

    
    this.bindToStream = function(webRtcStream, completion) {
      var _this = this;
      bindStreamToNativeVideoElement(_domElement, webRtcStream, function(err) {
        if (err) {
          completion(err);
          return;
        }

        _this.startObservingSize();

        webRtcStream.onended = function() {
          _this.trigger('mediaStopped', this);
        };


        _domElement.addEventListener('error', _onVideoError, false);
        completion(null);
      });

      return this;
    };


    
    this.unbindStream = function() {
      if (_domElement) {
        unbindNativeStream(_domElement);
      }

      this.stopObservingSize();

      return this;
    };

    this.setAudioVolume = function(value) {
      if (_domElement) _domElement.volume = value;
    };

    this.getAudioVolume = function() {
      
      if (_domElement) return _domElement.volume;
      return DefaultAudioVolume;
    };

    
    
    
    this.audioChannelType = function(type) {
      if (type !== void 0) {
        _domElement.mozAudioChannelType = type;
      }

      if ('mozAudioChannelType' in _domElement) {
        return _domElement.mozAudioChannelType;
      } else {
        return 'unknown';
      }
    };

    this.whenTimeIncrements = function(callback, context) {
      if (_domElement) {
        var lastTime, handler;
        handler = OT.$.bind(function() {
          if (_domElement) {
            if (!lastTime || lastTime >= _domElement.currentTime) {
              lastTime = _domElement.currentTime;
            } else {
              _domElement.removeEventListener('timeupdate', handler, false);
              callback.call(context, this);
            }
          }
        }, this);
        _domElement.addEventListener('timeupdate', handler, false);
      }
    };

    this.destroy = function() {
      this.unbindStream();

      if (_domElement) {
        
        
        _domElement.removeEventListener('pause', _playVideoOnPause);

        OT.$.removeElement(_domElement);
        _domElement = null;
      }

      return void 0;
    };

    this.onRatioAvailable = function(callback) {
      if(ratioAvailable) {
        callback();
      } else {
        ratioAvailableListeners.push(callback);
      }
    };
  };



  
  
  
  
  
  
  
  var canBeOrientatedMixin = function canBeOrientatedMixin (self,
                                                            getDomElementCallback,
                                                            orientationChangedHandler,
                                                            initialOrientation) {
    var _orientation = initialOrientation;

    OT.$.defineProperties(self, {
      isRotated: {
        get: function() {
          return this.orientation() &&
                    (this.orientation().videoOrientation === 270 ||
                     this.orientation().videoOrientation === 90);
        }
      },

      orientation: {
        get: function() { return _orientation; },
        set: function(orientation) {
          _orientation = orientation;

          var transform = VideoOrientationTransforms[orientation.videoOrientation] ||
                          VideoOrientationTransforms.ROTATED_NORMAL;

          switch(OT.$.env.name) {
            case 'Chrome':
            case 'Safari':
              getDomElementCallback().style.webkitTransform = transform;
              break;

            case 'IE':
              if (OT.$.env.version >= 9) {
                getDomElementCallback().style.msTransform = transform;
              }
              else {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                var radians = orientation.videoOrientation * DEGREE_TO_RADIANS,
                    element = getDomElementCallback(),
                    costheta = Math.cos(radians),
                    sintheta = Math.sin(radians);

                
                
                
                

                element.style.filter = 'progid:DXImageTransform.Microsoft.Matrix(' +
                                          'M11='+costheta+',' +
                                          'M12='+(-sintheta)+',' +
                                          'M21='+sintheta+',' +
                                          'M22='+costheta+',SizingMethod=\'auto expand\')';
              }


              break;

            default:
              
              getDomElementCallback().style.transform = transform;
          }

          orientationChangedHandler(_orientation);

        }
      },

      
      
      
      audioChannelType: {
        get: function() {
          if ('mozAudioChannelType' in this.domElement) {
            return this.domElement.mozAudioChannelType;
          } else {
            return 'unknown';
          }
        },
        set: function(type) {
          if ('mozAudioChannelType' in this.domElement) {
            this.domElement.mozAudioChannelType = type;
          }
        }
      }
    });
  };

  function createNativeVideoElement(fallbackText, attributes) {
    var videoElement = document.createElement('video');
    videoElement.setAttribute('autoplay', '');
    videoElement.innerHTML = fallbackText;

    if (attributes) {
      if (attributes.muted === true) {
        delete attributes.muted;
        videoElement.muted = 'true';
      }

      for (var key in attributes) {
        if(!attributes.hasOwnProperty(key)) {
          continue;
        }
        videoElement.setAttribute(key, attributes[key]);
      }
    }

    return videoElement;
  }


  
  var _videoErrorCodes = {};

  
  
  if (window.MediaError) {
    _videoErrorCodes[window.MediaError.MEDIA_ERR_ABORTED] = 'The fetching process for the media ' +
      'resource was aborted by the user agent at the user\'s request.';
    _videoErrorCodes[window.MediaError.MEDIA_ERR_NETWORK] = 'A network error of some description ' +
      'caused the user agent to stop fetching the media resource, after the resource was ' +
      'established to be usable.';
    _videoErrorCodes[window.MediaError.MEDIA_ERR_DECODE] = 'An error of some description ' +
      'occurred while decoding the media resource, after the resource was established to be ' +
      ' usable.';
    _videoErrorCodes[window.MediaError.MEDIA_ERR_SRC_NOT_SUPPORTED] = 'The media resource ' +
      'indicated by the src attribute was not suitable.';
  }

  function videoElementErrorCodeToStr(errorCode) {
    return _videoErrorCodes[parseInt(errorCode, 10)] || 'An unknown error occurred.';
  }

  function bindStreamToNativeVideoElement(videoElement, webRtcStream, completion) {
    var timeout,
        minVideoTracksForTimeUpdate = OT.$.env.name === 'Chrome' ? 1 : 0,
        loadedEvent = webRtcStream.getVideoTracks().length > minVideoTracksForTimeUpdate ?
          'timeupdate' : 'loadedmetadata';

    var cleanup = function cleanup () {
          clearTimeout(timeout);
          videoElement.removeEventListener(loadedEvent, onLoad, false);
          videoElement.removeEventListener('error', onError, false);
          webRtcStream.onended = null;
        },

        onLoad = function onLoad () {
          cleanup();
          completion(null);
        },

        onError = function onError (event) {
          cleanup();
          unbindNativeStream(videoElement);
          completion('There was an unexpected problem with the Video Stream: ' +
            videoElementErrorCodeToStr(event.target.error.code));
        },

        onStoppedLoading = function onStoppedLoading () {
          
          
          cleanup();
          unbindNativeStream(videoElement);
          completion('Stream ended while trying to bind it to a video element.');
        };

    videoElement.addEventListener(loadedEvent, onLoad, false);
    videoElement.addEventListener('error', onError, false);
    webRtcStream.onended = onStoppedLoading;

    
    if (videoElement.srcObject !== void 0) {
      videoElement.srcObject = webRtcStream;
    } else if (videoElement.mozSrcObject !== void 0) {
      videoElement.mozSrcObject = webRtcStream;
    } else {
      videoElement.src = window.URL.createObjectURL(webRtcStream);
    }
  }


  function unbindNativeStream(videoElement) {
    if (videoElement.srcObject !== void 0) {
      videoElement.srcObject = null;
    } else if (videoElement.mozSrcObject !== void 0) {
      videoElement.mozSrcObject = null;
    } else {
      window.URL.revokeObjectURL(videoElement.src);
    }
  }


})(window);








!(function() {


  var defaultAspectRatio = 4.0/3.0,
      miniWidth = 128,
      miniHeight = 128,
      microWidth = 64,
      microHeight = 64;

  









  function fixFitModeCover(element, containerWidth, containerHeight, intrinsicRatio, rotated) {

    var $video = OT.$('.OT_video-element', element);

    if ($video.length > 0) {

      var cssProps = {left: '', top: ''};

      if (OTPlugin.isInstalled()) {
        cssProps.width = '100%';
        cssProps.height = '100%';
      } else {
        intrinsicRatio = intrinsicRatio || defaultAspectRatio;
        intrinsicRatio = rotated ? 1 / intrinsicRatio : intrinsicRatio;

        var containerRatio = containerWidth / containerHeight;

        var enforcedVideoWidth,
            enforcedVideoHeight;

        if (rotated) {
          
          enforcedVideoHeight = containerWidth;
          enforcedVideoWidth = enforcedVideoHeight * intrinsicRatio;

          cssProps.width = enforcedVideoWidth + 'px';
          cssProps.height = enforcedVideoHeight + 'px';
          cssProps.top = (enforcedVideoWidth + containerHeight) / 2 + 'px';
        } else {
          if (intrinsicRatio < containerRatio) {
            
            enforcedVideoWidth = containerWidth;
            enforcedVideoHeight = enforcedVideoWidth / intrinsicRatio;

            cssProps.width = enforcedVideoWidth + 'px';
            cssProps.height = enforcedVideoHeight + 'px';
            cssProps.top = (-enforcedVideoHeight + containerHeight) / 2 + 'px';
          } else {
            enforcedVideoHeight = containerHeight;
            enforcedVideoWidth = enforcedVideoHeight * intrinsicRatio;

            cssProps.width = enforcedVideoWidth + 'px';
            cssProps.height = enforcedVideoHeight + 'px';
            cssProps.left = (-enforcedVideoWidth + containerWidth) / 2 + 'px';
          }
        }
      }

      $video.css(cssProps);
    }
  }

  








  function fixFitModeContain(element, containerWidth, containerHeight, intrinsicRatio, rotated) {

    var $video = OT.$('.OT_video-element', element);

    if ($video.length > 0) {

      var cssProps = {left: '', top: ''};


      if (OTPlugin.isInstalled()) {
        intrinsicRatio = intrinsicRatio || defaultAspectRatio;

        var containerRatio = containerWidth / containerHeight;

        var enforcedVideoWidth,
            enforcedVideoHeight;

        if (intrinsicRatio < containerRatio) {
          enforcedVideoHeight = containerHeight;
          enforcedVideoWidth = containerHeight * intrinsicRatio;

          cssProps.width = enforcedVideoWidth + 'px';
          cssProps.height = enforcedVideoHeight + 'px';
          cssProps.left = (containerWidth - enforcedVideoWidth) / 2 + 'px';
        } else {
          enforcedVideoWidth = containerWidth;
          enforcedVideoHeight = enforcedVideoWidth / intrinsicRatio;

          cssProps.width = enforcedVideoWidth + 'px';
          cssProps.height = enforcedVideoHeight + 'px';
          cssProps.top = (containerHeight - enforcedVideoHeight) / 2 + 'px';
        }
      } else {
        if (rotated) {
          cssProps.width = containerHeight + 'px';
          cssProps.height = containerWidth + 'px';
          cssProps.top = containerHeight + 'px';
        } else {
          cssProps.width = '100%';
          cssProps.height = '100%';
        }
      }

      $video.css(cssProps);
    }
  }

  function fixMini(container, width, height) {
    var w = parseInt(width, 10),
        h = parseInt(height, 10);

    if(w < microWidth || h < microHeight) {
      OT.$.addClass(container, 'OT_micro');
    } else {
      OT.$.removeClass(container, 'OT_micro');
    }
    if(w < miniWidth || h < miniHeight) {
      OT.$.addClass(container, 'OT_mini');
    } else {
      OT.$.removeClass(container, 'OT_mini');
    }
  }

  var getOrCreateContainer = function getOrCreateContainer(elementOrDomId, insertMode) {
    var container,
        domId;

    if (elementOrDomId && elementOrDomId.nodeName) {
      
      
      container = elementOrDomId;
      if (!container.getAttribute('id') || container.getAttribute('id').length === 0) {
        container.setAttribute('id', 'OT_' + OT.$.uuid());
      }

      domId = container.getAttribute('id');
    } else {
      
      container = OT.$('#' + elementOrDomId).get(0);
      domId = elementOrDomId || ('OT_' + OT.$.uuid());
    }

    if (!container) {
      container = OT.$.createElement('div', {id: domId});
      container.style.backgroundColor = '#000000';
      document.body.appendChild(container);
    } else {
      if(!(insertMode == null || insertMode === 'replace')) {
        var placeholder = document.createElement('div');
        placeholder.id = ('OT_' + OT.$.uuid());
        if(insertMode === 'append') {
          container.appendChild(placeholder);
          container = placeholder;
        } else if(insertMode === 'before') {
          container.parentNode.insertBefore(placeholder, container);
          container = placeholder;
        } else if(insertMode === 'after') {
          container.parentNode.insertBefore(placeholder, container.nextSibling);
          container = placeholder;
        }
      } else {
        OT.$.emptyElement(container);
      }
    }

    return container;
  };

  
  
  OT.WidgetView = function(targetElement, properties) {

    var widgetView = {};

    var container = getOrCreateContainer(targetElement, properties && properties.insertMode),
        widgetContainer = document.createElement('div'),
        oldContainerStyles = {},
        sizeObserver,
        videoElement,
        videoObserver,
        posterContainer,
        loadingContainer,
        width,
        height,
        loading = true,
        audioOnly = false,
        fitMode = 'cover',
        fixFitMode = fixFitModeCover;

    OT.$.eventing(widgetView);

    if (properties) {
      width = properties.width;
      height = properties.height;

      if (width) {
        if (typeof(width) === 'number') {
          width = width + 'px';
        }
      }

      if (height) {
        if (typeof(height) === 'number') {
          height = height + 'px';
        }
      }

      container.style.width = width ? width : '264px';
      container.style.height = height ? height : '198px';
      container.style.overflow = 'hidden';
      fixMini(container, width || '264px', height || '198px');

      if (properties.mirror === undefined || properties.mirror) {
        OT.$.addClass(container, 'OT_mirrored');
      }

      if (properties.fitMode === 'contain') {
        fitMode = 'contain';
        fixFitMode = fixFitModeContain;
      } else if (properties.fitMode !== 'cover') {
        OT.warn('Invalid fit value "' + properties.fitMode + '" passed. ' +
        'Only "contain" and "cover" can be used.');
      }
    }

    if (properties.classNames) OT.$.addClass(container, properties.classNames);

    OT.$(container).addClass('OT_loading OT_fit-mode-' + fitMode);

    OT.$.addClass(widgetContainer, 'OT_widget-container');
    widgetContainer.style.width = '100%'; 
    widgetContainer.style.height = '100%'; 
    container.appendChild(widgetContainer);

    loadingContainer = document.createElement('div');
    OT.$.addClass(loadingContainer, 'OT_video-loading');
    widgetContainer.appendChild(loadingContainer);

    posterContainer = document.createElement('div');
    OT.$.addClass(posterContainer, 'OT_video-poster');
    widgetContainer.appendChild(posterContainer);

    oldContainerStyles.width = container.offsetWidth;
    oldContainerStyles.height = container.offsetHeight;

    if (!OTPlugin.isInstalled()) {
      
      sizeObserver = OT.$(container).observeSize(
        function(size) {
          var width = size.width,
              height = size.height;

          fixMini(container, width, height);

          if (videoElement) {
            fixFitMode(widgetContainer, width, height, videoElement.aspectRatio(),
              videoElement.isRotated());
          }
        })[0];


      
      
      videoObserver = OT.$.observeNodeOrChildNodeRemoval(container, function(removedNodes) {
        if (!videoElement) return;

        
        
        var videoRemoved = OT.$.some(removedNodes, function(node) {
          return node === widgetContainer || node.nodeName === 'VIDEO';
        });

        if (videoRemoved) {
          videoElement.destroy();
          videoElement = null;
        }

        if (widgetContainer) {
          OT.$.removeElement(widgetContainer);
          widgetContainer = null;
        }

        if (sizeObserver) {
          sizeObserver.disconnect();
          sizeObserver = null;
        }

        if (videoObserver) {
          videoObserver.disconnect();
          videoObserver = null;
        }
      });
    }

    widgetView.destroy = function() {
      if (sizeObserver) {
        sizeObserver.disconnect();
        sizeObserver = null;
      }

      if (videoObserver) {
        videoObserver.disconnect();
        videoObserver = null;
      }

      if (videoElement) {
        videoElement.destroy();
        videoElement = null;
      }

      if (container) {
        OT.$.removeElement(container);
        container = null;
      }
    };

    widgetView.setBackgroundImageURI = function(bgImgURI) {
      if (bgImgURI.substr(0, 5) !== 'http:' && bgImgURI.substr(0, 6) !== 'https:') {
        if (bgImgURI.substr(0, 22) !== 'data:image/png;base64,') {
          bgImgURI = 'data:image/png;base64,' + bgImgURI;
        }
      }
      OT.$.css(posterContainer, 'backgroundImage', 'url(' + bgImgURI + ')');
      OT.$.css(posterContainer, 'backgroundSize', 'contain');
      OT.$.css(posterContainer, 'opacity', '1.0');
    };

    if (properties && properties.style && properties.style.backgroundImageURI) {
      widgetView.setBackgroundImageURI(properties.style.backgroundImageURI);
    }

    widgetView.bindVideo = function(webRTCStream, options, completion) {
      
      
      if (videoElement) {
        videoElement.destroy();
        videoElement = null;
      }

      var onError = options && options.error ? options.error : void 0;
      delete options.error;

      var video = new OT.VideoElement({attributes: options}, onError);

      
      if (options.audioVolume) video.setAudioVolume(options.audioVolume);

      
      video.audioChannelType('telephony');

      video.appendTo(widgetContainer).bindToStream(webRTCStream, function(err) {
        if (err) {
          video.destroy();
          completion(err);
          return;
        }

        videoElement = video;

        
        OT.$.css(video.domElement(), 'height', '');

        var fixFitModePartial = function() {
          fixFitMode(widgetContainer, container.offsetWidth, container.offsetHeight,
            video.aspectRatio(), video.isRotated());
        };

        video.on({
          orientationChanged: fixFitModePartial,
          videoDimensionsChanged: function(oldValue, newValue) {
            fixFitModePartial();
            widgetView.trigger('videoDimensionsChanged', oldValue, newValue);
          },
          mediaStopped: function() {
            widgetView.trigger('mediaStopped');
          }
        });

        video.onRatioAvailable(fixFitModePartial);

        completion(null, video);
      });

      OT.$.addClass(video.domElement(), 'OT_video-element');

      
      
      OT.$.css(video.domElement(), 'height', '1px');

      return video;
    };

    OT.$.defineProperties(widgetView, {

      video: {
        get: function() {
          return videoElement;
        }
      },

      showPoster: {
        get: function() {
          return !OT.$.isDisplayNone(posterContainer);
        },
        set: function(newValue) {
          if(newValue) {
            OT.$.show(posterContainer);
          } else {
            OT.$.hide(posterContainer);
          }
        }
      },

      poster: {
        get: function() {
          return OT.$.css(posterContainer, 'backgroundImage');
        },
        set: function(src) {
          OT.$.css(posterContainer, 'backgroundImage', 'url(' + src + ')');
        }
      },

      loading: {
        get: function() { return loading; },
        set: function(l) {
          loading = l;

          if (loading) {
            OT.$.addClass(container, 'OT_loading');
          } else {
            OT.$.removeClass(container, 'OT_loading');
          }
        }
      },

      audioOnly: {
        get: function() { return audioOnly; },
        set: function(a) {
          audioOnly = a;

          if (audioOnly) {
            OT.$.addClass(container, 'OT_audio-only');
          } else {
            OT.$.removeClass(container, 'OT_audio-only');
          }
        }
      },

      domId: {
        get: function() { return container.getAttribute('id'); }
      }

    });

    widgetView.domElement = container;

    widgetView.addError = function(errorMsg, helpMsg, classNames) {
      container.innerHTML = '<p>' + errorMsg +
        (helpMsg ? ' <span class="ot-help-message">' + helpMsg + '</span>' : '') +
        '</p>';
      OT.$.addClass(container, classNames || 'OT_subscriber_error');
      if(container.querySelector('p').offsetHeight > container.offsetHeight) {
        container.querySelector('span').style.display = 'none';
      }
    };

    return widgetView;
  };

})(window);







if (!OT.properties) {
  throw new Error('OT.properties does not exist, please ensure that you include a valid ' +
    'properties file.');
}

OT.useSSL = function () {
  return OT.properties.supportSSL && (window.location.protocol.indexOf('https') >= 0 ||
        window.location.protocol.indexOf('chrome-extension') >= 0);
};


OT.properties = function(properties) {
  var props = OT.$.clone(properties);

  props.debug = properties.debug === 'true' || properties.debug === true;
  props.supportSSL = properties.supportSSL === 'true' || properties.supportSSL === true;

  if (window.OTProperties) {
    
    if (window.OTProperties.cdnURL) props.cdnURL = window.OTProperties.cdnURL;
    if (window.OTProperties.cdnURLSSL) props.cdnURLSSL = window.OTProperties.cdnURLSSL;
    if (window.OTProperties.configURL) props.configURL = window.OTProperties.configURL;
    if (window.OTProperties.assetURL) props.assetURL = window.OTProperties.assetURL;
    if (window.OTProperties.cssURL) props.cssURL = window.OTProperties.cssURL;
  }

  if (!props.assetURL) {
    if (OT.useSSL()) {
      props.assetURL = props.cdnURLSSL + '/webrtc/' + props.version;
    } else {
      props.assetURL = props.cdnURL + '/webrtc/' + props.version;
    }
  }

  var isIE89 = OT.$.env.name === 'IE' && OT.$.env.version <= 9;
  if (!(isIE89 && window.location.protocol.indexOf('https') < 0)) {
    props.apiURL = props.apiURLSSL;
    props.loggingURL = props.loggingURLSSL;
  }

  if (!props.configURL) props.configURL = props.assetURL + '/js/dynamic_config.min.js';
  if (!props.cssURL) props.cssURL = props.assetURL + '/css/TB.min.css';

  return props;
}(OT.properties);



!(function() {
  

  

  var currentGuidStorage,
      currentGuid;

  var isInvalidStorage = function isInvalidStorage (storageInterface) {
    return !(OT.$.isFunction(storageInterface.get) && OT.$.isFunction(storageInterface.set));
  };

  var getClientGuid = function getClientGuid (completion) {
    if (currentGuid) {
      completion(null, currentGuid);
      return;
    }

    
    
    
    currentGuidStorage.get(completion);
  };

  




































  OT.overrideGuidStorage = function (storageInterface) {
    if (isInvalidStorage(storageInterface)) {
      throw new Error('The storageInterface argument does not seem to be valid, ' +
                                        'it must implement get and set methods');
    }

    if (currentGuidStorage === storageInterface) {
      return;
    }

    currentGuidStorage = storageInterface;

    
    
    if (currentGuid) {
      currentGuidStorage.set(currentGuid, function(error) {
        if (error) {
          OT.error('Failed to send initial Guid value (' + currentGuid +
                                ') to the newly assigned Guid Storage. The error was: ' + error);
          
        }
      });
    }
  };

  if (!OT._) OT._ = {};
  OT._.getClientGuid = function (completion) {
    getClientGuid(function(error, guid) {
      if (error) {
        completion(error);
        return;
      }

      if (!guid) {
        
        
        guid = OT.$.uuid();
        currentGuidStorage.set(guid, function(error) {
          if (error) {
            completion(error);
            return;
          }

          currentGuid = guid;
        });
      }
      else if (!currentGuid) {
        currentGuid = guid;
      }

      completion(null, currentGuid);
    });
  };


  
  
  OT.overrideGuidStorage({
    get: function(completion) {
      completion(null, OT.$.getCookie('opentok_client_id'));
    },

    set: function(guid, completion) {
      OT.$.setCookie('opentok_client_id', guid);
      completion(null);
    }
  });

})(window);





!(function() {
  

  

  var nativeGetUserMedia,
      vendorToW3CErrors,
      gumNamesToMessages,
      mapVendorErrorName,
      parseErrorEvent,
      areInvalidConstraints;

  
  nativeGetUserMedia = (function() {
    if (navigator.getUserMedia) {
      return OT.$.bind(navigator.getUserMedia, navigator);
    } else if (navigator.mozGetUserMedia) {
      return OT.$.bind(navigator.mozGetUserMedia, navigator);
    } else if (navigator.webkitGetUserMedia) {
      return OT.$.bind(navigator.webkitGetUserMedia, navigator);
    } else if (OTPlugin.isInstalled()) {
      return OT.$.bind(OTPlugin.getUserMedia, OTPlugin);
    }
  })();

  
  
  
  vendorToW3CErrors = {
    PERMISSION_DENIED: 'PermissionDeniedError',
    NOT_SUPPORTED_ERROR: 'NotSupportedError',
    MANDATORY_UNSATISFIED_ERROR: ' ConstraintNotSatisfiedError',
    NO_DEVICES_FOUND: 'NoDevicesFoundError',
    HARDWARE_UNAVAILABLE: 'HardwareUnavailableError',
    TrackStartError: 'HardwareUnavailableError'
  };

  gumNamesToMessages = {
    PermissionDeniedError: 'End-user denied permission to hardware devices',
    PermissionDismissedError: 'End-user dismissed permission to hardware devices',
    NotSupportedError: 'A constraint specified is not supported by the browser.',
    ConstraintNotSatisfiedError: 'It\'s not possible to satisfy one or more constraints ' +
      'passed into the getUserMedia function',
    OverconstrainedError: 'Due to changes in the environment, one or more mandatory ' +
      'constraints can no longer be satisfied.',
    NoDevicesFoundError: 'No voice or video input devices are available on this machine.',
    HardwareUnavailableError: 'The selected voice or video devices are unavailable. Verify ' +
      'that the chosen devices are not in use by another application.'
  };

  
  mapVendorErrorName = function mapVendorErrorName(vendorErrorName, vendorErrors) {
    var errorName, errorMessage;

    if(vendorErrors.hasOwnProperty(vendorErrorName)) {
      errorName = vendorErrors[vendorErrorName];
    } else {
      
      
      errorName = vendorErrorName;
    }

    if(gumNamesToMessages.hasOwnProperty(errorName)) {
      errorMessage = gumNamesToMessages[errorName];
    } else {
      errorMessage = 'Unknown Error while getting user media';
    }

    return {
      name: errorName,
      message: errorMessage
    };
  };

  
  
  parseErrorEvent = function parseErrorObject(event) {
    var error;

    if (OT.$.isObject(event) && event.name) {
      error = mapVendorErrorName(event.name, vendorToW3CErrors);
      error.constraintName = event.constraintName;
    } else if (typeof event === 'string') {
      error = mapVendorErrorName(event, vendorToW3CErrors);
    } else {
      error = {
        message: 'Unknown Error type while getting media'
      };
    }

    return error;
  };

  
  
  areInvalidConstraints = function(constraints) {
    if (!constraints || !OT.$.isObject(constraints)) return true;

    for (var key in constraints) {
      if(!constraints.hasOwnProperty(key)) {
        continue;
      }
      if (constraints[key]) return false;
    }

    return true;
  };


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  OT.$.getUserMedia = function(constraints, success, failure, accessDialogOpened,
    accessDialogClosed, accessDenied, customGetUserMedia) {

    var getUserMedia = nativeGetUserMedia;

    if(OT.$.isFunction(customGetUserMedia)) {
      getUserMedia = customGetUserMedia;
    }

    
    
    if (areInvalidConstraints(constraints)) {
      OT.error('Couldn\'t get UserMedia: All constraints were false');
      
      failure.call(null, {
        name: 'NO_VALID_CONSTRAINTS',
        message: 'Video and Audio was disabled, you need to enabled at least one'
      });

      return;
    }

    var triggerOpenedTimer = null,
        displayedPermissionDialog = false,

        finaliseAccessDialog = function() {
          if (triggerOpenedTimer) {
            clearTimeout(triggerOpenedTimer);
          }

          if (displayedPermissionDialog && accessDialogClosed) accessDialogClosed();
        },

        triggerOpened = function() {
          triggerOpenedTimer = null;
          displayedPermissionDialog = true;

          if (accessDialogOpened) accessDialogOpened();
        },

        onStream = function(stream) {
          finaliseAccessDialog();
          success.call(null, stream);
        },

        onError = function(event) {
          finaliseAccessDialog();
          var error = parseErrorEvent(event);

          
          if (error.name === 'PermissionDeniedError' || error.name === 'PermissionDismissedError') {
            accessDenied.call(null, error);
          } else {
            failure.call(null, error);
          }
        };

    try {
      getUserMedia(constraints, onStream, onError);
    } catch (e) {
      OT.error('Couldn\'t get UserMedia: ' + e.toString());
      onError();
      return;
    }

    
    
    
    
    
    
    
    
    
    
    
    if (location.protocol.indexOf('https') === -1) {
      
      
      triggerOpenedTimer = setTimeout(triggerOpened, 100);

    } else {
      
      triggerOpenedTimer = setTimeout(triggerOpened, 500);
    }
  };

})();







!(function() {

  var adjustModal = function(callback) {
    return function setFullHeightDocument(window, document) {
      
      document.querySelector('html').style.height = document.body.style.height = '100%';
      callback(window, document);
    };
  };

  var addCss = function(document, url, callback) {
    var head = document.head || document.getElementsByTagName('head')[0];
    var cssTag = OT.$.createElement('link', {
      type: 'text/css',
      media: 'screen',
      rel: 'stylesheet',
      href: url
    });
    head.appendChild(cssTag);
    OT.$.on(cssTag, 'error', function(error) {
      OT.error('Could not load CSS for dialog', url, error && error.message || error);
    });
    OT.$.on(cssTag, 'load', callback);
  };

  var addDialogCSS = function(document, urls, callback) {
    var allURLs = [
      '//fonts.googleapis.com/css?family=Didact+Gothic',
      OT.properties.cssURL
    ].concat(urls);
    var remainingStylesheets = allURLs.length;
    OT.$.forEach(allURLs, function(stylesheetUrl) {
      addCss(document, stylesheetUrl, function() {
        if(--remainingStylesheets <= 0) {
          callback();
        }
      });
    });

  };

  var templateElement = function(classes, children, tagName) {
    var el = OT.$.createElement(tagName || 'div', { 'class': classes }, children, this);
    el.on = OT.$.bind(OT.$.on, OT.$, el);
    el.off = OT.$.bind(OT.$.off, OT.$, el);
    return el;
  };

  var checkBoxElement = function (classes, nameAndId, onChange) {
    var checkbox = templateElement.call(this, '', null, 'input');
    checkbox = OT.$(checkbox).on('change', onChange);

    if (OT.$.env.name === 'IE' && OT.$.env.version <= 8) {
      
      checkbox.on('click', function() {
        checkbox.first.blur();
        checkbox.first.focus();
      });
    }

    checkbox.attr({
      name: nameAndId,
      id: nameAndId,
      type: 'checkbox'
    });

    return checkbox.first;
  };

  var linkElement = function(children, href, classes) {
    var link = templateElement.call(this, classes || '', children, 'a');
    link.setAttribute('href', href);
    return link;
  };

  OT.Dialogs = {};

  OT.Dialogs.Plugin = {};

  OT.Dialogs.Plugin.promptToInstall = function() {
    var modal = new OT.$.Modal(adjustModal(function(window, document) {

      var el = OT.$.bind(templateElement, document),
          btn = function(children, size) {
            var classes = 'OT_dialog-button ' +
                          (size ? 'OT_dialog-button-' + size : 'OT_dialog-button-large'),
                b = el(classes, children);

            b.enable = function() {
              OT.$.removeClass(this, 'OT_dialog-button-disabled');
              return this;
            };

            b.disable = function() {
              OT.$.addClass(this, 'OT_dialog-button-disabled');
              return this;
            };

            return b;
          },
          downloadButton = btn('Download plugin'),
          cancelButton = btn('cancel', 'small'),
          refreshButton = btn('Refresh browser'),
          acceptEULA,
          checkbox,
          close,
          root;

      OT.$.addClass(cancelButton, 'OT_dialog-no-natural-margin OT_dialog-button-block');
      OT.$.addClass(refreshButton, 'OT_dialog-no-natural-margin');

      function onDownload() {
        modal.trigger('download');
        setTimeout(function() {
          root.querySelector('.OT_dialog-messages-main').innerHTML =
                                              'Plugin installation successful';
          var sections = root.querySelectorAll('.OT_dialog-section');
          OT.$.addClass(sections[0], 'OT_dialog-hidden');
          OT.$.removeClass(sections[1], 'OT_dialog-hidden');
        }, 3000);
      }

      function onRefresh() {
        modal.trigger('refresh');
      }

      function onToggleEULA() {
        if (checkbox.checked) {
          enableButtons();
        }
        else {
          disableButtons();
        }
      }

      function enableButtons() {
        downloadButton.enable();
        downloadButton.on('click', onDownload);

        refreshButton.enable();
        refreshButton.on('click', onRefresh);
      }

      function disableButtons() {
        downloadButton.disable();
        downloadButton.off('click', onDownload);

        refreshButton.disable();
        refreshButton.off('click', onRefresh);
      }

      downloadButton.disable();
      refreshButton.disable();

      cancelButton.on('click', function() {
        modal.trigger('cancelButtonClicked');
        modal.close();
      });

      close = el('OT_closeButton', '&times;')
        .on('click', function() {
          modal.trigger('closeButtonClicked');
          modal.close();
        }).first;

      var protocol = (window.location.protocol.indexOf('https') >= 0 ? 'https' : 'http');
      acceptEULA = linkElement.call(document,
                                    'end-user license agreement',
                                    protocol + '://tokbox.com/support/ie-eula');

      checkbox = checkBoxElement.call(document, null, 'acceptEULA', onToggleEULA);

      root = el('OT_dialog-centering', [
        el('OT_dialog-centering-child', [
          el('OT_root OT_dialog OT_dialog-plugin-prompt', [
            close,
            el('OT_dialog-messages', [
              el('OT_dialog-messages-main', 'This app requires real-time communication')
            ]),
            el('OT_dialog-section', [
              el('OT_dialog-single-button-with-title', [
                el('OT_dialog-button-title', [
                  checkbox,
                  (function() {
                    var x = el('', 'accept', 'label');
                    x.setAttribute('for', checkbox.id);
                    x.style.margin = '0 5px';
                    return x;
                  })(),
                  acceptEULA
                ]),
                el('OT_dialog-actions-card', [
                  downloadButton,
                  cancelButton
                ])
              ])
            ]),
            el('OT_dialog-section OT_dialog-hidden', [
              el('OT_dialog-button-title', [
                'You can now enjoy webRTC enabled video via Internet Explorer.'
              ]),
              refreshButton
            ])
          ])
        ])
      ]);

      addDialogCSS(document, [], function() {
        document.body.appendChild(root);
      });

    }));
    return modal;
  };

  OT.Dialogs.Plugin.promptToReinstall = function() {
    var modal = new OT.$.Modal(adjustModal(function(window, document) {

      var el = OT.$.bind(templateElement, document),
          close,
          okayButton,
          root;

      close = el('OT_closeButton', '&times;')
                .on('click', function() {
                  modal.trigger('closeButtonClicked');
                  modal.close();
                }).first;

      okayButton =
        el('OT_dialog-button OT_dialog-button-large OT_dialog-no-natural-margin', 'Okay')
          .on('click', function() {
            modal.trigger('okay');
          }).first;

      root = el('OT_dialog-centering', [
        el('OT_dialog-centering-child', [
          el('OT_ROOT OT_dialog OT_dialog-plugin-reinstall', [
            close,
            el('OT_dialog-messages', [
              el('OT_dialog-messages-main', 'Reinstall Opentok Plugin'),
              el('OT_dialog-messages-minor', 'Uh oh! Try reinstalling the OpenTok plugin again ' +
                'to enable real-time video communication for Internet Explorer.')
            ]),
            el('OT_dialog-section', [
              el('OT_dialog-single-button', okayButton)
            ])
          ])
        ])
      ]);

      addDialogCSS(document, [], function() {
        document.body.appendChild(root);
      });

    }));

    return modal;
  };

  OT.Dialogs.Plugin.updateInProgress = function() {

    var progressBar,
        progressText,
        progressValue = 0;

    var modal = new OT.$.Modal(adjustModal(function(window, document) {

      var el = OT.$.bind(templateElement, document),
          root;

      progressText = el('OT_dialog-plugin-upgrade-percentage', '0%', 'strong');

      progressBar = el('OT_dialog-progress-bar-fill');

      root = el('OT_dialog-centering', [
        el('OT_dialog-centering-child', [
          el('OT_ROOT OT_dialog OT_dialog-plugin-upgrading', [
            el('OT_dialog-messages', [
              el('OT_dialog-messages-main', [
                'One moment please... ',
                progressText
              ]),
              el('OT_dialog-progress-bar', progressBar),
              el('OT_dialog-messages-minor OT_dialog-no-natural-margin',
                'Please wait while the OpenTok plugin is updated')
            ])
          ])
        ])
      ]);

      addDialogCSS(document, [], function() {
        document.body.appendChild(root);
        if(progressValue != null) {
          modal.setUpdateProgress(progressValue);
        }
      });
    }));

    modal.setUpdateProgress = function(newProgress) {
      if(progressBar && progressText) {
        if(newProgress > 99) {
          OT.$.css(progressBar, 'width', '');
          progressText.innerHTML = '100%';
        } else if(newProgress < 1) {
          OT.$.css(progressBar, 'width', '0%');
          progressText.innerHTML = '0%';
        } else {
          OT.$.css(progressBar, 'width', newProgress + '%');
          progressText.innerHTML = newProgress + '%';
        }
      } else {
        progressValue = newProgress;
      }
    };

    return modal;
  };

  OT.Dialogs.Plugin.updateComplete = function(error) {
    var modal = new OT.$.Modal(adjustModal(function(window, document) {
      var el = OT.$.bind(templateElement, document),
          reloadButton,
          root;

      reloadButton =
        el('OT_dialog-button OT_dialog-button-large OT_dialog-no-natural-margin', 'Reload')
          .on('click', function() {
            modal.trigger('reload');
          }).first;

      var msgs;

      if(error) {
        msgs = ['Update Failed.', error + '' || 'NO ERROR'];
      } else {
        msgs = ['Update Complete.',
          'The OpenTok plugin has been succesfully updated. ' +
          'Please reload your browser.'];
      }

      root = el('OT_dialog-centering', [
        el('OT_dialog-centering-child', [
          el('OT_root OT_dialog OT_dialog-plugin-upgraded', [
            el('OT_dialog-messages', [
              el('OT_dialog-messages-main', msgs[0]),
              el('OT_dialog-messages-minor', msgs[1])
            ]),
            el('OT_dialog-single-button', reloadButton)
          ])
        ])
      ]);

      addDialogCSS(document, [], function() {
        document.body.appendChild(root);
      });

    }));

    return modal;

  };


})();





!(function(window) {

  

  

  
  
  
  
  

  var chromeToW3CDeviceKinds = {
    audio: 'audioInput',
    video: 'videoInput'
  };


  OT.$.shouldAskForDevices = function(callback) {
    var MST = window.MediaStreamTrack;

    if(MST != null && OT.$.isFunction(MST.getSources)) {
      window.MediaStreamTrack.getSources(function(sources) {
        var hasAudio = sources.some(function(src) {
          return src.kind === 'audio';
        });

        var hasVideo = sources.some(function(src) {
          return src.kind === 'video';
        });

        callback.call(null, { video: hasVideo, audio: hasAudio });
      });

    } else {
      
      OT.$.shouldAskForDevices = function(callback) {
        setTimeout(OT.$.bind(callback, null, { video: true, audio: true }));
      };

      OT.$.shouldAskForDevices(callback);
    }
  };


  OT.$.getMediaDevices = function(callback) {
    if(OT.$.hasCapabilities('getMediaDevices')) {
      window.MediaStreamTrack.getSources(function(sources) {
        var filteredSources = OT.$.filter(sources, function(source) {
          return chromeToW3CDeviceKinds[source.kind] != null;
        });
        callback(void 0, OT.$.map(filteredSources, function(source) {
          return {
            deviceId: source.id,
            label: source.label,
            kind: chromeToW3CDeviceKinds[source.kind]
          };
        }));
      });
    } else {
      callback(new Error('This browser does not support getMediaDevices APIs'));
    }
  };

})(window);






var loadCSS = function loadCSS(cssURL) {
  var style = document.createElement('link');
  style.type = 'text/css';
  style.media = 'screen';
  style.rel = 'stylesheet';
  style.href = cssURL;
  var head = document.head || document.getElementsByTagName('head')[0];
  head.appendChild(style);
};












OT.Config = (function() {
  var _loaded = false,
      _global = {},
      _partners = {},
      _script,
      _head = document.head || document.getElementsByTagName('head')[0],
      _loadTimer,

      _clearTimeout = function() {
        if (_loadTimer) {
          clearTimeout(_loadTimer);
          _loadTimer = null;
        }
      },

      _cleanup = function() {
        _clearTimeout();

        if (_script) {
          _script.onload = _script.onreadystatechange = null;

          if ( _head && _script.parentNode ) {
            _head.removeChild( _script );
          }

          _script = undefined;
        }
      },

      _onLoad = function() {
        
        if (_script.readyState && !/loaded|complete/.test( _script.readyState )) {
            
          return;
        }

        _clearTimeout();

        if (!_loaded) {
          
          
          
          _this._onLoadTimeout();
        }
      },

      _onLoadError = function() {
        _cleanup();

        OT.warn('TB DynamicConfig failed to load due to an error');
        this.trigger('dynamicConfigLoadFailed');
      },

      _getModule = function(moduleName, apiKey) {
        if (apiKey && _partners[apiKey] && _partners[apiKey][moduleName]) {
          return _partners[apiKey][moduleName];
        }

        return _global[moduleName];
      },

      _this;

  _this = {
    
    loadTimeout: 4000,

    _onLoadTimeout: function() {
      _cleanup();

      OT.warn('TB DynamicConfig failed to load in ' + _this.loadTimeout + ' ms');
      this.trigger('dynamicConfigLoadFailed');
    },

    load: function(configUrl) {
      if (!configUrl) throw new Error('You must pass a valid configUrl to Config.load');

      _loaded = false;

      setTimeout(function() {
        _script = document.createElement( 'script' );
        _script.async = 'async';
        _script.src = configUrl;
        _script.onload = _script.onreadystatechange = OT.$.bind(_onLoad, _this);
        _script.onerror = OT.$.bind(_onLoadError, _this);
        _head.appendChild(_script);
      },1);

      _loadTimer = setTimeout(function() {
        _this._onLoadTimeout();
      }, this.loadTimeout);
    },


    isLoaded: function() {
      return _loaded;
    },

    reset: function() {
      this.off();
      _cleanup();
      _loaded = false;
      _global = {};
      _partners = {};
    },

    
    
    replaceWith: function(config) {
      _cleanup();

      if (!config) config = {};

      _global = config.global || {};
      _partners = config.partners || {};

      if (!_loaded) _loaded = true;
      this.trigger('dynamicConfigChanged');
    },

    
    
    
    
    
    
    
    get: function(moduleName, key, apiKey) {
      var module = _getModule(moduleName, apiKey);
      return module ? module[key] : null;
    }
  };

  OT.$.eventing(_this);

  return _this;
})();

















OT.$.registerCapability('getUserMedia', function() {
  if (OT.$.env === 'Node') return false;
  return !!(navigator.webkitGetUserMedia ||
            navigator.mozGetUserMedia ||
            OTPlugin.isInstalled());
});
















OT.$.registerCapability('PeerConnection', function() {
  if (OT.$.env === 'Node') {
    return false;
  }
  else if (typeof(window.webkitRTCPeerConnection) === 'function' &&
                    !!window.webkitRTCPeerConnection.prototype.addStream) {
    return true;
  } else if (typeof(window.mozRTCPeerConnection) === 'function' && OT.$.env.version > 20.0) {
    return true;
  } else {
    return OTPlugin.isInstalled();
  }
});







OT.$.registerCapability('webrtc', function() {
  if (OT.properties) {
    var minimumVersions = OT.properties.minimumVersion || {},
        minimumVersion = minimumVersions[OT.$.env.name.toLowerCase()];

    if(minimumVersion && OT.$.env.versionGreaterThan(minimumVersion)) {
      OT.debug('Support for', OT.$.env.name, 'is disabled because we require',
        minimumVersion, 'but this is', OT.$.env.version);
      return false;
    }
  }

  if (OT.$.env === 'Node') {
    
    return true;
  }

  return OT.$.hasCapabilities('getUserMedia', 'PeerConnection');
});











OT.$.registerCapability('bundle', function() {
  return OT.$.hasCapabilities('webrtc') &&
            (OT.$.env.name === 'Chrome' ||
              OT.$.env.name === 'Node' ||
              OTPlugin.isInstalled());
});










OT.$.registerCapability('RTCPMux', function() {
  return OT.$.hasCapabilities('webrtc') &&
              (OT.$.env.name === 'Chrome' ||
                OT.$.env.name === 'Node' ||
                OTPlugin.isInstalled());
});





OT.$.registerCapability('getMediaDevices', function() {
  return OT.$.isFunction(window.MediaStreamTrack) &&
            OT.$.isFunction(window.MediaStreamTrack.getSources);
});


OT.$.registerCapability('audioOutputLevelStat', function() {
  return OT.$.env.name === 'Chrome' || OT.$.env.name === 'IE';
});

OT.$.registerCapability('webAudioCapableRemoteStream', function() {
  return OT.$.env.name === 'Firefox';
});

OT.$.registerCapability('webAudio', function() {
  return 'AudioContext' in window;
});










OT.Analytics = function(loggingUrl) {

  var LOG_VERSION = '1';
  var _analytics = new OT.$.Analytics(loggingUrl, OT.debug, OT._.getClientGuid);

  this.logError = function(code, type, message, details, options) {
    if (!options) options = {};
    var partnerId = options.partnerId;

    if (OT.Config.get('exceptionLogging', 'enabled', partnerId) !== true) {
      return;
    }

    OT._.getClientGuid(function(error, guid) {
      if (error) {
        
        return;
      }
      var data = OT.$.extend({
        
        'clientVersion' : 'js-' + OT.properties.version.replace('v', ''),
        'guid' : guid,
        'partnerId' : partnerId,
        'source' : window.location.href,
        'logVersion' : LOG_VERSION,
        'clientSystemTime' : new Date().getTime()
      }, options);
      _analytics.logError(code, type, message, details, data);
    });

  };

  this.logEvent = function(options, throttle) {
    var partnerId = options.partnerId;

    if (!options) options = {};

    OT._.getClientGuid(function(error, guid) {
      if (error) {
        
        return;
      }

      
      var data = OT.$.extend({
        
        'clientVersion' : 'js-' + OT.properties.version.replace('v', ''),
        'guid' : guid,
        'partnerId' : partnerId,
        'source' : window.location.href,
        'logVersion' : LOG_VERSION,
        'clientSystemTime' : new Date().getTime()
      }, options);
      _analytics.logEvent(data, false, throttle);
    });
  };

  this.logQOS = function(options) {
    var partnerId = options.partnerId;

    if (!options) options = {};

    OT._.getClientGuid(function(error, guid) {
      if (error) {
        
        return;
      }

      
      var data = OT.$.extend({
        
        'clientVersion' : 'js-' + OT.properties.version.replace('v', ''),
        'guid' : guid,
        'partnerId' : partnerId,
        'source' : window.location.href,
        'logVersion' : LOG_VERSION,
        'clientSystemTime' : new Date().getTime(),
        'duration' : 0 
      }, options);

      _analytics.logQOS(data);
    });
  };
};









OT.ConnectivityAttemptPinger = function (options) {
  var _state = 'Initial',
    _previousState,
    states = ['Initial', 'Attempt',  'Success', 'Failure'],
    pingTimer,               
    PING_INTERVAL = 5000,
    PING_COUNT_TOTAL = 6,
    pingCount;

  
  var stateChanged = function(newState) {
      _state = newState;
      var invalidSequence = false;
      switch (_state) {
        case 'Attempt':
          if (_previousState !== 'Initial') {
            invalidSequence = true;
          }
          startAttemptPings();
          break;
        case 'Success':
          if (_previousState !== 'Attempt') {
            invalidSequence = true;
          }
          stopAttemptPings();
          break;
        case 'Failure':
          if (_previousState !== 'Attempt') {
            invalidSequence = true;
          }
          stopAttemptPings();
          break;
      }
      if (invalidSequence) {
        var data = options ? OT.$.clone(options) : {};
        data.action = 'Internal Error';
        data.variation = 'Non-fatal';
        data.payload = {
          debug: 'Invalid sequence: ' + options.action + ' ' +
            _previousState + ' -> ' + _state
        };
        OT.analytics.logEvent(data);
      }
    },

    setState = OT.$.statable(this, states, 'Failure', stateChanged),

    startAttemptPings = function() {
      pingCount = 0;
      pingTimer = setInterval(function() {
        if (pingCount < PING_COUNT_TOTAL) {
          var data = OT.$.extend(options, {variation: 'Attempting'});
          OT.analytics.logEvent(data);
        } else {
          stopAttemptPings();
        }
        pingCount++;
      }, PING_INTERVAL);
    },

    stopAttemptPings = function() {
      clearInterval(pingTimer);
    };

  this.setVariation = function(variation) {
    _previousState = _state;
    setState(variation);

    
    
    
    
    
  };

  this.stop = function() {
    stopAttemptPings();
  };
};


































OT.StylableComponent = function(self, initalStyles, showControls, logSetStyleWithPayload) {
  if (!self.trigger) {
    throw new Error('OT.StylableComponent is dependent on the mixin OT.$.eventing. ' +
      'Ensure that this is included in the object before StylableComponent.');
  }

  var _readOnly = false;

  
  var onStyleChange = function(key, value, oldValue) {
    if (oldValue) {
      self.trigger('styleValueChanged', key, value, oldValue);
    } else {
      self.trigger('styleValueChanged', key, value);
    }
  };

  if(showControls === false) {
    initalStyles = {
      buttonDisplayMode: 'off',
      nameDisplayMode: 'off',
      audioLevelDisplayMode: 'off'
    };

    _readOnly = true;
    logSetStyleWithPayload({
      showControls: false
    });
  }

  var _style = new Style(initalStyles, onStyleChange);






















  
  self.getStyle = function(key) {
    return _style.get(key);
  };



































































































































































  if(_readOnly) {
    self.setStyle = function() {
      OT.warn('Calling setStyle() has no effect because the' +
        'showControls option was set to false');
      return this;
    };
  } else {
    self.setStyle = function(keyOrStyleHash, value, silent) {
      var logPayload = {};
      if (typeof(keyOrStyleHash) !== 'string') {
        _style.setAll(keyOrStyleHash, silent);
        logPayload = keyOrStyleHash;
      } else {
        _style.set(keyOrStyleHash, value);
        logPayload[keyOrStyleHash] = value;
      }
      if (logSetStyleWithPayload) logSetStyleWithPayload(logPayload);
      return this;
    };
  }
};



var Style = function(initalStyles, onStyleChange) {

  var _style = {},
      _COMPONENT_STYLES,
      _validStyleValues,
      isValidStyle,
      castValue;


  _COMPONENT_STYLES = [
    'showMicButton',
    'showSpeakerButton',
    'nameDisplayMode',
    'buttonDisplayMode',
    'backgroundImageURI',
    'audioLevelDisplayMode'
  ];

  _validStyleValues = {
    buttonDisplayMode: ['auto', 'mini', 'mini-auto', 'off', 'on'],
    nameDisplayMode: ['auto', 'off', 'on'],
    audioLevelDisplayMode: ['auto', 'off', 'on'],
    showSettingsButton: [true, false],
    showMicButton: [true, false],
    backgroundImageURI: null,
    showControlBar: [true, false],
    showArchiveStatus: [true, false],
    videoDisabledDisplayMode: ['auto', 'off', 'on']
  };

  
  isValidStyle = function(key, value) {
    return key === 'backgroundImageURI' ||
      (_validStyleValues.hasOwnProperty(key) &&
        OT.$.arrayIndexOf(_validStyleValues[key], value) !== -1 );
  };

  castValue = function(value) {
    switch(value) {
      case 'true':
        return true;
      case 'false':
        return false;
      default:
        return value;
    }
  };

  
  this.getAll = function() {
    var style = OT.$.clone(_style);

    for (var key in style) {
      if(!style.hasOwnProperty(key)) {
        continue;
      }
      if (OT.$.arrayIndexOf(_COMPONENT_STYLES, key) < 0) {

        
        delete style[key];
      }
    }

    return style;
  };

  this.get = function(key) {
    if (key) {
      return _style[key];
    }

    
    return this.getAll();
  };

  
  this.setAll = function(newStyles, silent) {
    var oldValue, newValue;

    for (var key in newStyles) {
      if(!newStyles.hasOwnProperty(key)) {
        continue;
      }
      newValue = castValue(newStyles[key]);

      if (isValidStyle(key, newValue)) {
        oldValue = _style[key];

        if (newValue !== oldValue) {
          _style[key] = newValue;
          if (!silent) onStyleChange(key, newValue, oldValue);
        }

      } else {
        OT.warn('Style.setAll::Invalid style property passed ' + key + ' : ' + newValue);
      }
    }

    return this;
  };

  this.set = function(key, value) {
    OT.debug('setStyle: ' + key.toString());

    var newValue = castValue(value),
        oldValue;

    if (!isValidStyle(key, newValue)) {
      OT.warn('Style.set::Invalid style property passed ' + key + ' : ' + newValue);
      return this;
    }

    oldValue = _style[key];
    if (newValue !== oldValue) {
      _style[key] = newValue;

      onStyleChange(key, value, oldValue);
    }

    return this;
  };

  if (initalStyles) this.setAll(initalStyles, true);
};





















OT.generateSimpleStateMachine = function(initialState, states, transitions) {
  var validStates = states.slice(),
      validTransitions = OT.$.clone(transitions);

  var isValidState = function (state) {
    return OT.$.arrayIndexOf(validStates, state) !== -1;
  };

  var isValidTransition = function(fromState, toState) {
    return validTransitions[fromState] &&
      OT.$.arrayIndexOf(validTransitions[fromState], toState) !== -1;
  };

  return function(stateChangeFailed) {
    var currentState = initialState,
        previousState = null;

    this.current = currentState;

    function signalChangeFailed(message, newState) {
      stateChangeFailed({
        message: message,
        newState: newState,
        currentState: currentState,
        previousState: previousState
      });
    }

    
    function handleInvalidStateChanges(newState) {
      if (!isValidState(newState)) {
        signalChangeFailed('\'' + newState + '\' is not a valid state', newState);

        return false;
      }

      if (!isValidTransition(currentState, newState)) {
        signalChangeFailed('\'' + currentState + '\' cannot transition to \'' +
          newState + '\'', newState);

        return false;
      }

      return true;
    }


    this.set = function(newState) {
      if (!handleInvalidStateChanges(newState)) return;
      previousState = currentState;
      this.current = currentState = newState;
    };

  };
};








!(function() {


























































  var validStates,
      validTransitions,
      initialState = 'NotSubscribing';

  validStates = [
    'NotSubscribing', 'Init', 'ConnectingToPeer',
    'BindingRemoteStream', 'Subscribing', 'Failed',
    'Destroyed'
  ];

  validTransitions = {
    NotSubscribing: ['NotSubscribing', 'Init', 'Destroyed'],
    Init: ['NotSubscribing', 'ConnectingToPeer', 'BindingRemoteStream', 'Destroyed'],
    ConnectingToPeer: ['NotSubscribing', 'BindingRemoteStream', 'Failed', 'Destroyed'],
    BindingRemoteStream: ['NotSubscribing', 'Subscribing', 'Failed', 'Destroyed'],
    Subscribing: ['NotSubscribing', 'Failed', 'Destroyed'],
    Failed: ['Destroyed'],
    Destroyed: []
  };

  OT.SubscribingState = OT.generateSimpleStateMachine(initialState, validStates, validTransitions);

  OT.SubscribingState.prototype.isDestroyed = function() {
    return this.current === 'Destroyed';
  };

  OT.SubscribingState.prototype.isFailed = function() {
    return this.current === 'Failed';
  };

  OT.SubscribingState.prototype.isSubscribing = function() {
    return this.current === 'Subscribing';
  };

  OT.SubscribingState.prototype.isAttemptingToSubscribe = function() {
    return OT.$.arrayIndexOf(
      [ 'Init', 'ConnectingToPeer', 'BindingRemoteStream' ],
      this.current
    ) !== -1;
  };

})(window);








!(function() {


























































  var validStates = [
      'NotPublishing', 'GetUserMedia', 'BindingMedia', 'MediaBound',
      'PublishingToSession', 'Publishing', 'Failed',
      'Destroyed'
    ],

    validTransitions = {
      NotPublishing: ['NotPublishing', 'GetUserMedia', 'Destroyed'],
      GetUserMedia: ['BindingMedia', 'Failed', 'NotPublishing', 'Destroyed'],
      BindingMedia: ['MediaBound', 'Failed', 'NotPublishing', 'Destroyed'],
      MediaBound: ['NotPublishing', 'PublishingToSession', 'Failed', 'Destroyed'],
      PublishingToSession: ['NotPublishing', 'Publishing', 'Failed', 'Destroyed'],
      Publishing: ['NotPublishing', 'MediaBound', 'Failed', 'Destroyed'],
      Failed: ['Destroyed'],
      Destroyed: []
    },

    initialState = 'NotPublishing';

  OT.PublishingState = OT.generateSimpleStateMachine(initialState, validStates, validTransitions);

  OT.PublishingState.prototype.isDestroyed = function() {
    return this.current === 'Destroyed';
  };

  OT.PublishingState.prototype.isAttemptingToPublish = function() {
    return OT.$.arrayIndexOf(
      [ 'GetUserMedia', 'BindingMedia', 'MediaBound', 'PublishingToSession' ],
      this.current) !== -1;
  };

  OT.PublishingState.prototype.isPublishing = function() {
    return this.current === 'Publishing';
  };

})(window);




!(function() {











  OT.Microphone = function(webRTCStream, muted) {
    var _muted;

    OT.$.defineProperties(this, {
      muted: {
        get: function() {
          return _muted;
        },
        set: function(muted) {
          if (_muted === muted) return;

          _muted = muted;

          var audioTracks = webRTCStream.getAudioTracks();

          for (var i=0, num=audioTracks.length; i<num; ++i) {
            audioTracks[i].setEnabled(!_muted);
          }
        }
      }
    });

    
    if (muted !== undefined) {
      this.muted(muted === true);

    } else if (webRTCStream.getAudioTracks().length) {
      this.muted(!webRTCStream.getAudioTracks()[0].enabled);

    } else {
      this.muted(false);
    }

  };

})(window);














OT.IntervalRunner = function(callback, frequency) {
  var _callback = callback,
    _frequency = frequency,
    _intervalId = null;

  this.start = function() {
    _intervalId = window.setInterval(_callback, 1000 / _frequency);
  };

  this.stop = function() {
    window.clearInterval(_intervalId);
    _intervalId = null;
  };
};



!(function() {
  

  

  























  OT.Event = OT.$.eventing.Event();
  





























  








  
  OT.Event.names = {
    
    ACTIVE: 'active',
    INACTIVE: 'inactive',
    UNKNOWN: 'unknown',

    
    PER_SESSION: 'perSession',
    PER_STREAM: 'perStream',

    
    EXCEPTION: 'exception',
    ISSUE_REPORTED: 'issueReported',

    
    SESSION_CONNECTED: 'sessionConnected',
    SESSION_DISCONNECTED: 'sessionDisconnected',
    STREAM_CREATED: 'streamCreated',
    STREAM_DESTROYED: 'streamDestroyed',
    CONNECTION_CREATED: 'connectionCreated',
    CONNECTION_DESTROYED: 'connectionDestroyed',
    SIGNAL: 'signal',
    STREAM_PROPERTY_CHANGED: 'streamPropertyChanged',
    MICROPHONE_LEVEL_CHANGED: 'microphoneLevelChanged',


    
    RESIZE: 'resize',
    SETTINGS_BUTTON_CLICK: 'settingsButtonClick',
    DEVICE_INACTIVE: 'deviceInactive',
    INVALID_DEVICE_NAME: 'invalidDeviceName',
    ACCESS_ALLOWED: 'accessAllowed',
    ACCESS_DENIED: 'accessDenied',
    ACCESS_DIALOG_OPENED: 'accessDialogOpened',
    ACCESS_DIALOG_CLOSED: 'accessDialogClosed',
    ECHO_CANCELLATION_MODE_CHANGED: 'echoCancellationModeChanged',
    MEDIA_STOPPED: 'mediaStopped',
    PUBLISHER_DESTROYED: 'destroyed',

    
    SUBSCRIBER_DESTROYED: 'destroyed',

    
    DEVICES_DETECTED: 'devicesDetected',

    
    DEVICES_SELECTED: 'devicesSelected',
    CLOSE_BUTTON_CLICK: 'closeButtonClick',

    MICLEVEL : 'microphoneActivityLevel',
    MICGAINCHANGED : 'microphoneGainChanged',

    
    ENV_LOADED: 'envLoaded',
    ENV_UNLOADED: 'envUnloaded',

    
    AUDIO_LEVEL_UPDATED: 'audioLevelUpdated'
  };

  OT.ExceptionCodes = {
    JS_EXCEPTION: 2000,
    AUTHENTICATION_ERROR: 1004,
    INVALID_SESSION_ID: 1005,
    CONNECT_FAILED: 1006,
    CONNECT_REJECTED: 1007,
    CONNECTION_TIMEOUT: 1008,
    NOT_CONNECTED: 1010,
    P2P_CONNECTION_FAILED: 1013,
    API_RESPONSE_FAILURE: 1014,
    TERMS_OF_SERVICE_FAILURE: 1026,
    UNABLE_TO_PUBLISH: 1500,
    UNABLE_TO_SUBSCRIBE: 1501,
    UNABLE_TO_FORCE_DISCONNECT: 1520,
    UNABLE_TO_FORCE_UNPUBLISH: 1530
  };

  



























































































































































































  OT.ExceptionEvent = function (type, message, title, code, component, target) {
    OT.Event.call(this, type);

    this.message = message;
    this.title = title;
    this.code = code;
    this.component = component;
    this.target = target;
  };


  OT.IssueReportedEvent = function (type, issueId) {
    OT.Event.call(this, type);
    this.issueId = issueId;
  };

  
  OT.EnvLoadedEvent = function (type) {
    OT.Event.call(this, type);
  };


















































































  var connectionEventPluralDeprecationWarningShown = false;
  OT.ConnectionEvent = function (type, connection, reason) {
    OT.Event.call(this, type, false);

    if (OT.$.canDefineProperty) {
      Object.defineProperty(this, 'connections', {
        get: function() {
          if(!connectionEventPluralDeprecationWarningShown) {
            OT.warn('OT.ConnectionEvent connections property is deprecated, ' +
              'use connection instead.');
            connectionEventPluralDeprecationWarningShown = true;
          }
          return [connection];
        }
      });
    } else {
      this.connections = [connection];
    }

    this.connection = connection;
    this.reason = reason;
  };










































































































  var streamEventPluralDeprecationWarningShown = false;
  OT.StreamEvent = function (type, stream, reason, cancelable) {
    OT.Event.call(this, type, cancelable);

    if (OT.$.canDefineProperty) {
      Object.defineProperty(this, 'streams', {
        get: function() {
          if(!streamEventPluralDeprecationWarningShown) {
            OT.warn('OT.StreamEvent streams property is deprecated, use stream instead.');
            streamEventPluralDeprecationWarningShown = true;
          }
          return [stream];
        }
      });
    } else {
      this.streams = [stream];
    }

    this.stream = stream;
    this.reason = reason;
  };





















































  var sessionConnectedConnectionsDeprecationWarningShown = false;
  var sessionConnectedStreamsDeprecationWarningShown = false;
  var sessionConnectedArchivesDeprecationWarningShown = false;

  OT.SessionConnectEvent = function (type) {
    OT.Event.call(this, type, false);
    if (OT.$.canDefineProperty) {
      Object.defineProperties(this, {
        connections: {
          get: function() {
            if(!sessionConnectedConnectionsDeprecationWarningShown) {
              OT.warn('OT.SessionConnectedEvent no longer includes connections. Listen ' +
                'for connectionCreated events instead.');
              sessionConnectedConnectionsDeprecationWarningShown = true;
            }
            return [];
          }
        },
        streams: {
          get: function() {
            if(!sessionConnectedStreamsDeprecationWarningShown) {
              OT.warn('OT.SessionConnectedEvent no longer includes streams. Listen for ' +
                'streamCreated events instead.');
              sessionConnectedConnectionsDeprecationWarningShown = true;
            }
            return [];
          }
        },
        archives: {
          get: function() {
            if(!sessionConnectedArchivesDeprecationWarningShown) {
              OT.warn('OT.SessionConnectedEvent no longer includes archives. Listen for ' +
                'archiveStarted events instead.');
              sessionConnectedArchivesDeprecationWarningShown = true;
            }
            return [];
          }
        }
      });
    } else {
      this.connections = [];
      this.streams = [];
      this.archives = [];
    }
  };











































  OT.SessionDisconnectEvent = function (type, reason, cancelable) {
    OT.Event.call(this, type, cancelable);
    this.reason = reason;
  };
























































  OT.StreamPropertyChangedEvent = function (type, stream, changedProperty, oldValue, newValue) {
    OT.Event.call(this, type, false);
    this.type = type;
    this.stream = stream;
    this.changedProperty = changedProperty;
    this.oldValue = oldValue;
    this.newValue = newValue;
  };

  OT.VideoDimensionsChangedEvent = function (target, oldValue, newValue) {
    OT.Event.call(this, 'videoDimensionsChanged', false);
    this.type = 'videoDimensionsChanged';
    this.target = target;
    this.oldValue = oldValue;
    this.newValue = newValue;
  };














  OT.ArchiveEvent = function (type, archive) {
    OT.Event.call(this, type, false);
    this.type = type;
    this.id = archive.id;
    this.name = archive.name;
    this.status = archive.status;
    this.archive = archive;
  };

  OT.ArchiveUpdatedEvent = function (stream, key, oldValue, newValue) {
    OT.Event.call(this, 'updated', false);
    this.target = stream;
    this.changedProperty = key;
    this.oldValue = oldValue;
    this.newValue = newValue;
  };














  OT.SignalEvent = function(type, data, from) {
    OT.Event.call(this, type ? 'signal:' + type : OT.Event.names.SIGNAL, false);
    this.data = data;
    this.from = from;
  };

  OT.StreamUpdatedEvent = function (stream, key, oldValue, newValue) {
    OT.Event.call(this, 'updated', false);
    this.target = stream;
    this.changedProperty = key;
    this.oldValue = oldValue;
    this.newValue = newValue;
  };

  OT.DestroyedEvent = function(type, target, reason) {
    OT.Event.call(this, type, false);
    this.target = target;
    this.reason = reason;
  };
























































  OT.VideoEnabledChangedEvent = function(type, properties) {
    OT.Event.call(this, type, false);
    this.reason = properties.reason;
  };

  OT.VideoDisableWarningEvent = function(type) {
    OT.Event.call(this, type, false);
  };












  OT.AudioLevelUpdatedEvent = function(audioLevel) {
    OT.Event.call(this, OT.Event.names.AUDIO_LEVEL_UPDATED, false);
    this.audioLevel = audioLevel;
  };

  OT.MediaStoppedEvent = function(target) {
    OT.Event.call(this, OT.Event.names.MEDIA_STOPPED, true);
    this.target = target;
  };

})(window);




var screenSharingExtensionByKind = {},
    screenSharingExtensionClasses = {};

OT.registerScreenSharingExtensionHelper = function(kind, helper) {
  screenSharingExtensionClasses[kind] = helper;
  if (helper.autoRegisters && helper.isSupportedInThisBrowser) {
    OT.registerScreenSharingExtension(kind);
  }
};




















OT.registerScreenSharingExtension = function(kind) {
  var initArgs = Array.prototype.slice.call(arguments, 1);

  if (screenSharingExtensionClasses[kind] == null) {
    throw new Error('Unsupported kind passed to OT.registerScreenSharingExtension');
  }

  var x = screenSharingExtensionClasses[kind]
          .register.apply(screenSharingExtensionClasses[kind], initArgs);
  screenSharingExtensionByKind[kind] = x;
};

var screenSharingPickHelper = function() {

  var foundClass = OT.$.find(OT.$.keys(screenSharingExtensionClasses), function(cls) {
    return screenSharingExtensionClasses[cls].isSupportedInThisBrowser;
  });

  if (foundClass === void 0) {
    return {};
  }

  return {
    name: foundClass,
    proto: screenSharingExtensionClasses[foundClass],
    instance: screenSharingExtensionByKind[foundClass]
  };

};

OT.pickScreenSharingHelper = function() {
  return screenSharingPickHelper();
};






















































OT.checkScreenSharingCapability = function(callback) {

  var response = {
    supported: false,
    extensionRequired: void 0,
    extensionRegistered: void 0,
    extensionInstalled: void 0,
    supportedSources: {}
  };

  

  var helper = screenSharingPickHelper();

  if (helper.name === void 0) {
    setTimeout(callback.bind(null, response));
    return;
  }

  response.supported = true;
  response.extensionRequired = helper.proto.extensionRequired ? helper.name : void 0;

  response.supportedSources = {
    screen: helper.proto.sources.screen,
    application: helper.proto.sources.application,
    window: helper.proto.sources.window,
    browser: helper.proto.sources.browser
  };

  if (!helper.instance) {
    response.extensionRegistered = false;
    if (response.extensionRequired) {
      response.extensionInstalled = false;
    }
    setTimeout(callback.bind(null, response));
    return;
  }

  response.extensionRegistered = response.extensionRequired ? true : void 0;
  helper.instance.isInstalled(function(installed) {
    response.extensionInstalled = response.extensionRequired ? installed : void 0;
    callback(response);
  });
  
};



OT.registerScreenSharingExtensionHelper('firefox', {
  isSupportedInThisBrowser: OT.$.env.name === 'Firefox',
  autoRegisters: true,
  extensionRequired: false,
  getConstraintsShowsPermissionUI: false,
  sources: {
    screen: true,
    application: OT.$.env.name === 'Firefox' && OT.$.env.version >= 34,
    window: OT.$.env.name === 'Firefox' && OT.$.env.version >= 34,
    browser: OT.$.env.name === 'Firefox' && OT.$.env.version >= 38
  },
  register: function() {
    return {
      isInstalled: function(callback) {
        callback(true);
      },
      getConstraints: function(source, constraints, callback) {
        constraints.video = {
          mediaSource: source
        };

        
        if (constraints.browserWindow) {
          constraints.video.browserWindow = constraints.browserWindow;
          delete constraints.browserWindow;
        }
        if (typeof constraints.scrollWithPage !== 'undefined') {
          constraints.video.scrollWithPage = constraints.scrollWithPage;
          delete constraints.scrollWithPage;
        }

        callback(void 0, constraints);
      }
    };
  }
});

OT.registerScreenSharingExtensionHelper('chrome', {
  isSupportedInThisBrowser: !!navigator.webkitGetUserMedia && typeof chrome !== 'undefined',
  autoRegisters: false,
  extensionRequired: true,
  getConstraintsShowsPermissionUI: true,
  sources: {
    screen: true,
    application: false,
    window: false,
    browser: false
  },
  register: function (extensionID) {
    if(!extensionID) {
      throw new Error('initChromeScreenSharingExtensionHelper: extensionID is required.');
    }

    var isChrome = !!navigator.webkitGetUserMedia && typeof chrome !== 'undefined',
        callbackRegistry = {},
        isInstalled = void 0;

    var prefix = 'com.tokbox.screenSharing.' + extensionID;
    var request = function(method, payload) {
      var res = { payload: payload, from: 'jsapi' };
      res[prefix] = method;
      return res;
    };

    var addCallback = function(fn, timeToWait) {
      var requestId = OT.$.uuid(),
          timeout;
      callbackRegistry[requestId] = function() {
        clearTimeout(timeout);
        timeout = null;
        fn.apply(null, arguments);
      };
      if(timeToWait) {
        timeout = setTimeout(function() {
          delete callbackRegistry[requestId];
          fn(new Error('Timeout waiting for response to request.'));
        }, timeToWait);
      }
      return requestId;
    };

    var isAvailable = function(callback) {
      if(!callback) {
        throw new Error('isAvailable: callback is required.');
      }

      if(!isChrome) {
        setTimeout(callback.bind(null, false));
      }

      if(isInstalled !== void 0) {
        setTimeout(callback.bind(null, isInstalled));
      } else {
        var requestId = addCallback(function(error, event) {
          if(isInstalled !== true) {
            isInstalled = (event === 'extensionLoaded');
          }
          callback(isInstalled);
        }, 2000);
        var post = request('isExtensionInstalled', { requestId: requestId });
        window.postMessage(post, '*');
      }
    };

    var getConstraints = function(source, constraints, callback) {
      if(!callback) {
        throw new Error('getSourceId: callback is required');
      }
      isAvailable(function(isInstalled) {
        if(isInstalled) {
          var requestId = addCallback(function(error, event, payload) {
            if(event === 'permissionDenied') {
              callback(new Error('PermissionDeniedError'));
            } else {
              if (!constraints.video) {
                constraints.video = {};
              }
              if (!constraints.video.mandatory) {
                constraints.video.mandatory = {};
              }
              constraints.video.mandatory.chromeMediaSource = 'desktop';
              constraints.video.mandatory.chromeMediaSourceId = payload.sourceId;
              callback(void 0, constraints);
            }
          });
          window.postMessage(request('getSourceId', { requestId: requestId, source: source }), '*');
        } else {
          callback(new Error('Extension is not installed'));
        }
      });
    };

    window.addEventListener('message', function(event) {

      if (event.origin !== window.location.origin) {
        return;
      }

      if(!(event.data != null && typeof event.data === 'object')) {
        return;
      }

      if(event.data.from !== 'extension') {
        return;
      }

      var method = event.data[prefix],
          payload = event.data.payload;

      if(payload && payload.requestId) {
        var callback = callbackRegistry[payload.requestId];
        delete callbackRegistry[payload.requestId];
        if(callback) {
          callback(null, method, payload);
        }
      }

      if(method === 'extensionLoaded') {
        isInstalled = true;
      }
    });

    return {
      isInstalled: isAvailable,
      getConstraints: getConstraints
    };
  }
});


















OT.StreamChannel = function(options) {
  this.id = options.id;
  this.type = options.type;
  this.active = OT.$.castToBoolean(options.active);
  this.orientation = options.orientation || OT.VideoOrientation.ROTATED_NORMAL;
  if (options.frameRate) this.frameRate = parseFloat(options.frameRate, 10);
  this.width = parseInt(options.width, 10);
  this.height = parseInt(options.height, 10);

  
  this.source = options.source || 'camera';
  this.fitMode = options.fitMode || 'cover';

  OT.$.eventing(this, true);

  
  this.update = function(attributes) {
    var videoDimensions = {},
        oldVideoDimensions = {};

    for (var key in attributes) {
      if(!attributes.hasOwnProperty(key)) {
        continue;
      }

      
      var oldValue = this[key];

      switch(key) {
        case 'active':
          this.active = OT.$.castToBoolean(attributes[key]);
          break;

        case 'disableWarning':
          this.disableWarning = OT.$.castToBoolean(attributes[key]);
          break;

        case 'frameRate':
          this.frameRate = parseFloat(attributes[key], 10);
          break;

        case 'width':
        case 'height':
          this[key] = parseInt(attributes[key], 10);

          videoDimensions[key] = this[key];
          oldVideoDimensions[key] = oldValue;
          break;

        case 'orientation':
          this[key] = attributes[key];

          videoDimensions[key] = this[key];
          oldVideoDimensions[key] = oldValue;
          break;

        case 'fitMode':
          this[key] = attributes[key];
          break;

        case 'source':
          this[key] = attributes[key];
          break;

        default:
          OT.warn('Tried to update unknown key ' + key + ' on ' + this.type +
            ' channel ' + this.id);
          return;
      }

      this.trigger('update', this, key, oldValue, this[key]);
    }

    if (OT.$.keys(videoDimensions).length) {
      
      
      this.trigger('update', this, 'videoDimensions', oldVideoDimensions, videoDimensions);
    }

    return true;
  };
};










!(function() {

  var validPropertyNames = ['name', 'archiving'];




































































  OT.Stream = function(id, name, creationTime, connection, session, channel) {
    var destroyedReason;

    this.id = this.streamId = id;
    this.name = name;
    this.creationTime = Number(creationTime);

    this.connection = connection;
    this.channel = channel;
    this.publisher = OT.publishers.find({streamId: this.id});

    OT.$.eventing(this);

    var onChannelUpdate = OT.$.bind(function(channel, key, oldValue, newValue) {
      var _key = key;

      switch(_key) {
        case 'active':
          _key = channel.type === 'audio' ? 'hasAudio' : 'hasVideo';
          this[_key] = newValue;
          break;

        case 'disableWarning':
          _key = channel.type === 'audio' ? 'audioDisableWarning': 'videoDisableWarning';
          this[_key] = newValue;
          if (!this[channel.type === 'audio' ? 'hasAudio' : 'hasVideo']) {
            return; 
          }
          break;

        case 'fitMode':
          _key = 'defaultFitMode';
          this[_key] = newValue;
          break;

        case 'source':
          _key = channel.type === 'audio' ? 'audioType' : 'videoType';
          this[_key] = newValue;
          break;

        case 'orientation':
        case 'width':
        case 'height':
          this.videoDimensions = {
            width: channel.width,
            height: channel.height,
            orientation: channel.orientation
          };

          
          return;
      }

      this.dispatchEvent( new OT.StreamUpdatedEvent(this, _key, oldValue, newValue) );
    }, this);

    var associatedWidget = OT.$.bind(function() {
      if(this.publisher) {
        return this.publisher;
      } else {
        return OT.subscribers.find(function(subscriber) {
          return subscriber.stream.id === this.id &&
            subscriber.session.id === session.id;
        });
      }
    }, this);

    
    this.getChannelsOfType = function (type) {
      return OT.$.filter(this.channel, function(channel) {
        return channel.type === type;
      });
    };

    this.getChannel = function (id) {
      for (var i=0; i<this.channel.length; ++i) {
        if (this.channel[i].id === id) return this.channel[i];
      }

      return null;
    };


    
    
    
    

    var audioChannel = this.getChannelsOfType('audio')[0],
        videoChannel = this.getChannelsOfType('video')[0];

    
    
    this.hasAudio = audioChannel != null && audioChannel.active;
    this.hasVideo = videoChannel != null && videoChannel.active;

    this.videoType = videoChannel && videoChannel.source;
    this.defaultFitMode = videoChannel && videoChannel.fitMode;

    this.videoDimensions = {};
    if (videoChannel) {
      this.videoDimensions.width = videoChannel.width;
      this.videoDimensions.height = videoChannel.height;
      this.videoDimensions.orientation = videoChannel.orientation;

      videoChannel.on('update', onChannelUpdate);
      this.frameRate = videoChannel.frameRate;
    }

    if (audioChannel) {
      audioChannel.on('update', onChannelUpdate);
    }

    this.setChannelActiveState = function(channelType, activeState, activeReason) {
      var attributes = {
        active: activeState
      };
      if (activeReason) {
        attributes.activeReason = activeReason;
      }
      updateChannelsOfType(channelType, attributes);
    };

    this.setVideoDimensions = function(width, height) {
      updateChannelsOfType('video', {
        width: width,
        height: height,
        orientation: 0
      });
    };

    this.setRestrictFrameRate = function(restrict) {
      updateChannelsOfType('video', {
        restrictFrameRate: restrict
      });
    };

    var updateChannelsOfType = OT.$.bind(function(channelType, attributes) {
      var setChannelActiveState;
      if (!this.publisher) {
        var subscriber = OT.subscribers.find(function(subscriber) {
          return subscriber.stream && subscriber.stream.id === this.id &&
            subscriber.session.id === session.id;
        }, this);

        setChannelActiveState = function(channel) {
          session._.subscriberChannelUpdate(this, subscriber, channel, attributes);
        };
      } else {
        setChannelActiveState = function(channel) {
          session._.streamChannelUpdate(this, channel, attributes);
        };
      }

      OT.$.forEach(this.getChannelsOfType(channelType), OT.$.bind(setChannelActiveState, this));
    }, this);

    this.destroyed = false;
    this.destroyedReason = void 0;

    this.destroy = function(reason, quiet) {
      destroyedReason = reason || 'clientDisconnected';
      this.destroyed = true;
      this.destroyedReason = destroyedReason;

      if (quiet !== true) {
        this.dispatchEvent(
          new OT.DestroyedEvent(
            'destroyed',      
                              
            this,
            destroyedReason
          )
        );
      }
    };

    

    
    
    
    
    
    
    
    
    this._ = {};
    this._.updateProperty = OT.$.bind(function(key, value) {
      if (OT.$.arrayIndexOf(validPropertyNames, key) === -1) {
        OT.warn('Unknown stream property "' + key + '" was modified to "' + value + '".');
        return;
      }

      var oldValue = this[key],
          newValue = value;

      switch(key) {
        case 'name':
          this[key] = newValue;
          break;

        case 'archiving':
          var widget = associatedWidget();
          if(widget) {
            widget._.archivingStatus(newValue);
          }
          this[key] = newValue;
          break;
      }

      var event = new OT.StreamUpdatedEvent(this, key, oldValue, newValue);
      this.dispatchEvent(event);
    }, this);

    
    this._.update = OT.$.bind(function(attributes) {
      for (var key in attributes) {
        if(!attributes.hasOwnProperty(key)) {
          continue;
        }
        this._.updateProperty(key, attributes[key]);
      }
    }, this);

    this._.updateChannel = OT.$.bind(function(channelId, attributes) {
      this.getChannel(channelId).update(attributes);
    }, this);
  };

})(window);









!(function() {

  var parseErrorFromJSONDocument,
      onGetResponseCallback,
      onGetErrorCallback;

  OT.SessionInfo = function(jsonDocument) {
    var sessionJSON = jsonDocument[0];

    OT.log('SessionInfo Response:');
    OT.log(jsonDocument);

    

    this.sessionId = sessionJSON.session_id;
    this.partnerId = sessionJSON.partner_id;
    this.sessionStatus = sessionJSON.session_status;

    this.messagingServer = sessionJSON.messaging_server_url;

    this.messagingURL = sessionJSON.messaging_url;
    this.symphonyAddress = sessionJSON.symphony_address;

    this.p2pEnabled = !!(sessionJSON.properties &&
      sessionJSON.properties.p2p &&
      sessionJSON.properties.p2p.preference &&
      sessionJSON.properties.p2p.preference.value === 'enabled');
  };

  
  
  
  OT.SessionInfo.get = function(session, onSuccess, onFailure) {
    var sessionInfoURL = OT.properties.apiURL + '/session/' + session.id + '?extended=true',

        startTime = OT.$.now(),

        options,

        validateRawSessionInfo = function(sessionInfo) {
          session.logEvent('Instrumentation', null, 'gsi', OT.$.now() - startTime);
          var error = parseErrorFromJSONDocument(sessionInfo);
          if (error === false) {
            onGetResponseCallback(session, onSuccess, sessionInfo);
          } else {
            onGetErrorCallback(session, onFailure, error, JSON.stringify(sessionInfo));
          }
        };


    if(OT.$.env.name === 'IE' && OT.$.env.version < 10) {
      sessionInfoURL = sessionInfoURL + '&format=json&token=' + encodeURIComponent(session.token) +
                                        '&version=1&cache=' + OT.$.uuid();
      options = {
        xdomainrequest: true
      };
    }
    else {
      options = {
        headers: {
          'X-TB-TOKEN-AUTH': session.token,
          'X-TB-VERSION': 1
        }
      };
    }
    session.logEvent('SessionInfo', 'Attempt');
    OT.$.getJSON(sessionInfoURL, options, function(error, sessionInfo) {
      if(error) {
        var responseText = sessionInfo;
        onGetErrorCallback(session, onFailure,
          new OT.Error(error.target && error.target.status || error.code, error.message ||
            'Could not connect to the OpenTok API Server.'), responseText);
      } else {
        validateRawSessionInfo(sessionInfo);
      }
    });
  };

  var messageServerToClientErrorCodes = {};
  messageServerToClientErrorCodes['404'] = OT.ExceptionCodes.INVALID_SESSION_ID;
  messageServerToClientErrorCodes['409'] = OT.ExceptionCodes.TERMS_OF_SERVICE_FAILURE;
  messageServerToClientErrorCodes['400'] = OT.ExceptionCodes.INVALID_SESSION_ID;
  messageServerToClientErrorCodes['403'] = OT.ExceptionCodes.AUTHENTICATION_ERROR;

  
  
  parseErrorFromJSONDocument = function(jsonDocument) {
    if(OT.$.isArray(jsonDocument)) {

      var errors = OT.$.filter(jsonDocument, function(node) {
        return node.error != null;
      });

      var numErrorNodes = errors.length;
      if(numErrorNodes === 0) {
        return false;
      }

      var errorCode = errors[0].error.code;
      if (messageServerToClientErrorCodes[errorCode.toString()]) {
        errorCode = messageServerToClientErrorCodes[errorCode];
      }

      return {
        code: errorCode,
        message: errors[0].error.errorMessage && errors[0].error.errorMessage.message
      };
    } else {
      return {
        code: null,
        message: 'Unknown error: getSessionInfo JSON response was badly formed'
      };
    }
  };

  
  onGetResponseCallback = function(session, onSuccess, rawSessionInfo) {
    session.logEvent('SessionInfo', 'Success',
      {messagingServer: rawSessionInfo[0].messaging_server_url});

    onSuccess( new OT.SessionInfo(rawSessionInfo) );
  };
  

  onGetErrorCallback = function(session, onFailure, error, responseText) {
    var payload = {
      reason:'GetSessionInfo',
      code: (error.code || 'No code'),
      message: error.message + ':' +
        (responseText || 'Empty responseText from API server')
    };
    session.logConnectivityEvent('Failure', payload);

    onFailure(error, session);
  };

})(window);




!(function() {
  

  

  












































































































































































































































  OT.Error = function(code, message) {
    this.code = code;
    this.message = message;
  };

  var errorsCodesToTitle = {
    1004: 'Authentication error',
    1005: 'Invalid Session ID',
    1006: 'Connect Failed',
    1007: 'Connect Rejected',
    1008: 'Connect Time-out',
    1009: 'Security Error',
    1010: 'Not Connected',
    1011: 'Invalid Parameter',
    1012: 'Peer-to-peer Stream Play Failed',
    1013: 'Connection Failed',
    1014: 'API Response Failure',
    1015: 'Session connected, cannot test network',
    1021: 'Request Timeout',
    1026: 'Terms of Service Violation: Export Compliance',
    1500: 'Unable to Publish',
    1503: 'No TURN server found',
    1520: 'Unable to Force Disconnect',
    1530: 'Unable to Force Unpublish',
    1553: 'ICEWorkflow failed',
    1600: 'createOffer, createAnswer, setLocalDescription, setRemoteDescription',
    2000: 'Internal Error',
    2001: 'Unexpected HTTP error codes (f.e. 500)',
    4000: 'WebSocket Connection Failed',
    4001: 'WebSocket Network Disconnected'
  };

  function _exceptionHandler(component, msg, errorCode, context) {
    var title = errorsCodesToTitle[errorCode],
        contextCopy = context ? OT.$.clone(context) : {};

    OT.error('OT.exception :: title: ' + title + ' (' + errorCode + ') msg: ' + msg);

    if (!contextCopy.partnerId) contextCopy.partnerId = OT.APIKEY;

    try {
      OT.analytics.logError(errorCode, 'tb.exception', title, {details:msg}, contextCopy);

      OT.dispatchEvent(
        new OT.ExceptionEvent(OT.Event.names.EXCEPTION, msg, title, errorCode, component, component)
      );
    } catch(err) {
      OT.error('OT.exception :: Failed to dispatch exception - ' + err.toString());
      
      
    }
  }










  OT.handleJsException = function(errorMsg, code, options) {
    options = options || {};

    var context,
        session = options.session;

    if (session) {
      context = {
        sessionId: session.sessionId
      };

      if (session.isConnected()) context.connectionId = session.connection.connectionId;
      if (!options.target) options.target = session;

    } else if (options.sessionId) {
      context = {
        sessionId: options.sessionId
      };

      if (!options.target) options.target = null;
    }

    _exceptionHandler(options.target, errorMsg, code, context);
  };

  
  OT.dispatchError = function (code, message, completionHandler, session) {
    OT.error(code, message);

    if (completionHandler && OT.$.isFunction(completionHandler)) {
      completionHandler.call(null, new OT.Error(code, message));
    }

    OT.handleJsException(message, code, {
      session: session
    });
  };

})(window);





!(function() {

  

  

  
  
  
  
  
  
  
  function EnvironmentLoader() {
    var _configReady = false,

        
        
        _pluginSupported = OTPlugin.isSupported(),
        _pluginLoadAttemptComplete = _pluginSupported ? OTPlugin.isReady() : true,

        isReady = function() {
          return !OT.$.isDOMUnloaded() && OT.$.isReady() &&
                      _configReady && _pluginLoadAttemptComplete;
        },

        onLoaded = function() {
          if (isReady()) {
            OT.dispatchEvent(new OT.EnvLoadedEvent(OT.Event.names.ENV_LOADED));
          }
        },


        onDomReady = function() {
          OT.$.onDOMUnload(onDomUnload);

          
          OT.Config.load(OT.properties.configURL);

          onLoaded();
        },

        onDomUnload = function() {
          OT.dispatchEvent(new OT.EnvLoadedEvent(OT.Event.names.ENV_UNLOADED));
        },

        onPluginReady = function(err) {
          
          
          _pluginLoadAttemptComplete = true;

          if (err) {
            OT.debug('TB Plugin failed to load or was not installed');
          }

          onLoaded();
        },

        configLoaded = function() {
          _configReady = true;
          OT.Config.off('dynamicConfigChanged', configLoaded);
          OT.Config.off('dynamicConfigLoadFailed', configLoadFailed);

          onLoaded();
        },

        configLoadFailed = function() {
          configLoaded();
        };


    OT.Config.on('dynamicConfigChanged', configLoaded);
    OT.Config.on('dynamicConfigLoadFailed', configLoadFailed);

    OT.$.onDOMLoad(onDomReady);

    
    
    if (_pluginSupported) OTPlugin.ready(onPluginReady);

    this.onLoad = function(cb, context) {
      if (isReady()) {
        cb.call(context);
        return;
      }

      OT.on(OT.Event.names.ENV_LOADED, cb, context);
    };

    this.onUnload = function(cb, context) {
      if (this.isUnloaded()) {
        cb.call(context);
        return;
      }

      OT.on(OT.Event.names.ENV_UNLOADED, cb, context);
    };

    this.isUnloaded = function() {
      return OT.$.isDOMUnloaded();
    };
  }

  var EnvLoader = new EnvironmentLoader();

  OT.onLoad = function(cb, context) {
    EnvLoader.onLoad(cb, context);
  };

  OT.onUnload = function(cb, context) {
    EnvLoader.onUnload(cb, context);
  };

  OT.isUnloaded = function() {
    return EnvLoader.isUnloaded();
  };

})();










var _intervalId,
    _lastHash = document.location.hash;

OT.HAS_REQUIREMENTS = 1;
OT.NOT_HAS_REQUIREMENTS = 0;








OT.checkSystemRequirements = function() {
  OT.debug('OT.checkSystemRequirements()');

  
  var systemRequirementsMet = OT.$.hasCapabilities('websockets', 'webrtc') ||
                                    OTPlugin.isInstalled();

  systemRequirementsMet = systemRequirementsMet ?
                                    this.HAS_REQUIREMENTS : this.NOT_HAS_REQUIREMENTS;

  OT.checkSystemRequirements = function() {
    OT.debug('OT.checkSystemRequirements()');
    return systemRequirementsMet;
  };

  if(systemRequirementsMet === this.NOT_HAS_REQUIREMENTS) {
    OT.analytics.logEvent({
      action: 'checkSystemRequirements',
      variation: 'notHasRequirements',
      partnerId: OT.APIKEY,
      payload: {userAgent: OT.$.env.userAgent}
    });
  }

  return systemRequirementsMet;
};














OT.upgradeSystemRequirements = function(){
  
  OT.onLoad( function() {

    if(OTPlugin.isSupported()) {
      OT.Dialogs.Plugin.promptToInstall().on({
        download: function() {
          window.location = OTPlugin.pathToInstaller();
        },
        refresh: function() {
          location.reload();
        },
        closed: function() {}
      });
      return;
    }

    var id = '_upgradeFlash';

       
    document.body.appendChild((function() {
      var d = document.createElement('iframe');
      d.id = id;
      d.style.position = 'absolute';
      d.style.position = 'fixed';
      d.style.height = '100%';
      d.style.width = '100%';
      d.style.top = '0px';
      d.style.left = '0px';
      d.style.right = '0px';
      d.style.bottom = '0px';
      d.style.zIndex = 1000;
      try {
        d.style.backgroundColor = 'rgba(0,0,0,0.2)';
      } catch (err) {
        
        
        d.style.backgroundColor = 'transparent';
        d.setAttribute('allowTransparency', 'true');
      }
      d.setAttribute('frameBorder', '0');
      d.frameBorder = '0';
      d.scrolling = 'no';
      d.setAttribute('scrolling', 'no');

      var minimumBrowserVersion = OT.properties.minimumVersion[OT.$.env.name.toLowerCase()],
          isSupportedButOld =  minimumBrowserVersion > OT.$.env.version;
      d.src = OT.properties.assetURL + '/html/upgrade.html#' +
                        encodeURIComponent(isSupportedButOld ? 'true' : 'false') + ',' +
                        encodeURIComponent(JSON.stringify(OT.properties.minimumVersion)) + '|' +
                        encodeURIComponent(document.location.href);

      return d;
    })());

    
    
    
    if (_intervalId) clearInterval(_intervalId);
    _intervalId = setInterval(function(){
      var hash = document.location.hash,
          re = /^#?\d+&/;
      if (hash !== _lastHash && re.test(hash)) {
        _lastHash = hash;
        if (hash.replace(re, '') === 'close_window'){
          document.body.removeChild(document.getElementById(id));
          document.location.hash = '';
        }
      }
    }, 100);
  });
};






OT.ConnectionCapabilities = function(capabilitiesHash) {
  
  var castCapabilities = function(capabilitiesHash) {
    capabilitiesHash.supportsWebRTC = OT.$.castToBoolean(capabilitiesHash.supportsWebRTC);
    return capabilitiesHash;
  };

  
  var _caps = castCapabilities(capabilitiesHash);
  this.supportsWebRTC = _caps.supportsWebRTC;
};





































OT.Capabilities = function(permissions) {
  this.publish = OT.$.arrayIndexOf(permissions, 'publish') !== -1 ? 1 : 0;
  this.subscribe = OT.$.arrayIndexOf(permissions, 'subscribe') !== -1 ? 1 : 0;
  this.forceUnpublish = OT.$.arrayIndexOf(permissions, 'forceunpublish') !== -1 ? 1 : 0;
  this.forceDisconnect = OT.$.arrayIndexOf(permissions, 'forcedisconnect') !== -1 ? 1 : 0;
  this.supportsWebRTC = OT.$.hasCapabilities('webrtc') ? 1 : 0;

  this.permittedTo = function(action) {
    return this.hasOwnProperty(action) && this[action] === 1;
  };
};











































OT.Connection = function(id, creationTime, data, capabilitiesHash, permissionsHash) {
  var destroyedReason;

  this.id = this.connectionId = id;
  this.creationTime = creationTime ? Number(creationTime) : null;
  this.data = data;
  this.capabilities = new OT.ConnectionCapabilities(capabilitiesHash);
  this.permissions = new OT.Capabilities(permissionsHash);
  this.quality = null;

  OT.$.eventing(this);

  this.destroy = OT.$.bind(function(reason, quiet) {
    destroyedReason = reason || 'clientDisconnected';

    if (quiet !== true) {
      this.dispatchEvent(
        new OT.DestroyedEvent(
          'destroyed',      
                            
          this,
          destroyedReason
        )
      );
    }
  }, this);

  this.destroyed = function() {
    return destroyedReason !== void 0;
  };

  this.destroyedReason = function() {
    return destroyedReason;
  };

};

OT.Connection.fromHash = function(hash) {
  return new OT.Connection(hash.id,
                           hash.creationTime,
                           hash.data,
                           OT.$.extend(hash.capablities || {}, { supportsWebRTC: true }),
                           hash.permissions || [] );
};










!(function() {

  var MAX_SIGNAL_DATA_LENGTH = 8192,
      MAX_SIGNAL_TYPE_LENGTH = 128;

  
  
  
  
  
  
  
  
  
  
  
  OT.Signal = function(sessionId, fromConnectionId, options) {
    var isInvalidType = function(type) {
        
        
        return !/^[a-zA-Z0-9\-\._~]+$/.exec(type);
      },

      validateTo = function(toAddress) {
        if (!toAddress) {
          return {
            code: 400,
            reason: 'The signal to field was invalid. Either set it to a OT.Connection, ' +
                                'OT.Session, or omit it entirely'
          };
        }

        if ( !(toAddress instanceof OT.Connection || toAddress instanceof OT.Session) ) {
          return {
            code: 400,
            reason: 'The To field was invalid'
          };
        }

        return null;
      },

      validateType = function(type) {
        var error = null;

        if (type === null || type === void 0) {
          error = {
            code: 400,
            reason: 'The signal type was null or undefined. Either set it to a String value or ' +
              'omit it'
          };
        }
        else if (type.length > MAX_SIGNAL_TYPE_LENGTH) {
          error = {
            code: 413,
            reason: 'The signal type was too long, the maximum length of it is ' +
              MAX_SIGNAL_TYPE_LENGTH + ' characters'
          };
        }
        else if ( isInvalidType(type) ) {
          error = {
            code: 400,
            reason: 'The signal type was invalid, it can only contain letters, ' +
              'numbers, \'-\', \'_\', and \'~\'.'
          };
        }

        return error;
      },

      validateData = function(data) {
        var error = null;
        if (data === null || data === void 0) {
          error = {
            code: 400,
            reason: 'The signal data was null or undefined. Either set it to a String value or ' +
              'omit it'
          };
        }
        else {
          try {
            if (JSON.stringify(data).length > MAX_SIGNAL_DATA_LENGTH) {
              error = {
                code: 413,
                reason: 'The data field was too long, the maximum size of it is ' +
                  MAX_SIGNAL_DATA_LENGTH + ' characters'
              };
            }
          }
          catch(e) {
            error = {code: 400, reason: 'The data field was not valid JSON'};
          }
        }

        return error;
      };


    this.toRaptorMessage = function() {
      var to = this.to;

      if (to && typeof(to) !== 'string') {
        to = to.id;
      }

      return OT.Raptor.Message.signals.create(OT.APIKEY, sessionId, to, this.type, this.data);
    };

    this.toHash = function() {
      return options;
    };


    this.error = null;

    if (options) {
      if (options.hasOwnProperty('data')) {
        this.data = OT.$.clone(options.data);
        this.error = validateData(this.data);
      }

      if (options.hasOwnProperty('to')) {
        this.to = options.to;

        if (!this.error) {
          this.error = validateTo(this.to);
        }
      }

      if (options.hasOwnProperty('type')) {
        if (!this.error) {
          this.error = validateType(options.type);
        }
        this.type = options.type;
      }
    }

    this.valid = this.error === null;
  };

}(this));











function SignalError(code, message) {
  this.code = code;
  this.message = message;

  
  this.reason = message;
}



OT.Raptor.Socket = function(connectionId, widgetId, messagingSocketUrl, symphonyUrl, dispatcher) {
  var _states = ['disconnected', 'connecting', 'connected', 'error', 'disconnecting'],
      _sessionId,
      _token,
      _rumor,
      _dispatcher,
      _completion;


  
  var setState = OT.$.statable(this, _states, 'disconnected'),

      onConnectComplete = function onConnectComplete(error) {
        if (error) {
          setState('error');
        }
        else {
          setState('connected');
        }

        _completion.apply(null, arguments);
      },

      onClose = OT.$.bind(function onClose (err) {
        var reason = this.is('disconnecting') ? 'clientDisconnected' : 'networkDisconnected';

        if(err && err.code === 4001) {
          reason = 'networkTimedout';
        }

        setState('disconnected');

        _dispatcher.onClose(reason);
      }, this),

      onError = function onError () {};
      


  

  this.connect = function (token, sessionInfo, completion) {
    if (!this.is('disconnected', 'error')) {
      OT.warn('Cannot connect the Raptor Socket as it is currently connected. You should ' +
        'disconnect first.');
      return;
    }

    setState('connecting');
    _sessionId = sessionInfo.sessionId;
    _token = token;
    _completion = completion;

    var rumorChannel = '/v2/partner/' + OT.APIKEY + '/session/' + _sessionId;

    _rumor = new OT.Rumor.Socket(messagingSocketUrl, symphonyUrl);
    _rumor.onClose(onClose);
    _rumor.onMessage(OT.$.bind(_dispatcher.dispatch, _dispatcher));

    _rumor.connect(connectionId, OT.$.bind(function(error) {
      if (error) {
        onConnectComplete({
          reason: 'WebSocketConnection',
          code: error.code,
          message: error.message
        });
        return;
      }

      
      _rumor.onError(onError);

      OT.debug('Raptor Socket connected. Subscribing to ' +
        rumorChannel + ' on ' + messagingSocketUrl);

      _rumor.subscribe([rumorChannel]);

      
      var connectMessage = OT.Raptor.Message.connections.create(OT.APIKEY,
        _sessionId, _rumor.id());
      this.publish(connectMessage, {'X-TB-TOKEN-AUTH': _token}, OT.$.bind(function(error) {
        if (error) {
          error.message = 'ConnectToSession:' + error.code +
              ':Received error response to connection create message.';
          var payload = {
            reason: 'ConnectToSession',
            code: error.code,
            message: 'Received error response to connection create message.'
          };
          var event = {
            action: 'Connect',
            variation: 'Failure',
            payload: payload,
            sessionId: _sessionId,
            partnerId: OT.APIKEY,
            connectionId: connectionId
          };
          OT.analytics.logEvent(event);
          onConnectComplete(payload);
          return;
        }

        this.publish( OT.Raptor.Message.sessions.get(OT.APIKEY, _sessionId),
          function (error) {
          if (error) {
            var payload = {
              reason: 'GetSessionState',
              code: error.code,
              message: 'Received error response to session read'
            };
            var event = {
              action: 'Connect',
              variation: 'Failure',
              payload: payload,
              sessionId: _sessionId,
              partnerId: OT.APIKEY,
              connectionId: connectionId
            };
            OT.analytics.logEvent(event);
            onConnectComplete(payload);
          } else {
            onConnectComplete.apply(null, arguments);
          }
        });
      }, this));
    }, this));
  };


  this.disconnect = function (drainSocketBuffer) {
    if (this.is('disconnected')) return;

    setState('disconnecting');
    _rumor.disconnect(drainSocketBuffer);
  };

  
  
  
  
  
  
  this.publish = function (message, headers, completion) {
    if (_rumor.isNot('connected')) {
      OT.error('OT.Raptor.Socket: cannot publish until the socket is connected.' + message);
      return;
    }

    var transactionId = OT.$.uuid(),
        _headers = {},
        _completion;

    
    
    if (headers) {
      if (OT.$.isFunction(headers)) {
        _headers = {};
        _completion = headers;
      }
      else {
        _headers = headers;
      }
    }
    if (!_completion && completion && OT.$.isFunction(completion)) _completion = completion;


    if (_completion) _dispatcher.registerCallback(transactionId, _completion);

    OT.debug('OT.Raptor.Socket Publish (ID:' + transactionId + ') ');
    OT.debug(message);

    _rumor.publish([symphonyUrl], message, OT.$.extend(_headers, {
      'Content-Type': 'application/x-raptor+v2',
      'TRANSACTION-ID': transactionId,
      'X-TB-FROM-ADDRESS': _rumor.id()
    }));

    return transactionId;
  };

  
  this.streamCreate = function(name, streamId, audioFallbackEnabled, channels, minBitrate,
    maxBitrate, completion) {
    var message = OT.Raptor.Message.streams.create( OT.APIKEY,
                                                    _sessionId,
                                                    streamId,
                                                    name,
                                                    audioFallbackEnabled,
                                                    channels,
                                                    minBitrate,
                                                    maxBitrate);

    this.publish(message, function(error, message) {
      completion(error, streamId, message);
    });
  };

  this.streamDestroy = function(streamId) {
    this.publish( OT.Raptor.Message.streams.destroy(OT.APIKEY, _sessionId, streamId) );
  };

  this.streamChannelUpdate = function(streamId, channelId, attributes) {
    this.publish( OT.Raptor.Message.streamChannels.update(OT.APIKEY, _sessionId,
      streamId, channelId, attributes) );
  };

  this.subscriberCreate = function(streamId, subscriberId, channelsToSubscribeTo, completion) {
    this.publish( OT.Raptor.Message.subscribers.create(OT.APIKEY, _sessionId,
      streamId, subscriberId, _rumor.id(), channelsToSubscribeTo), completion );
  };

  this.subscriberDestroy = function(streamId, subscriberId) {
    this.publish( OT.Raptor.Message.subscribers.destroy(OT.APIKEY, _sessionId,
      streamId, subscriberId) );
  };

  this.subscriberUpdate = function(streamId, subscriberId, attributes) {
    this.publish( OT.Raptor.Message.subscribers.update(OT.APIKEY, _sessionId,
      streamId, subscriberId, attributes) );
  };

  this.subscriberChannelUpdate = function(streamId, subscriberId, channelId, attributes) {
    this.publish( OT.Raptor.Message.subscriberChannels.update(OT.APIKEY, _sessionId,
      streamId, subscriberId, channelId, attributes) );
  };

  this.forceDisconnect = function(connectionIdToDisconnect, completion) {
    this.publish( OT.Raptor.Message.connections.destroy(OT.APIKEY, _sessionId,
      connectionIdToDisconnect), completion );
  };

  this.forceUnpublish = function(streamIdToUnpublish, completion) {
    this.publish( OT.Raptor.Message.streams.destroy(OT.APIKEY, _sessionId,
      streamIdToUnpublish), completion );
  };

  this.jsepCandidate = function(streamId, candidate) {
    this.publish(
      OT.Raptor.Message.streams.candidate(OT.APIKEY, _sessionId, streamId, candidate)
    );
  };

  this.jsepCandidateP2p = function(streamId, subscriberId, candidate) {
    this.publish(
      OT.Raptor.Message.subscribers.candidate(OT.APIKEY, _sessionId, streamId,
        subscriberId, candidate)
    );
  };

  this.jsepOffer = function(uri, offerSdp) {
    this.publish( OT.Raptor.Message.offer(uri, offerSdp) );
  };

  this.jsepAnswer = function(streamId, answerSdp) {
    this.publish( OT.Raptor.Message.streams.answer(OT.APIKEY, _sessionId, streamId, answerSdp) );
  };

  this.jsepAnswerP2p = function(streamId, subscriberId, answerSdp) {
    this.publish( OT.Raptor.Message.subscribers.answer(OT.APIKEY, _sessionId, streamId,
      subscriberId, answerSdp) );
  };

  this.signal = function(options, completion, logEventFn) {
    var signal = new OT.Signal(_sessionId, _rumor.id(), options || {});

    if (!signal.valid) {
      if (completion && OT.$.isFunction(completion)) {
        completion( new SignalError(signal.error.code, signal.error.reason), signal.toHash() );
      }

      return;
    }

    this.publish( signal.toRaptorMessage(), function(err) {
      var error;
      if (err) {
        error = new SignalError(err.code, err.message);
      } else {
        var typeStr = signal.data? typeof(signal.data) : null;
        logEventFn('signal', 'send', {type: typeStr});
      }

      if (completion && OT.$.isFunction(completion)) completion(error, signal.toHash());
    });
  };

  this.id = function() {
    return _rumor && _rumor.id();
  };

  if(dispatcher == null) {
    dispatcher = new OT.Raptor.Dispatcher();
  }
  _dispatcher = dispatcher;
};





















OT.GetStatsAudioLevelSampler = function(peerConnection) {

  if (!OT.$.hasCapabilities('audioOutputLevelStat')) {
    throw new Error('The current platform does not provide the required capabilities');
  }

  var _peerConnection = peerConnection,
      _statsProperty = 'audioOutputLevel';

  





  this.sample = function(done) {
    _peerConnection.getStats(function(error, stats) {
      if (!error) {
        for (var idx = 0; idx < stats.length; idx++) {
          var stat = stats[idx];
          var audioOutputLevel = parseFloat(stat[_statsProperty]);
          if (!isNaN(audioOutputLevel)) {
            
            done(audioOutputLevel / 32768);
            return;
          }
        }
      }

      done(null);
    });
  };
};
















OT.AnalyserAudioLevelSampler = function(audioContext) {

  var _sampler = this,
      _analyser = null,
      _timeDomainData = null,
      _webRTCStream = null;

  var buildAnalyzer = function(stream) {
        var sourceNode = audioContext.createMediaStreamSource(stream);
        var analyser = audioContext.createAnalyser();
        sourceNode.connect(analyser);
        return analyser;
      };

  OT.$.defineProperties(_sampler, {
    webRTCStream: {
      get: function() {
        return _webRTCStream;
      },
      set: function(webRTCStream) {
        
        _webRTCStream = webRTCStream;
        _analyser = buildAnalyzer(_webRTCStream);
        _timeDomainData = new Uint8Array(_analyser.frequencyBinCount);
      }
    }
  });

  this.sample = function(done) {
    if (_analyser) {
      _analyser.getByteTimeDomainData(_timeDomainData);

      
      var max = 0;
      for (var idx = 0; idx < _timeDomainData.length; idx++) {
        max = Math.max(max, Math.abs(_timeDomainData[idx] - 128));
      }

      
      
      done(max / 128);
    } else {
      done(null);
    }
  };
};










OT.AudioLevelTransformer = function() {

  var _averageAudioLevel = null;

  




  this.transform = function(audioLevel) {
    if (_averageAudioLevel === null || audioLevel >= _averageAudioLevel) {
      _averageAudioLevel = audioLevel;
    } else {
      
      _averageAudioLevel = audioLevel * 0.3 + _averageAudioLevel * 0.7;
    }

    
    var logScaled = (Math.log(_averageAudioLevel) / Math.LN10) / 1.5 + 1;

    return Math.min(Math.max(logScaled, 0), 1);
  };
};












OT.audioContext = function() {
  var context = new window.AudioContext();
  OT.audioContext = function() {
    return context;
  };
  return context;
};








OT.Archive = function(id, name, status) {
  this.id = id;
  this.name = name;
  this.status = status;

  this._ = {};

  OT.$.eventing(this);

  
  this._.update = OT.$.bind(function (attributes) {
    for (var key in attributes) {
      if(!attributes.hasOwnProperty(key)) {
        continue;
      }
      var oldValue = this[key];
      this[key] = attributes[key];

      var event = new OT.ArchiveUpdatedEvent(this, key, oldValue, this[key]);
      this.dispatchEvent(event);
    }
  }, this);

  this.destroy = function() {};

};









OT.analytics = new OT.Analytics(OT.properties.loggingURL);








































OT.Subscriber = function(targetElement, options, completionHandler) {
  var _widgetId = OT.$.uuid(),
      _domId = targetElement || _widgetId,
      _container,
      _streamContainer,
      _chrome,
      _audioLevelMeter,
      _fromConnectionId,
      _peerConnection,
      _session = options.session,
      _stream = options.stream,
      _subscribeStartTime,
      _startConnectingTime,
      _properties,
      _audioVolume = 100,
      _state,
      _prevStats,
      _lastSubscribeToVideoReason,
      _audioLevelCapable =  OT.$.hasCapabilities('audioOutputLevelStat') ||
                            OT.$.hasCapabilities('webAudioCapableRemoteStream'),
      _audioLevelSampler,
      _audioLevelRunner,
      _frameRateRestricted = false,
      _connectivityAttemptPinger,
      _subscriber = this;

  _properties = OT.$.defaults({}, options, {
    showControls: true,
    fitMode: _stream.defaultFitMode || 'cover'
  });

  this.id = _domId;
  this.widgetId = _widgetId;
  this.session = _session;
  this.stream = _stream = _properties.stream;
  this.streamId = _stream.id;

  _prevStats = {
    timeStamp: OT.$.now()
  };

  if (!_session) {
    OT.handleJsException('Subscriber must be passed a session option', 2000, {
      session: _session,
      target: this
    });

    return;
  }

  OT.$.eventing(this, false);

  if (typeof completionHandler === 'function') {
    this.once('subscribeComplete', completionHandler);
  }

  if(_audioLevelCapable) {
    this.on({
      'audioLevelUpdated:added': function(count) {
        if (count === 1 && _audioLevelRunner) {
          _audioLevelRunner.start();
        }
      },
      'audioLevelUpdated:removed': function(count) {
        if (count === 0 && _audioLevelRunner) {
          _audioLevelRunner.stop();
        }
      }
    });
  }

  var logAnalyticsEvent = function(action, variation, payload, throttle) {
        var args = [{
          action: action,
          variation: variation,
          payload: payload,
          streamId: _stream ? _stream.id : null,
          sessionId: _session ? _session.sessionId : null,
          connectionId: _session && _session.isConnected() ?
            _session.connection.connectionId : null,
          partnerId: _session && _session.isConnected() ? _session.sessionInfo.partnerId : null,
          subscriberId: _widgetId,
        }];
        if (throttle) args.push(throttle);
        OT.analytics.logEvent.apply(OT.analytics, args);
      },

      logConnectivityEvent = function(variation, payload) {
        if (variation === 'Attempt' || !_connectivityAttemptPinger) {
          _connectivityAttemptPinger = new OT.ConnectivityAttemptPinger({
            action: 'Subscribe',
            sessionId: _session ? _session.sessionId : null,
            connectionId: _session && _session.isConnected() ?
              _session.connection.connectionId : null,
            partnerId: _session.isConnected() ? _session.sessionInfo.partnerId : null,
            streamId: _stream ? _stream.id : null
          });
        }
        _connectivityAttemptPinger.setVariation(variation);
        logAnalyticsEvent('Subscribe', variation, payload);
      },

      recordQOS = OT.$.bind(function(parsedStats) {
        var QoSBlob = {
          streamType : 'WebRTC',
          width: _container ? Number(OT.$.width(_container.domElement).replace('px', '')) : null,
          height: _container ? Number(OT.$.height(_container.domElement).replace('px', '')) : null,
          sessionId: _session ? _session.sessionId : null,
          connectionId: _session ? _session.connection.connectionId : null,
          mediaServerName: _session ? _session.sessionInfo.messagingServer : null,
          p2pFlag: _session ? _session.sessionInfo.p2pEnabled : false,
          partnerId: _session ? _session.apiKey : null,
          streamId: _stream.id,
          subscriberId: _widgetId,
          version: OT.properties.version,
          duration: parseInt(OT.$.now() - _subscribeStartTime, 10),
          remoteConnectionId: _stream.connection.connectionId
        };

        OT.analytics.logQOS( OT.$.extend(QoSBlob, parsedStats) );
        this.trigger('qos', parsedStats);
      }, this),


      stateChangeFailed = function(changeFailed) {
        OT.error('Subscriber State Change Failed: ', changeFailed.message);
        OT.debug(changeFailed);
      },

      onLoaded = function() {
        if (_state.isSubscribing() || !_streamContainer) return;

        OT.debug('OT.Subscriber.onLoaded');

        _state.set('Subscribing');
        _subscribeStartTime = OT.$.now();

        var payload = {
          pcc: parseInt(_subscribeStartTime - _startConnectingTime, 10),
          hasRelayCandidates: _peerConnection && _peerConnection.hasRelayCandidates()
        };
        logAnalyticsEvent('createPeerConnection', 'Success', payload);

        _container.loading(false);
        _chrome.showAfterLoading();

        if(_frameRateRestricted) {
          _stream.setRestrictFrameRate(true);
        }

        if(_audioLevelMeter && _subscriber.getStyle('audioLevelDisplayMode') === 'auto') {
          _audioLevelMeter[_container.audioOnly() ? 'show' : 'hide']();
        }

        this.trigger('subscribeComplete', null, this);
        this.trigger('loaded', this);

        logConnectivityEvent('Success', {streamId: _stream.id});
      },

      onDisconnected = function() {
        OT.debug('OT.Subscriber has been disconnected from the Publisher\'s PeerConnection');

        if (_state.isAttemptingToSubscribe()) {
          
          _state.set('Failed');
          this.trigger('subscribeComplete', new OT.Error(null, 'ClientDisconnected'));

        } else if (_state.isSubscribing()) {
          _state.set('Failed');

          
          
        }

        this.disconnect();
      },

      onPeerConnectionFailure = OT.$.bind(function(code, reason, peerConnection, prefix) {
        var payload;
        if (_state.isAttemptingToSubscribe()) {
          
          
          payload = {
            reason: prefix ? prefix : 'PeerConnectionError',
            message: 'Subscriber PeerConnection Error: ' + reason,
            hasRelayCandidates: _peerConnection && _peerConnection.hasRelayCandidates()
          };
          logAnalyticsEvent('createPeerConnection', 'Failure', payload);

          _state.set('Failed');
          this.trigger('subscribeComplete', new OT.Error(null, reason));

        } else if (_state.isSubscribing()) {
          
          _state.set('Failed');
          this.trigger('error', reason);
        }

        this.disconnect();

        payload = {
          reason: prefix ? prefix : 'PeerConnectionError',
          message: 'Subscriber PeerConnection Error: ' + reason,
          code: OT.ExceptionCodes.P2P_CONNECTION_FAILED
        };
        logConnectivityEvent('Failure', payload);

        OT.handleJsException('Subscriber PeerConnection Error: ' + reason,
          OT.ExceptionCodes.P2P_CONNECTION_FAILED, {
            session: _session,
            target: this
          }
        );
        _showError.call(this, reason);
      }, this),

      onRemoteStreamAdded = function(webRTCStream) {
        OT.debug('OT.Subscriber.onRemoteStreamAdded');

        _state.set('BindingRemoteStream');

        
        this.subscribeToAudio(_properties.subscribeToAudio);

        _lastSubscribeToVideoReason = 'loading';
        this.subscribeToVideo(_properties.subscribeToVideo, 'loading');

        var videoContainerOptions = {
          error: onPeerConnectionFailure,
          audioVolume: _audioVolume
        };

        
        
        
        
        var tracks,
            reenableVideoTrack = false;
        if (!_stream.hasVideo && OT.$.env.name === 'Chrome' && OT.$.env.version >= 35) {
          tracks = webRTCStream.getVideoTracks();
          if(tracks.length > 0) {
            tracks[0].enabled = false;
            reenableVideoTrack = tracks[0];
          }
        }

        _streamContainer = _container.bindVideo(webRTCStream,
                                            videoContainerOptions,
                                            OT.$.bind(function(err) {
          if (err) {
            onPeerConnectionFailure(null, err.message || err, _peerConnection, 'VideoElement');
            return;
          }

          
          
          if (reenableVideoTrack != null && _properties.subscribeToVideo) {
            reenableVideoTrack.enabled = true;
          }

          _streamContainer.orientation({
            width: _stream.videoDimensions.width,
            height: _stream.videoDimensions.height,
            videoOrientation: _stream.videoDimensions.orientation
          });

          onLoaded.call(this, null);
        }, this));

        if (OT.$.hasCapabilities('webAudioCapableRemoteStream') && _audioLevelSampler &&
          webRTCStream.getAudioTracks().length > 0) {
          _audioLevelSampler.webRTCStream(webRTCStream);
        }

        logAnalyticsEvent('createPeerConnection', 'StreamAdded');
        this.trigger('streamAdded', this);
      },

      onRemoteStreamRemoved = function(webRTCStream) {
        OT.debug('OT.Subscriber.onStreamRemoved');

        if (_streamContainer.stream === webRTCStream) {
          _streamContainer.destroy();
          _streamContainer = null;
        }


        this.trigger('streamRemoved', this);
      },

      streamDestroyed = function () {
        this.disconnect();
      },

      streamUpdated = function(event) {

        switch(event.changedProperty) {
          case 'videoDimensions':
            if (!_streamContainer) {
              
              break;
            }
            _streamContainer.orientation({
              width: event.newValue.width,
              height: event.newValue.height,
              videoOrientation: event.newValue.orientation
            });

            this.dispatchEvent(new OT.VideoDimensionsChangedEvent(
              this, event.oldValue, event.newValue
            ));

            break;

          case 'videoDisableWarning':
            _chrome.videoDisabledIndicator.setWarning(event.newValue);
            this.dispatchEvent(new OT.VideoDisableWarningEvent(
              event.newValue ? 'videoDisableWarning' : 'videoDisableWarningLifted'
            ));
            break;

          case 'hasVideo':

            setAudioOnly(!(_stream.hasVideo && _properties.subscribeToVideo));

            this.dispatchEvent(new OT.VideoEnabledChangedEvent(
              _stream.hasVideo ? 'videoEnabled' : 'videoDisabled', {
              reason: 'publishVideo'
            }));
            break;

          case 'hasAudio':
            
        }
      },

      

      
      
      
      chromeButtonMode = function(mode) {
        if (mode === false) return 'off';

        var defaultMode = this.getStyle('buttonDisplayMode');

        
        if (defaultMode === false) return 'on';

        
        return defaultMode;
      },

      updateChromeForStyleChange = function(key, value) {
        if (!_chrome) return;

        switch(key) {
          case 'nameDisplayMode':
            _chrome.name.setDisplayMode(value);
            _chrome.backingBar.setNameMode(value);
            break;

          case 'videoDisabledDisplayMode':
            _chrome.videoDisabledIndicator.setDisplayMode(value);
            break;

          case 'showArchiveStatus':
            _chrome.archive.setShowArchiveStatus(value);
            break;

          case 'buttonDisplayMode':
            _chrome.muteButton.setDisplayMode(value);
            _chrome.backingBar.setMuteMode(value);
            break;

          case 'audioLevelDisplayMode':
            _chrome.audioLevel.setDisplayMode(value);
            break;

          case 'bugDisplayMode':
            

          case 'backgroundImageURI':
            _container.setBackgroundImageURI(value);
        }
      },

      _createChrome = function() {

        var widgets = {
          backingBar: new OT.Chrome.BackingBar({
            nameMode: !_properties.name ? 'off' : this.getStyle('nameDisplayMode'),
            muteMode: chromeButtonMode.call(this, this.getStyle('showMuteButton'))
          }),

          name: new OT.Chrome.NamePanel({
            name: _properties.name,
            mode: this.getStyle('nameDisplayMode')
          }),

          muteButton: new OT.Chrome.MuteButton({
            muted: _properties.muted,
            mode: chromeButtonMode.call(this, this.getStyle('showMuteButton'))
          }),

          archive: new OT.Chrome.Archiving({
            show: this.getStyle('showArchiveStatus'),
            archiving: false
          })
        };

        if (_audioLevelCapable) {
          var audioLevelTransformer = new OT.AudioLevelTransformer();

          var audioLevelUpdatedHandler = function(evt) {
            _audioLevelMeter.setValue(audioLevelTransformer.transform(evt.audioLevel));
          };

          _audioLevelMeter = new OT.Chrome.AudioLevelMeter({
            mode: this.getStyle('audioLevelDisplayMode'),
            onActivate: function() {
              _subscriber.on('audioLevelUpdated', audioLevelUpdatedHandler);
            },
            onPassivate: function() {
              _subscriber.off('audioLevelUpdated', audioLevelUpdatedHandler);
            }
          });

          widgets.audioLevel = _audioLevelMeter;
        }

        widgets.videoDisabledIndicator = new OT.Chrome.VideoDisabledIndicator({
          mode: this.getStyle('videoDisabledDisplayMode')
        });

        _chrome = new OT.Chrome({
          parent: _container.domElement
        }).set(widgets).on({
          muted: function() {
            muteAudio.call(this, true);
          },

          unmuted: function() {
            muteAudio.call(this, false);
          }
        }, this);

        
        _chrome.hideWhileLoading();
      },

      _showError = function() {
        
        
        if (_container) {
          _container.addError(
            'The stream was unable to connect due to a network error.',
            'Make sure your connection isn\'t blocked by a firewall.'
          );
        }
      };
      
  OT.StylableComponent(this, {
    nameDisplayMode: 'auto',
    buttonDisplayMode: 'auto',
    audioLevelDisplayMode: 'auto',
    videoDisabledDisplayMode: 'auto',
    backgroundImageURI: null,
    showArchiveStatus: true,
    showMicButton: true
  }, _properties.showControls, function (payload) {
    logAnalyticsEvent('SetStyle', 'Subscriber', payload, 0.1);
  });

  var setAudioOnly = function(audioOnly) {
    if (_container) {
      _container.audioOnly(audioOnly);
      _container.showPoster(audioOnly);
    }

    if (_audioLevelMeter && _subscriber.getStyle('audioLevelDisplayMode') === 'auto') {
      _audioLevelMeter[audioOnly ? 'show' : 'hide']();
    }
  };

  
  var notifyGetStatsCalled = (function() {
    var callCount = 0;
    return function throttlingNotifyGetStatsCalled() {
      if (callCount % 100 === 0) {
        logAnalyticsEvent('getStats', 'Called');
      }
      callCount++;
    };
  })();

  this.destroy = function(reason, quiet) {
    if (_state.isDestroyed()) return;

    if(reason === 'streamDestroyed') {
      if (_state.isAttemptingToSubscribe()) {
        
        
        this.trigger('subscribeComplete', new OT.Error(null, 'InvalidStreamID'));
      }
    }

    _state.set('Destroyed');

    if(_audioLevelRunner) {
      _audioLevelRunner.stop();
    }

    this.disconnect();

    if (_chrome) {
      _chrome.destroy();
      _chrome = null;
    }

    if (_container) {
      _container.destroy();
      _container = null;
      this.element = null;
    }

    if (_stream && !_stream.destroyed) {
      logAnalyticsEvent('unsubscribe', null, {streamId: _stream.id});
    }

    this.id = _domId = null;
    this.stream = _stream = null;
    this.streamId = null;

    this.session =_session = null;
    _properties = null;

    if (quiet !== true) {
      this.dispatchEvent(
        new OT.DestroyedEvent(
          OT.Event.names.SUBSCRIBER_DESTROYED,
          this,
          reason
        ),
        OT.$.bind(this.off, this)
      );
    }

    return this;
  };

  this.disconnect = function() {
    if (!_state.isDestroyed() && !_state.isFailed()) {
      
      
      _state.set('NotSubscribing');
    }

    if (_streamContainer) {
      _streamContainer.destroy();
      _streamContainer = null;
    }

    if (_peerConnection) {
      _peerConnection.destroy();
      _peerConnection = null;

      logAnalyticsEvent('disconnect', 'PeerConnection', {streamId: _stream.id});
    }
  };

  this.processMessage = function(type, fromConnection, message) {
    OT.debug('OT.Subscriber.processMessage: Received ' + type + ' message from ' +
      fromConnection.id);
    OT.debug(message);

    if (_fromConnectionId !== fromConnection.id) {
      _fromConnectionId = fromConnection.id;
    }

    if (_peerConnection) {
      _peerConnection.processMessage(type, message);
    }
  };

  this.disableVideo = function(active) {
    if (!active) {
      OT.warn('Due to high packet loss and low bandwidth, video has been disabled');
    } else {
      if (_lastSubscribeToVideoReason === 'auto') {
        OT.info('Video has been re-enabled');
        _chrome.videoDisabledIndicator.disableVideo(false);
      } else {
        OT.info('Video was not re-enabled because it was manually disabled');
        return;
      }
    }
    this.subscribeToVideo(active, 'auto');
    if(!active) {
      _chrome.videoDisabledIndicator.disableVideo(true);
    }
    var payload = active ? {videoEnabled: true} : {videoDisabled: true};
    logAnalyticsEvent('updateQuality', 'video', payload);
  };

  


















  this.getImgData = function() {
    if (!this.isSubscribing()) {
      OT.error('OT.Subscriber.getImgData: Cannot getImgData before the Subscriber ' +
        'is subscribing.');
      return null;
    }

    return _streamContainer.imgData();
  };

  this.getStats = function(callback) {
    if (!_peerConnection) {
      callback(new OT.$.Error('Subscriber is not connected cannot getStats', 1015));
      return;
    }

    notifyGetStatsCalled();

    _peerConnection.getStats(function(error, stats) {
      if (error) {
        callback(error);
        return;
      }

      var otStats = {
        timestamp: 0
      };

      OT.$.forEach(stats, function(stat) {
        if (OT.getStatsHelpers.isInboundStat(stat)) {
          var video = OT.getStatsHelpers.isVideoStat(stat);
          var audio = OT.getStatsHelpers.isAudioStat(stat);

          
          
          if (audio || video) {
            otStats.timestamp = OT.getStatsHelpers.normalizeTimestamp(stat.timestamp);
          }
          if (video) {
            otStats.video = OT.getStatsHelpers.parseStatCategory(stat);
          } else if (audio) {
            otStats.audio = OT.getStatsHelpers.parseStatCategory(stat);
          }
        }
      });

      callback(null, otStats);
    });
  };

  


















  this.setAudioVolume = function(value) {
    value = parseInt(value, 10);
    if (isNaN(value)) {
      OT.error('OT.Subscriber.setAudioVolume: value should be an integer between 0 and 100');
      return this;
    }
    _audioVolume = Math.max(0, Math.min(100, value));
    if (_audioVolume !== value) {
      OT.warn('OT.Subscriber.setAudioVolume: value should be an integer between 0 and 100');
    }
    if(_properties.muted && _audioVolume > 0) {
      _properties.premuteVolume = value;
      muteAudio.call(this, false);
    }
    if (_streamContainer) {
      _streamContainer.setAudioVolume(_audioVolume);
    }
    return this;
  };

  










  this.getAudioVolume = function() {
    if(_properties.muted) {
      return 0;
    }
    if (_streamContainer) return _streamContainer.getAudioVolume();
    else return _audioVolume;
  };

  

























  this.subscribeToAudio = function(pValue) {
    var value = OT.$.castToBoolean(pValue, true);

    if (_peerConnection) {
      _peerConnection.subscribeToAudio(value && !_properties.subscribeMute);

      if (_session && _stream && value !== _properties.subscribeToAudio) {
        _stream.setChannelActiveState('audio', value && !_properties.subscribeMute);
      }
    }

    _properties.subscribeToAudio = value;

    return this;
  };

  var muteAudio = function(_mute) {
    _chrome.muteButton.muted(_mute);

    if(_mute === _properties.mute) {
      return;
    }
    if(OT.$.env.name === 'Chrome' || OTPlugin.isInstalled()) {
      _properties.subscribeMute = _properties.muted = _mute;
      this.subscribeToAudio(_properties.subscribeToAudio);
    } else {
      if(_mute) {
        _properties.premuteVolume = this.getAudioVolume();
        _properties.muted = true;
        this.setAudioVolume(0);
      } else if(_properties.premuteVolume || _properties.audioVolume) {
        _properties.muted = false;
        this.setAudioVolume(_properties.premuteVolume || _properties.audioVolume);
      }
    }
    _properties.mute = _properties.mute;
  };

  var reasonMap = {
    auto: 'quality',
    publishVideo: 'publishVideo',
    subscribeToVideo: 'subscribeToVideo'
  };


  

























  this.subscribeToVideo = function(pValue, reason) {
    var value = OT.$.castToBoolean(pValue, true);
    
    setAudioOnly(!(value && _stream.hasVideo));

    if ( value && _container  && _container.video()) {
      _container.loading(value);
      _container.video().whenTimeIncrements(function() {
        _container.loading(false);
      }, this);
    }

    if (_chrome && _chrome.videoDisabledIndicator) {
      _chrome.videoDisabledIndicator.disableVideo(false);
    }

    if (_peerConnection) {
      _peerConnection.subscribeToVideo(value);

      if (_session && _stream && (value !== _properties.subscribeToVideo ||
          reason !== _lastSubscribeToVideoReason)) {
        _stream.setChannelActiveState('video', value, reason);
      }
    }

    _properties.subscribeToVideo = value;
    _lastSubscribeToVideoReason = reason;

    if (reason !== 'loading') {
      this.dispatchEvent(new OT.VideoEnabledChangedEvent(
        value ? 'videoEnabled' : 'videoDisabled',
        {
          reason: reasonMap[reason] || 'subscribeToVideo'
        }
      ));
    }

    return this;
  };

  this.isSubscribing = function() {
    return _state.isSubscribing();
  };

  this.isWebRTC = true;

  this.isLoading = function() {
    return _container && _container.loading();
  };

  this.videoElement = function() {
    return _streamContainer.domElement();
  };

  this.videoWidth = function() {
    return _streamContainer.videoWidth();
  };

  this.videoHeight = function() {
    return _streamContainer.videoHeight();
  };

  

































  this.restrictFrameRate = function(val) {
    OT.debug('OT.Subscriber.restrictFrameRate(' + val + ')');

    logAnalyticsEvent('restrictFrameRate', val.toString(), {streamId: _stream.id});

    if (_session.sessionInfo.p2pEnabled) {
      OT.warn('OT.Subscriber.restrictFrameRate: Cannot restrictFrameRate on a P2P session');
    }

    if (typeof val !== 'boolean') {
      OT.error('OT.Subscriber.restrictFrameRate: expected a boolean value got a ' + typeof val);
    } else {
      _frameRateRestricted = val;
      _stream.setRestrictFrameRate(val);
    }
    return this;
  };

  this.on('styleValueChanged', updateChromeForStyleChange, this);

  this._ = {
    archivingStatus: function(status) {
      if(_chrome) {
        _chrome.archive.setArchiving(status);
      }
    }
  };

  _state = new OT.SubscribingState(stateChangeFailed);

  OT.debug('OT.Subscriber: subscribe to ' + _stream.id);

  _state.set('Init');

  if (!_stream) {
    
    OT.error('OT.Subscriber: No stream parameter.');
    return false;
  }

  _stream.on({
    updated: streamUpdated,
    destroyed: streamDestroyed
  }, this);

  _fromConnectionId = _stream.connection.id;
  _properties.name = _properties.name || _stream.name;
  _properties.classNames = 'OT_root OT_subscriber';

  if (_properties.style) {
    this.setStyle(_properties.style, null, true);
  }
  if (_properties.audioVolume) {
    this.setAudioVolume(_properties.audioVolume);
  }

  _properties.subscribeToAudio = OT.$.castToBoolean(_properties.subscribeToAudio, true);
  _properties.subscribeToVideo = OT.$.castToBoolean(_properties.subscribeToVideo, true);

  _container = new OT.WidgetView(targetElement, _properties);
  this.id = _domId = _container.domId();
  this.element = _container.domElement;

  _createChrome.call(this);

  _startConnectingTime = OT.$.now();

  if (_stream.connection.id !== _session.connection.id) {
    logAnalyticsEvent('createPeerConnection', 'Attempt', '');

    _state.set('ConnectingToPeer');

    _peerConnection = new OT.SubscriberPeerConnection(_stream.connection, _session,
      _stream, this, _properties);

    _peerConnection.on({
      disconnected: onDisconnected,
      error: onPeerConnectionFailure,
      remoteStreamAdded: onRemoteStreamAdded,
      remoteStreamRemoved: onRemoteStreamRemoved,
      qos: recordQOS
    }, this);

    
    _peerConnection.init();

    if (OT.$.hasCapabilities('audioOutputLevelStat')) {
      _audioLevelSampler = new OT.GetStatsAudioLevelSampler(_peerConnection, 'out');
    } else if (OT.$.hasCapabilities('webAudioCapableRemoteStream')) {
      _audioLevelSampler = new OT.AnalyserAudioLevelSampler(OT.audioContext());
    }

    if(_audioLevelSampler) {
      var subscriber = this;
      
      
      _audioLevelRunner = new OT.IntervalRunner(function() {
        _audioLevelSampler.sample(function(audioOutputLevel) {
          if (audioOutputLevel !== null) {
            OT.$.requestAnimationFrame(function() {
              subscriber.dispatchEvent(
                new OT.AudioLevelUpdatedEvent(audioOutputLevel));
            });
          }
        });
      }, 60);
    }
  } else {
    logAnalyticsEvent('createPeerConnection', 'Attempt', '');

    var publisher = _session.getPublisherForStream(_stream);
    if(!(publisher && publisher._.webRtcStream())) {
      this.trigger('subscribeComplete', new OT.Error(null, 'InvalidStreamID'));
      return this;
    }

    
    onRemoteStreamAdded.call(this, publisher._.webRtcStream());
  }

  logConnectivityEvent('Attempt', {streamId: _stream.id});


 





























































































































































































};













































OT.Session = function(apiKey, sessionId) {
  OT.$.eventing(this);

  
  
  if (!OT.checkSystemRequirements()) {
    OT.upgradeSystemRequirements();
    return;
  }

  if(sessionId == null) {
    sessionId = apiKey;
    apiKey = null;
  }

  this.id = this.sessionId = sessionId;

  var _initialConnection = true,
      _apiKey = apiKey,
      _token,
      _session = this,
      _sessionId = sessionId,
      _socket,
      _widgetId = OT.$.uuid(),
      _connectionId = OT.$.uuid(),
      sessionConnectFailed,
      sessionDisconnectedHandler,
      connectionCreatedHandler,
      connectionDestroyedHandler,
      streamCreatedHandler,
      streamPropertyModifiedHandler,
      streamDestroyedHandler,
      archiveCreatedHandler,
      archiveDestroyedHandler,
      archiveUpdatedHandler,
      init,
      reset,
      disconnectComponents,
      destroyPublishers,
      destroySubscribers,
      connectMessenger,
      getSessionInfo,
      onSessionInfoResponse,
      permittedTo,
      _connectivityAttemptPinger,
      dispatchError;



  var setState = OT.$.statable(this, [
    'disconnected', 'connecting', 'connected', 'disconnecting'
  ], 'disconnected');

  this.connection = null;
  this.connections = new OT.$.Collection();
  this.streams = new OT.$.Collection();
  this.archives = new OT.$.Collection();








  sessionConnectFailed = function(reason, code) {
    setState('disconnected');

    OT.error(reason);

    this.trigger('sessionConnectFailed',
      new OT.Error(code || OT.ExceptionCodes.CONNECT_FAILED, reason));

    OT.handleJsException(reason, code || OT.ExceptionCodes.CONNECT_FAILED, {
      session: this
    });
  };

  sessionDisconnectedHandler = function(event) {
    var reason = event.reason;
    if(reason === 'networkTimedout') {
      reason = 'networkDisconnected';
      this.logEvent('Connect', 'TimeOutDisconnect', {reason: event.reason});
    } else {
      this.logEvent('Connect', 'Disconnected', {reason: event.reason});
    }

    var publicEvent = new OT.SessionDisconnectEvent('sessionDisconnected', reason);

    reset();
    disconnectComponents.call(this, reason);

    var defaultAction = OT.$.bind(function() {
      
      destroyPublishers.call(this, publicEvent.reason);
      
      if (!publicEvent.isDefaultPrevented()) destroySubscribers.call(this, publicEvent.reason);
    }, this);

    this.dispatchEvent(publicEvent, defaultAction);
  };

  connectionCreatedHandler = function(connection) {
    
    if (connection.id.match(/^symphony\./)) return;

    this.dispatchEvent(new OT.ConnectionEvent(
      OT.Event.names.CONNECTION_CREATED,
      connection
    ));
  };

  connectionDestroyedHandler = function(connection, reason) {
    
    if (connection.id.match(/^symphony\./)) return;

    
    
    
    if (connection.id === _socket.id()) return;

    this.dispatchEvent(
      new OT.ConnectionEvent(
        OT.Event.names.CONNECTION_DESTROYED,
        connection,
        reason
      )
    );
  };

  streamCreatedHandler = function(stream) {
    if(stream.connection.id !== this.connection.id) {
      this.dispatchEvent(new OT.StreamEvent(
        OT.Event.names.STREAM_CREATED,
        stream,
        null,
        false
      ));
    }
  };

  streamPropertyModifiedHandler = function(event) {
    var stream = event.target,
        propertyName = event.changedProperty,
        newValue = event.newValue;

    if (propertyName === 'videoDisableWarning' || propertyName === 'audioDisableWarning') {
      return; 
    }

    if (propertyName === 'orientation') {
      propertyName = 'videoDimensions';
      newValue = {width: newValue.width, height: newValue.height};
    }

    this.dispatchEvent(new OT.StreamPropertyChangedEvent(
      OT.Event.names.STREAM_PROPERTY_CHANGED,
      stream,
      propertyName,
      event.oldValue,
      newValue
    ));
  };

  streamDestroyedHandler = function(stream, reason) {

    
    
    if(stream.connection.id === this.connection.id) {
      OT.$.forEach(OT.publishers.where({ streamId: stream.id }), OT.$.bind(function(publisher) {
        publisher._.unpublishFromSession(this, reason);
      }, this));
      return;
    }

    var event = new OT.StreamEvent('streamDestroyed', stream, reason, true);

    var defaultAction = OT.$.bind(function() {
      if (!event.isDefaultPrevented()) {
        
        OT.$.forEach(OT.subscribers.where({streamId: stream.id}), function(subscriber) {
          if (subscriber.session.id === this.id) {
            if(subscriber.stream) {
              subscriber.destroy('streamDestroyed');
            }
          }
        }, this);
      } else {
        
      }
    }, this);

    this.dispatchEvent(event, defaultAction);
  };

  archiveCreatedHandler = function(archive) {
    this.dispatchEvent(new OT.ArchiveEvent('archiveStarted', archive));
  };

  archiveDestroyedHandler = function(archive) {
    this.dispatchEvent(new OT.ArchiveEvent('archiveDestroyed', archive));
  };

  archiveUpdatedHandler = function(event) {
    var archive = event.target,
      propertyName = event.changedProperty,
      newValue = event.newValue;

    if(propertyName === 'status' && newValue === 'stopped') {
      this.dispatchEvent(new OT.ArchiveEvent('archiveStopped', archive));
    } else {
      this.dispatchEvent(new OT.ArchiveEvent('archiveUpdated', archive));
    }
  };

  init = function() {
    _session.token = _token = null;
    setState('disconnected');
    _session.connection = null;
    _session.capabilities = new OT.Capabilities([]);
    _session.connections.destroy();
    _session.streams.destroy();
    _session.archives.destroy();
  };

  
  reset = function() {
    
    
    
    
    _connectionId = OT.$.uuid();
    init();
  };

  disconnectComponents = function(reason) {
    OT.$.forEach(OT.publishers.where({session: this}), function(publisher) {
      publisher.disconnect(reason);
    });

    OT.$.forEach(OT.subscribers.where({session: this}), function(subscriber) {
      subscriber.disconnect();
    });
  };

  destroyPublishers = function(reason) {
    OT.$.forEach(OT.publishers.where({session: this}), function(publisher) {
      publisher._.streamDestroyed(reason);
    });
  };

  destroySubscribers = function(reason) {
    OT.$.forEach(OT.subscribers.where({session: this}), function(subscriber) {
      subscriber.destroy(reason);
    });
  };

  connectMessenger = function() {
    OT.debug('OT.Session: connecting to Raptor');

    var socketUrl = this.sessionInfo.messagingURL,
        symphonyUrl = OT.properties.symphonyAddresss || this.sessionInfo.symphonyAddress;

    _socket = new OT.Raptor.Socket(_connectionId, _widgetId, socketUrl, symphonyUrl,
      OT.SessionDispatcher(this));


    _socket.connect(_token, this.sessionInfo, OT.$.bind(function(error, sessionState) {
      if (error) {
        _socket = void 0;
        this.logConnectivityEvent('Failure', error);

        sessionConnectFailed.call(this, error.message, error.code);
        return;
      }

      OT.debug('OT.Session: Received session state from Raptor', sessionState);

      this.connection = this.connections.get(_socket.id());
      if(this.connection) {
        this.capabilities = this.connection.permissions;
      }

      setState('connected');

      this.logConnectivityEvent('Success', null, {connectionId: this.connection.id});

      
      this.connection.on('destroyed', sessionDisconnectedHandler, this);

      
      this.connections.on({
        add: connectionCreatedHandler,
        remove: connectionDestroyedHandler
      }, this);

      
      this.streams.on({
        add: streamCreatedHandler,
        remove: streamDestroyedHandler,
        update: streamPropertyModifiedHandler
      }, this);

      this.archives.on({
        add: archiveCreatedHandler,
        remove: archiveDestroyedHandler,
        update: archiveUpdatedHandler
      }, this);

      this.dispatchEvent(
        new OT.SessionConnectEvent(OT.Event.names.SESSION_CONNECTED), OT.$.bind(function() {
          this.connections._triggerAddEvents(); 
          this.streams._triggerAddEvents(); 
          this.archives._triggerAddEvents();
        }, this)
      );

    }, this));
  };

  getSessionInfo = function() {
    if (this.is('connecting')) {
      OT.SessionInfo.get(
        this,
        OT.$.bind(onSessionInfoResponse, this),
        OT.$.bind(function(error) {
          sessionConnectFailed.call(this, error.message +
            (error.code ? ' (' + error.code + ')' : ''), error.code);
        }, this)
      );
    }
  };

  onSessionInfoResponse = function(sessionInfo) {
    if (this.is('connecting')) {
      var overrides = OT.properties.sessionInfoOverrides;
      this.sessionInfo = sessionInfo;
      if (overrides != null && typeof overrides === 'object') {
        this.sessionInfo = OT.$.defaults(overrides, this.sessionInfo);
      }
      if (this.sessionInfo.partnerId && this.sessionInfo.partnerId !== _apiKey) {
        this.apiKey = _apiKey = this.sessionInfo.partnerId;

        var reason = 'Authentication Error: The API key does not match the token or session.';

        var payload = {
          code: OT.ExceptionCodes.AUTHENTICATION_ERROR,
          message: reason
        };
        this.logEvent('Failure', 'SessionInfo', payload);

        sessionConnectFailed.call(this, reason, OT.ExceptionCodes.AUTHENTICATION_ERROR);
      } else {
        connectMessenger.call(this);
      }
    }
  };

  
  permittedTo = OT.$.bind(function(action) {
    return this.capabilities.permittedTo(action);
  }, this);

  dispatchError = OT.$.bind(function(code, message, completionHandler) {
    OT.dispatchError(code, message, completionHandler, this);
  }, this);

  this.logEvent = function(action, variation, payload, options) {
    var event = {
      action: action,
      variation: variation,
      payload: payload,
      sessionId: _sessionId,
      partnerId: _apiKey
    };

    event.connectionId = _connectionId;

    if (options) event = OT.$.extend(options, event);
    OT.analytics.logEvent(event);
  };

  







  function getTestNetworkConfig(token) {
    return new Promise(function(resolve, reject) {
      OT.$.getJSON(
        [OT.properties.apiURL, '/v2/partner/', _apiKey, '/session/', _sessionId, '/connection/',
          _connectionId, '/testNetworkConfig'].join(''),
        {
          headers: {'X-TB-TOKEN-AUTH': token}
        },
        function(errorEvent, response) {
          if (errorEvent) {
            var error = JSON.parse(errorEvent.target.responseText);
            if (error.code === -1) {
              reject(new OT.$.Error('Unexpected HTTP error codes ' +
              errorEvent.target.status, '2001'));
            } else if (error.code === 10001 || error.code === 10002) {
              reject(new OT.$.Error(error.message, '1004'));
            } else {
              reject(new OT.$.Error(error.message, error.code));
            }
          } else {
            resolve(response);
          }
        });
    });
  }

  




  this.testNetwork = function(token, publisher, callback) {

    
    var origCallback = callback;
    callback = function loggingCallback(error, stats) {
      if (error) {
        _session.logEvent('TestNetwork', 'Failure', {
          failureCode: error.name || error.message || 'unknown'
        });
      } else {
        _session.logEvent('TestNetwork', 'Success', stats);
      }
      origCallback(error, stats);
    };

    _session.logEvent('TestNetwork', 'Attempt', {});

    if(this.isConnected()) {
      callback(new OT.$.Error('Session connected, cannot test network', 1015));
      return;
    }

    var webRtcStreamPromise = new Promise(
      function(resolve, reject) {
        var webRtcStream = publisher._.webRtcStream();
        if (webRtcStream) {
          resolve(webRtcStream);
        } else {

          var onAccessAllowed = function() {
            unbind();
            resolve(publisher._.webRtcStream());
          };

          var onPublishComplete = function(error) {
            if (error) {
              unbind();
              reject(error);
            }
          };

          var unbind = function() {
            publisher.off('publishComplete', onPublishComplete);
            publisher.off('accessAllowed', onAccessAllowed);
          };

          publisher.on('publishComplete', onPublishComplete);
          publisher.on('accessAllowed', onAccessAllowed);

        }
      });

    var testConfig;
    var webrtcStats;
    Promise.all([getTestNetworkConfig(token), webRtcStreamPromise])
      .then(function(values) {
        var webRtcStream = values[1];
        testConfig = values[0];
        return OT.webrtcTest({mediaConfig: testConfig.media, localStream: webRtcStream});
      })
      .then(function(stats) {
        OT.debug('Received stats from webrtcTest: ', stats);
        if (stats.bandwidth < testConfig.media.thresholdBitsPerSecond) {
          return Promise.reject(new OT.$.Error('The detect bandwidth form the WebRTC stage of ' +
          'the test was not sufficient to run the HTTP stage of the test', 1553));
        }

        webrtcStats = stats;
      })
      .then(function() {
        return OT.httpTest({httpConfig: testConfig.http});
      })
      .then(function(httpStats) {
        var stats = {
          uploadBitsPerSecond: httpStats.uploadBandwidth,
          downloadBitsPerSecond: httpStats.downloadBandwidth,
          packetLossRatio: webrtcStats.packetLostRatio,
          roundTripTimeMilliseconds: webrtcStats.roundTripTime
        };
        callback(null, stats);
        
      })['catch'](function(error) {
        callback(error);
      });
  };

  this.logConnectivityEvent = function(variation, payload, options) {
    if (variation === 'Attempt' || !_connectivityAttemptPinger) {
      var pingerOptions = {
        action: 'Connect',
        sessionId: _sessionId,
        partnerId: _apiKey
      };
      if (this.connection && this.connection.id) {
        pingerOptions = event.connectionId = this.connection.id;
      } else if (_connectionId) {
        pingerOptions.connectionId = _connectionId;
      }
      _connectivityAttemptPinger = new OT.ConnectivityAttemptPinger(pingerOptions);
    }
    _connectivityAttemptPinger.setVariation(variation);
    this.logEvent('Connect', variation, payload, options);
  };




























































































  this.connect = function(token) {

    if(apiKey == null && arguments.length > 1 &&
      (typeof arguments[0] === 'string' || typeof arguments[0] === 'number') &&
      typeof arguments[1] === 'string') {
      _apiKey = token.toString();
      token = arguments[1];
    }

    
    var completionHandler = arguments[arguments.length - 1];

    if (this.is('connecting', 'connected')) {
      OT.warn('OT.Session: Cannot connect, the session is already ' + this.state);
      return this;
    }

    init();
    setState('connecting');
    this.token = _token = !OT.$.isFunction(token) && token;

    
    if (_initialConnection) {
      _initialConnection = false;
    } else {
      _widgetId = OT.$.uuid();
    }

    if (completionHandler && OT.$.isFunction(completionHandler)) {
      this.once('sessionConnected', OT.$.bind(completionHandler, null, null));
      this.once('sessionConnectFailed', completionHandler);
    }

    if(_apiKey == null || OT.$.isFunction(_apiKey)) {
      setTimeout(OT.$.bind(
        sessionConnectFailed,
        this,
        'API Key is undefined. You must pass an API Key to initSession.',
        OT.ExceptionCodes.AUTHENTICATION_ERROR
      ));

      return this;
    }

    if (!_sessionId) {
      setTimeout(OT.$.bind(
        sessionConnectFailed,
        this,
        'SessionID is undefined. You must pass a sessionID to initSession.',
        OT.ExceptionCodes.INVALID_SESSION_ID
      ));

      return this;
    }

    this.apiKey = _apiKey = _apiKey.toString();

    
    if (OT.APIKEY.length === 0) {
      OT.APIKEY = _apiKey;
    }

    this.logConnectivityEvent('Attempt');

    getSessionInfo.call(this);
    return this;
  };



























































  var disconnect = OT.$.bind(function disconnect(drainSocketBuffer) {
    if (_socket && _socket.isNot('disconnected')) {
      if (_socket.isNot('disconnecting')) {
        setState('disconnecting');
        _socket.disconnect(drainSocketBuffer);
      }
    }
    else {
      reset();
    }
  }, this);

  this.disconnect = function(drainSocketBuffer) {
    disconnect(drainSocketBuffer !== void 0 ? drainSocketBuffer : true);
  };

  this.destroy = function(reason) {
    this.streams.destroy();
    this.connections.destroy();
    this.archives.destroy();
    disconnect(reason !== 'unloaded');
  };






















































































































  this.publish = function(publisher, properties, completionHandler) {
    if(typeof publisher === 'function') {
      completionHandler = publisher;
      publisher = undefined;
    }
    if(typeof properties === 'function') {
      completionHandler = properties;
      properties = undefined;
    }
    if (this.isNot('connected')) {
      OT.analytics.logError(1010, 'OT.exception',
        'We need to be connected before you can publish', null, {
        action: 'Publish',
        variation: 'Failure',
        payload: {
          reason:'unconnected',
          code: OT.ExceptionCodes.NOT_CONNECTED,
          message: 'We need to be connected before you can publish'
        },
        sessionId: _sessionId,
        streamId: (publisher && publisher.stream) ? publisher.stream.id : null,
        partnerId: _apiKey,
      });

      if (completionHandler && OT.$.isFunction(completionHandler)) {
        dispatchError(OT.ExceptionCodes.NOT_CONNECTED,
          'We need to be connected before you can publish', completionHandler);
      }

      return null;
    }

    if (!permittedTo('publish')) {
      var errorMessage = 'This token does not allow publishing. The role must be at least ' +
        '`publisher` to enable this functionality';
      var payload = {
        reason: 'permission',
        code: OT.ExceptionCodes.UNABLE_TO_PUBLISH,
        message: errorMessage
      };
      this.logEvent('publish', 'Failure', payload);
      dispatchError(OT.ExceptionCodes.UNABLE_TO_PUBLISH, errorMessage, completionHandler);
      return null;
    }

    
    if (!publisher || typeof(publisher)==='string' || OT.$.isElementNode(publisher)) {
      
      publisher = OT.initPublisher(publisher, properties);

    } else if (publisher instanceof OT.Publisher){

      
      if ('session' in publisher && publisher.session && 'sessionId' in publisher.session) {
        
        if( publisher.session.sessionId === this.sessionId){
          OT.warn('Cannot publish ' + publisher.guid() + ' again to ' +
            this.sessionId + '. Please call session.unpublish(publisher) first.');
        } else {
          OT.warn('Cannot publish ' + publisher.guid() + ' publisher already attached to ' +
            publisher.session.sessionId+ '. Please call session.unpublish(publisher) first.');
        }
      }

    } else {
      dispatchError(OT.ExceptionCodes.UNABLE_TO_PUBLISH,
        'Session.publish :: First parameter passed in is neither a ' +
        'string nor an instance of the Publisher',
        completionHandler);
      return;
    }

    publisher.once('publishComplete', function(err) {
      if (err) {
        dispatchError(OT.ExceptionCodes.UNABLE_TO_PUBLISH,
          'Session.publish :: ' + err.message,
          completionHandler);
        return;
      }

      if (completionHandler && OT.$.isFunction(completionHandler)) {
        completionHandler.apply(null, arguments);
      }
    });

    
    publisher._.publishToSession(this);

    
    return publisher;
  };


















































































  this.unpublish = function(publisher) {
    if (!publisher) {
      OT.error('OT.Session.unpublish: publisher parameter missing.');
      return;
    }

    
    publisher._.unpublishFromSession(this, 'unpublished');
  };





























































































































































































  this.subscribe = function(stream, targetElement, properties, completionHandler) {

    if (!this.connection || !this.connection.connectionId) {
      dispatchError(OT.ExceptionCodes.UNABLE_TO_SUBSCRIBE,
                    'Session.subscribe :: Connection required to subscribe',
                    completionHandler);
      return;
    }

    if (!stream) {
      dispatchError(OT.ExceptionCodes.UNABLE_TO_SUBSCRIBE,
                    'Session.subscribe :: stream cannot be null',
                    completionHandler);
      return;
    }

    if (!stream.hasOwnProperty('streamId')) {
      dispatchError(OT.ExceptionCodes.UNABLE_TO_SUBSCRIBE,
                    'Session.subscribe :: invalid stream object',
                    completionHandler);
      return;
    }

    if(typeof targetElement === 'function') {
      completionHandler = targetElement;
      targetElement = undefined;
      properties = undefined;
    }

    if(typeof properties === 'function') {
      completionHandler = properties;
      properties = undefined;
    }

    var subscriber = new OT.Subscriber(targetElement, OT.$.extend(properties || {}, {
      stream: stream,
      session: this
    }), function(err) {

      if (err) {
        dispatchError(OT.ExceptionCodes.UNABLE_TO_SUBSCRIBE,
              'Session.subscribe :: ' + err.message,
              completionHandler);

      } else  if (completionHandler && OT.$.isFunction(completionHandler)) {
        completionHandler.apply(null, arguments);
      }

    });

    OT.subscribers.add(subscriber);

    return subscriber;

  };



























































  this.unsubscribe = function(subscriber) {
    if (!subscriber) {
      var errorMsg = 'OT.Session.unsubscribe: subscriber cannot be null';
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    if (!subscriber.stream) {
      OT.warn('OT.Session.unsubscribe:: tried to unsubscribe a subscriber that had no stream');
      return false;
    }

    OT.debug('OT.Session.unsubscribe: subscriber ' + subscriber.id);

    subscriber.destroy();

    return true;
  };














  this.getSubscribersForStream = function(stream) {
    return OT.subscribers.where({streamId: stream.id});
  };

















  this.getPublisherForStream = function(stream) {
    var streamId,
        errorMsg;

    if (typeof stream === 'string') {
      streamId = stream;
    } else if (typeof stream === 'object' && stream && stream.hasOwnProperty('id')) {
      streamId = stream.id;
    } else {
      errorMsg = 'Session.getPublisherForStream :: Invalid stream type';
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    return OT.publishers.where({streamId: streamId})[0];
  };

  
  this._ = {
    jsepCandidateP2p: function(streamId, subscriberId, candidate) {
      return _socket.jsepCandidateP2p(streamId, subscriberId, candidate);
    },

    jsepCandidate: function(streamId, candidate) {
      return _socket.jsepCandidate(streamId, candidate);
    },

    jsepOffer: function(streamId, offerSdp) {
      return _socket.jsepOffer(streamId, offerSdp);
    },

    jsepOfferP2p: function(streamId, subscriberId, offerSdp) {
      return _socket.jsepOfferP2p(streamId, subscriberId, offerSdp);
    },

    jsepAnswer: function(streamId, answerSdp) {
      return _socket.jsepAnswer(streamId, answerSdp);
    },

    jsepAnswerP2p: function(streamId, subscriberId, answerSdp) {
      return _socket.jsepAnswerP2p(streamId, subscriberId, answerSdp);
    },

    
    
    dispatchSignal: OT.$.bind(function(fromConnection, type, data) {
      var event = new OT.SignalEvent(type, data, fromConnection);
      event.target = this;

      
      
      this.trigger(OT.Event.names.SIGNAL, event);

      
      if (type) this.dispatchEvent(event);
    }, this),

    subscriberCreate: function(stream, subscriber, channelsToSubscribeTo, completion) {
      return _socket.subscriberCreate(stream.id, subscriber.widgetId,
        channelsToSubscribeTo, completion);
    },

    subscriberDestroy: function(stream, subscriber) {
      return _socket.subscriberDestroy(stream.id, subscriber.widgetId);
    },

    subscriberUpdate: function(stream, subscriber, attributes) {
      return _socket.subscriberUpdate(stream.id, subscriber.widgetId, attributes);
    },

    subscriberChannelUpdate: function(stream, subscriber, channel, attributes) {
      return _socket.subscriberChannelUpdate(stream.id, subscriber.widgetId, channel.id,
        attributes);
    },

    streamCreate: function(name, streamId, audioFallbackEnabled, channels, completion) {
      _socket.streamCreate(
        name,
        streamId,
        audioFallbackEnabled,
        channels,
        OT.Config.get('bitrates', 'min', OT.APIKEY),
        OT.Config.get('bitrates', 'max', OT.APIKEY),
        completion
      );
    },

    streamDestroy: function(streamId) {
      _socket.streamDestroy(streamId);
    },

    streamChannelUpdate: function(stream, channel, attributes) {
      _socket.streamChannelUpdate(stream.id, channel.id, attributes);
    }
  };
















































































































  this.signal = function(options, completion) {
    var _options = options,
        _completion = completion;

    if (OT.$.isFunction(_options)) {
      _completion = _options;
      _options = null;
    }

    if (this.isNot('connected')) {
      var notConnectedErrorMsg = 'Unable to send signal - you are not connected to the session.';
      dispatchError(500, notConnectedErrorMsg, _completion);
      return;
    }

    _socket.signal(_options, _completion, this.logEvent);
    if (options && options.data && (typeof(options.data) !== 'string')) {
      OT.warn('Signaling of anything other than Strings is deprecated. ' +
              'Please update the data property to be a string.');
    }
  };






















































































  this.forceDisconnect = function(connectionOrConnectionId, completionHandler) {
    if (this.isNot('connected')) {
      var notConnectedErrorMsg = 'Cannot call forceDisconnect(). You are not ' +
                                 'connected to the session.';
      dispatchError(OT.ExceptionCodes.NOT_CONNECTED, notConnectedErrorMsg, completionHandler);
      return;
    }

    var notPermittedErrorMsg = 'This token does not allow forceDisconnect. ' +
      'The role must be at least `moderator` to enable this functionality';

    if (permittedTo('forceDisconnect')) {
      var connectionId = typeof connectionOrConnectionId === 'string' ?
        connectionOrConnectionId : connectionOrConnectionId.id;

      _socket.forceDisconnect(connectionId, function(err) {
        if (err) {
          dispatchError(OT.ExceptionCodes.UNABLE_TO_FORCE_DISCONNECT,
            notPermittedErrorMsg, completionHandler);

        } else if (completionHandler && OT.$.isFunction(completionHandler)) {
          completionHandler.apply(null, arguments);
        }
      });
    } else {
      
      dispatchError(OT.ExceptionCodes.UNABLE_TO_FORCE_DISCONNECT,
        notPermittedErrorMsg, completionHandler);
    }
  };




























































  this.forceUnpublish = function(streamOrStreamId, completionHandler) {
    if (this.isNot('connected')) {
      var notConnectedErrorMsg = 'Cannot call forceUnpublish(). You are not ' +
                                 'connected to the session.';
      dispatchError(OT.ExceptionCodes.NOT_CONNECTED, notConnectedErrorMsg, completionHandler);
      return;
    }

    var notPermittedErrorMsg = 'This token does not allow forceUnpublish. ' +
      'The role must be at least `moderator` to enable this functionality';

    if (permittedTo('forceUnpublish')) {
      var stream = typeof streamOrStreamId === 'string' ?
        this.streams.get(streamOrStreamId) : streamOrStreamId;

      _socket.forceUnpublish(stream.id, function(err) {
        if (err) {
          dispatchError(OT.ExceptionCodes.UNABLE_TO_FORCE_UNPUBLISH,
            notPermittedErrorMsg, completionHandler);
        } else if (completionHandler && OT.$.isFunction(completionHandler)) {
          completionHandler.apply(null, arguments);
        }
      });
    } else {
      
      dispatchError(OT.ExceptionCodes.UNABLE_TO_FORCE_UNPUBLISH,
        notPermittedErrorMsg, completionHandler);
    }
  };

  this.getStateManager = function() {
    OT.warn('Fixme: Have not implemented session.getStateManager');
  };

  this.isConnected = function() {
    return this.is('connected');
  };

  this.capabilities = new OT.Capabilities([]);
















































































































































































































};



























var defaultConstraints = {
  audio: true,
  video: true
};















































OT.Publisher = function(options) {
  
  
  if (!OT.checkSystemRequirements()) {
    OT.upgradeSystemRequirements();
    return;
  }

  var _guid = OT.Publisher.nextId(),
      _domId,
      _widgetView,
      _targetElement,
      _stream,
      _streamId,
      _webRTCStream,
      _session,
      _peerConnections = {},
      _loaded = false,
      _publishStartTime,
      _microphone,
      _chrome,
      _audioLevelMeter,
      _properties,
      _validResolutions,
      _validFrameRates = [ 1, 7, 15, 30 ],
      _prevStats,
      _state,
      _iceServers,
      _audioLevelCapable = OT.$.hasCapabilities('webAudio'),
      _audioLevelSampler,
      _isScreenSharing = options && (
        options.videoSource === 'screen' ||
        options.videoSource === 'window' ||
        options.videoSource === 'tab' ||
        options.videoSource === 'browser' ||
        options.videoSource === 'application'
      ),
      _connectivityAttemptPinger,
      _publisher = this;

  _properties = OT.$.defaults(options || {}, {
    publishAudio: _isScreenSharing ? false : true,
    publishVideo: true,
    mirror: _isScreenSharing ? false : true,
    showControls: true,
    fitMode: _isScreenSharing ? 'contain' : 'cover',
    audioFallbackEnabled: _isScreenSharing ? false : true,
    maxResolution: _isScreenSharing ? { width: 1920, height: 1920 } : undefined
  });

  _validResolutions = {
    '320x240': {width: 320, height: 240},
    '640x480': {width: 640, height: 480},
    '1280x720': {width: 1280, height: 720}
  };
  
  if (_isScreenSharing) {
    if (window.location.protocol !== 'https:') {
      OT.warn('Screen Sharing typically requires pages to be loadever over HTTPS - unless this ' +
        'browser is configured locally to allow non-SSL pages, permission will be denied ' +
        'without user input.');
    }
  }

  _prevStats = {
    'timeStamp' : OT.$.now()
  };

  OT.$.eventing(this);

  if(!_isScreenSharing && _audioLevelCapable) {
    _audioLevelSampler = new OT.AnalyserAudioLevelSampler(OT.audioContext());

    var audioLevelRunner = new OT.IntervalRunner(function() {
      _audioLevelSampler.sample(function(audioInputLevel) {
        OT.$.requestAnimationFrame(function() {
          _publisher.dispatchEvent(
            new OT.AudioLevelUpdatedEvent(audioInputLevel));
        });
      });
    }, 60);

    this.on({
      'audioLevelUpdated:added': function(count) {
        if (count === 1) {
          audioLevelRunner.start();
        }
      },
      'audioLevelUpdated:removed': function(count) {
        if (count === 0) {
          audioLevelRunner.stop();
        }
      }
    });
  }

  
  var logAnalyticsEvent = function(action, variation, payload, throttle) {
        OT.analytics.logEvent({
          action: action,
          variation: variation,
          payload: payload,
          'sessionId': _session ? _session.sessionId : null,
          'connectionId': _session &&
            _session.isConnected() ? _session.connection.connectionId : null,
          'partnerId': _session ? _session.apiKey : OT.APIKEY,
          streamId: _streamId
        }, throttle);
      },

      logConnectivityEvent = function(variation, payload) {
        if (variation === 'Attempt' || !_connectivityAttemptPinger) {
          _connectivityAttemptPinger = new OT.ConnectivityAttemptPinger({
            action: 'Publish',
            'sessionId': _session ? _session.sessionId : null,
            'connectionId': _session &&
              _session.isConnected() ? _session.connection.connectionId : null,
            'partnerId': _session ? _session.apiKey : OT.APIKEY,
            streamId: _streamId
          });
        }
        if (variation === 'Failure' && payload.reason !== 'Non-fatal') {
          
          
          _connectivityAttemptPinger.setVariation(variation);
        }
        logAnalyticsEvent('Publish', variation, payload);
      },

      recordQOS = OT.$.bind(function(connection, parsedStats) {
        var QoSBlob = {
          streamType: 'WebRTC',
          sessionId: _session ? _session.sessionId : null,
          connectionId: _session && _session.isConnected() ?
            _session.connection.connectionId : null,
          partnerId: _session ? _session.apiKey : OT.APIKEY,
          streamId: _streamId,
          width: _widgetView ? Number(OT.$.width(_widgetView.domElement).replace('px', ''))
            : undefined,
          height: _widgetView ? Number(OT.$.height(_widgetView.domElement).replace('px', ''))
            : undefined,
          version: OT.properties.version,
          mediaServerName: _session ? _session.sessionInfo.messagingServer : null,
          p2pFlag: _session ? _session.sessionInfo.p2pEnabled : false,
          duration: _publishStartTime ? new Date().getTime() - _publishStartTime.getTime() : 0,
          remoteConnectionId: connection.id
        };
        OT.analytics.logQOS( OT.$.extend(QoSBlob, parsedStats) );
        this.trigger('qos', parsedStats);
      }, this),

      

      stateChangeFailed = function(changeFailed) {
        OT.error('Publisher State Change Failed: ', changeFailed.message);
        OT.debug(changeFailed);
      },

      onLoaded = OT.$.bind(function() {
        if (_state.isDestroyed()) {
          
          return;
        }

        OT.debug('OT.Publisher.onLoaded');

        _state.set('MediaBound');

        
        
        _widgetView.loading(this.session ? !_stream : false);

        _loaded = true;

        createChrome.call(this);

        this.trigger('initSuccess');
        this.trigger('loaded', this);
      }, this),

      onLoadFailure = OT.$.bind(function(reason) {
        var errorCode = OT.ExceptionCodes.P2P_CONNECTION_FAILED;
        var payload = {
          reason: 'Publisher PeerConnection Error: ',
          code: errorCode,
          message: reason
        };
        logConnectivityEvent('Failure', payload);

        _state.set('Failed');
        this.trigger('publishComplete', new OT.Error(errorCode,
          'Publisher PeerConnection Error: ' + reason));

        OT.handleJsException('Publisher PeerConnection Error: ' + reason,
          OT.ExceptionCodes.P2P_CONNECTION_FAILED, {
          session: _session,
          target: this
        });
      }, this),

      onStreamAvailable = OT.$.bind(function(webOTStream) {
        OT.debug('OT.Publisher.onStreamAvailable');

        _state.set('BindingMedia');

        cleanupLocalStream();
        _webRTCStream = webOTStream;

        _microphone = new OT.Microphone(_webRTCStream, !_properties.publishAudio);
        this.publishVideo(_properties.publishVideo &&
          _webRTCStream.getVideoTracks().length > 0);


        this.accessAllowed = true;
        this.dispatchEvent(new OT.Event(OT.Event.names.ACCESS_ALLOWED, false));

        var videoContainerOptions = {
          muted: true,
          error: onVideoError
        };

        _targetElement = _widgetView.bindVideo(_webRTCStream,
                                          videoContainerOptions,
                                          function(err) {
          if (err) {
            onLoadFailure(err);
            return;
          }

          onLoaded();
        });

        if(_audioLevelSampler && _webRTCStream.getAudioTracks().length > 0) {
          _audioLevelSampler.webRTCStream(_webRTCStream);
        }

      }, this),

      onStreamAvailableError = OT.$.bind(function(error) {
        OT.error('OT.Publisher.onStreamAvailableError ' + error.name + ': ' + error.message);

        _state.set('Failed');
        this.trigger('publishComplete', new OT.Error(OT.ExceptionCodes.UNABLE_TO_PUBLISH,
            error.message));

        if (_widgetView) _widgetView.destroy();

        var payload = {
          reason: 'GetUserMedia',
          code: OT.ExceptionCodes.UNABLE_TO_PUBLISH,
          message: 'Publisher failed to access camera/mic: ' + error.message
        };
        logConnectivityEvent('Failure', payload);

        OT.handleJsException(payload.reason,
          payload.code, {
          session: _session,
          target: this
        });
      }, this),

      onScreenSharingError = OT.$.bind(function(error) {
        OT.error('OT.Publisher.onScreenSharingError ' + error.message);
        _state.set('Failed');

        this.trigger('publishComplete', new OT.Error(OT.ExceptionCodes.UNABLE_TO_PUBLISH,
          'Screensharing: ' + error.message));

        var payload = {
          reason: 'ScreenSharing',
          message:error.message
        };
        logConnectivityEvent('Failure', payload);
      }, this),

      
      
      onAccessDenied = OT.$.bind(function(error) {
        OT.error('OT.Publisher.onStreamAvailableError Permission Denied');

        _state.set('Failed');
        var errorMessage = 'Publisher Access Denied: Permission Denied' +
            (error.message ? ': ' + error.message : '');
        var errorCode = OT.ExceptionCodes.UNABLE_TO_PUBLISH;
        this.trigger('publishComplete', new OT.Error(errorCode, errorMessage));

        var payload = {
          reason: 'GetUserMedia',
          code: errorCode,
          message: errorMessage
        };
        logConnectivityEvent('Failure', payload);

        this.dispatchEvent(new OT.Event(OT.Event.names.ACCESS_DENIED));
      }, this),

      onAccessDialogOpened = OT.$.bind(function() {
        logAnalyticsEvent('accessDialog', 'Opened');

        this.dispatchEvent(new OT.Event(OT.Event.names.ACCESS_DIALOG_OPENED, true));
      }, this),

      onAccessDialogClosed = OT.$.bind(function() {
        logAnalyticsEvent('accessDialog', 'Closed');

        this.dispatchEvent( new OT.Event(OT.Event.names.ACCESS_DIALOG_CLOSED, false));
      }, this),

      onVideoError = OT.$.bind(function(errorCode, errorReason) {
        OT.error('OT.Publisher.onVideoError');

        var message = errorReason + (errorCode ? ' (' + errorCode + ')' : '');
        logAnalyticsEvent('stream', null, {reason:'Publisher while playing stream: ' + message});

        _state.set('Failed');

        if (_state.isAttemptingToPublish()) {
          this.trigger('publishComplete', new OT.Error(OT.ExceptionCodes.UNABLE_TO_PUBLISH,
              message));
        } else {
          this.trigger('error', message);
        }

        OT.handleJsException('Publisher error playing stream: ' + message,
        OT.ExceptionCodes.UNABLE_TO_PUBLISH, {
          session: _session,
          target: this
        });
      }, this),

      onPeerDisconnected = OT.$.bind(function(peerConnection) {
        OT.debug('OT.Subscriber has been disconnected from the Publisher\'s PeerConnection');

        this.cleanupSubscriber(peerConnection.remoteConnection().id);
      }, this),

      onPeerConnectionFailure = OT.$.bind(function(code, reason, peerConnection, prefix) {
        var payload = {
          reason: prefix ? prefix : 'PeerConnectionError',
          code: OT.ExceptionCodes.UNABLE_TO_PUBLISH,
          message: (prefix ? prefix : '') + ':Publisher PeerConnection with connection ' +
            (peerConnection && peerConnection.remoteConnection &&
            peerConnection.remoteConnection().id) + ' failed: ' + reason,
          hasRelayCandidates: peerConnection.hasRelayCandidates()
        };
        if (_state.isPublishing()) {
          
          
          payload.reason = 'Non-fatal';
        }
        logConnectivityEvent('Failure', payload);

        OT.handleJsException('Publisher PeerConnection Error: ' + reason,
        OT.ExceptionCodes.UNABLE_TO_PUBLISH, {
          session: _session,
          target: this
        });

        
        
        
        

        delete _peerConnections[peerConnection.remoteConnection().id];
      }, this),

      

      
      
      
      assignStream = OT.$.bind(function(stream) {
        this.stream = _stream = stream;
        _stream.on('destroyed', this.disconnect, this);

        _state.set('Publishing');
        _widgetView.loading(!_loaded);
        _publishStartTime = new Date();

        this.trigger('publishComplete', null, this);

        this.dispatchEvent(new OT.StreamEvent('streamCreated', stream, null, false));

        var payload = {
          streamType: 'WebRTC',
        };
        logConnectivityEvent('Success', payload);
      }, this),

      
      cleanupLocalStream = function() {
        if (_webRTCStream) {
          
          
          _webRTCStream.stop();
          _webRTCStream = null;
        }
      },

      createPeerConnectionForRemote = OT.$.bind(function(remoteConnection) {
        var peerConnection = _peerConnections[remoteConnection.id];

        if (!peerConnection) {
          var startConnectingTime = OT.$.now();

          logAnalyticsEvent('createPeerConnection', 'Attempt');

          
          remoteConnection.on('destroyed',
            OT.$.bind(this.cleanupSubscriber, this, remoteConnection.id));

          peerConnection = _peerConnections[remoteConnection.id] = new OT.PublisherPeerConnection(
            remoteConnection,
            _session,
            _streamId,
            _webRTCStream
          );

          peerConnection.on({
            connected: function() {
              var payload = {
                pcc: parseInt(OT.$.now() - startConnectingTime, 10),
                hasRelayCandidates: peerConnection.hasRelayCandidates()
              };
              logAnalyticsEvent('createPeerConnection', 'Success', payload);
            },
            disconnected: onPeerDisconnected,
            error: onPeerConnectionFailure,
            qos: recordQOS
          }, this);

          peerConnection.init(_iceServers);
        }

        return peerConnection;
      }, this),

      

      
      
      
      chromeButtonMode = function(mode) {
        if (mode === false) return 'off';

        var defaultMode = this.getStyle('buttonDisplayMode');

        
        if (defaultMode === false) return 'on';

        
        return defaultMode;
      },

      updateChromeForStyleChange = function(key, value) {
        if (!_chrome) return;

        switch(key) {
          case 'nameDisplayMode':
            _chrome.name.setDisplayMode(value);
            _chrome.backingBar.setNameMode(value);
            break;

          case 'showArchiveStatus':
            logAnalyticsEvent('showArchiveStatus', 'styleChange', {mode: value ? 'on': 'off'});
            _chrome.archive.setShowArchiveStatus(value);
            break;

          case 'buttonDisplayMode':
            _chrome.muteButton.setDisplayMode(value);
            _chrome.backingBar.setMuteMode(value);
            break;

          case 'audioLevelDisplayMode':
            _chrome.audioLevel.setDisplayMode(value);
            break;

          case 'backgroundImageURI':
            _widgetView.setBackgroundImageURI(value);
        }
      },

      createChrome = function() {

        if(!this.getStyle('showArchiveStatus')) {
          logAnalyticsEvent('showArchiveStatus', 'createChrome', {mode: 'off'});
        }

        var widgets = {
          backingBar: new OT.Chrome.BackingBar({
            nameMode: !_properties.name ? 'off' : this.getStyle('nameDisplayMode'),
            muteMode: chromeButtonMode.call(this, this.getStyle('buttonDisplayMode'))
          }),

          name: new OT.Chrome.NamePanel({
            name: _properties.name,
            mode: this.getStyle('nameDisplayMode')
          }),

          muteButton: new OT.Chrome.MuteButton({
            muted: _properties.publishAudio === false,
            mode: chromeButtonMode.call(this, this.getStyle('buttonDisplayMode'))
          }),

          archive: new OT.Chrome.Archiving({
            show: this.getStyle('showArchiveStatus'),
            archiving: false
          })
        };

        if (_audioLevelCapable) {
          var audioLevelTransformer = new OT.AudioLevelTransformer();

          var audioLevelUpdatedHandler = function(evt) {
            _audioLevelMeter.setValue(audioLevelTransformer.transform(evt.audioLevel));
          };

          _audioLevelMeter = new OT.Chrome.AudioLevelMeter({
            mode: this.getStyle('audioLevelDisplayMode'),
            onActivate: function() {
              _publisher.on('audioLevelUpdated', audioLevelUpdatedHandler);
            },
            onPassivate: function() {
              _publisher.off('audioLevelUpdated', audioLevelUpdatedHandler);
            }
          });

          widgets.audioLevel = _audioLevelMeter;
        }

        _chrome = new OT.Chrome({
          parent: _widgetView.domElement
        }).set(widgets).on({
          muted: OT.$.bind(this.publishAudio, this, false),
          unmuted: OT.$.bind(this.publishAudio, this, true)
        });

        if(_audioLevelMeter && this.getStyle('audioLevelDisplayMode') === 'auto') {
          _audioLevelMeter[_widgetView.audioOnly() ? 'show' : 'hide']();
        }
      },

      reset = OT.$.bind(function() {
        if (_chrome) {
          _chrome.destroy();
          _chrome = null;
        }

        this.disconnect();

        _microphone = null;

        if (_targetElement) {
          _targetElement.destroy();
          _targetElement = null;
        }

        cleanupLocalStream();

        if (_widgetView) {
          _widgetView.destroy();
          _widgetView = null;
        }

        if (_session) {
          this._.unpublishFromSession(_session, 'reset');
        }

        this.id = _domId = null;
        this.stream = _stream = null;
        _loaded = false;

        this.session = _session = null;

        if (!_state.isDestroyed()) _state.set('NotPublishing');
      }, this);
      
  OT.StylableComponent(this, {
    showArchiveStatus: true,
    nameDisplayMode: 'auto',
    buttonDisplayMode: 'auto',
    audioLevelDisplayMode: _isScreenSharing ? 'off' : 'auto',
    backgroundImageURI: null
  }, _properties.showControls, function (payload) {
    logAnalyticsEvent('SetStyle', 'Publisher', payload, 0.1);
  });

  var setAudioOnly = function(audioOnly) {
    if (_widgetView) {
      _widgetView.audioOnly(audioOnly);
      _widgetView.showPoster(audioOnly);
    }
  
    if (_audioLevelMeter && _publisher.getStyle('audioLevelDisplayMode') === 'auto') {
      _audioLevelMeter[audioOnly ? 'show' : 'hide']();
    }
  };

  this.publish = function(targetElement) {
    OT.debug('OT.Publisher: publish');

    if ( _state.isAttemptingToPublish() || _state.isPublishing() ) reset();
    _state.set('GetUserMedia');

    if (!_properties.constraints) {
      _properties.constraints = OT.$.clone(defaultConstraints);

      if (_isScreenSharing) {
        if (_properties.audioSource != null) {
          OT.warn('Invalid audioSource passed to Publisher - when using screen sharing no ' +
            'audioSource may be used');
        }
        _properties.audioSource = null;
      }

      if(_properties.audioSource === null || _properties.audioSource === false) {
        _properties.constraints.audio = false;
        _properties.publishAudio = false;
      } else {
        if(typeof _properties.audioSource === 'object') {
          if(_properties.audioSource.deviceId != null) {
            _properties.audioSource = _properties.audioSource.deviceId;
          } else {
            OT.warn('Invalid audioSource passed to Publisher. Expected either a device ID');
          }
        }

        if (_properties.audioSource) {
          if (typeof _properties.constraints.audio !== 'object') {
            _properties.constraints.audio = {};
          }
          if (!_properties.constraints.audio.mandatory) {
            _properties.constraints.audio.mandatory = {};
          }
          if (!_properties.constraints.audio.optional) {
            _properties.constraints.audio.optional = [];
          }
          _properties.constraints.audio.mandatory.sourceId =
            _properties.audioSource;
        }
      }

      if(_properties.videoSource === null || _properties.videoSource === false) {
        _properties.constraints.video = false;
        _properties.publishVideo = false;
      } else {

        if(typeof _properties.videoSource === 'object' &&
          _properties.videoSource.deviceId == null) {
          OT.warn('Invalid videoSource passed to Publisher. Expected either a device ' +
            'ID or device.');
          _properties.videoSource = null;
        }

        var _setupVideoDefaults = function() {
          if (typeof _properties.constraints.video !== 'object') {
            _properties.constraints.video = {};
          }
          if (!_properties.constraints.video.mandatory) {
            _properties.constraints.video.mandatory = {};
          }
          if (!_properties.constraints.video.optional) {
            _properties.constraints.video.optional = [];
          }
        };

        if (_properties.videoSource) {
          _setupVideoDefaults();

          var mandatory = _properties.constraints.video.mandatory;
          
          if(_isScreenSharing) {
            
          } else if(_properties.videoSource.deviceId != null) {
            mandatory.sourceId = _properties.videoSource.deviceId;
          } else {
            mandatory.sourceId = _properties.videoSource;
          }
        }

        if (_properties.resolution) {
          if (!_validResolutions.hasOwnProperty(_properties.resolution)) {
            OT.warn('Invalid resolution passed to the Publisher. Got: ' +
              _properties.resolution + ' expecting one of "' +
              OT.$.keys(_validResolutions).join('","') + '"');
          } else {
            _properties.videoDimensions = _validResolutions[_properties.resolution];
            _setupVideoDefaults();
            if (OT.$.env.name === 'Chrome') {
              _properties.constraints.video.optional =
                _properties.constraints.video.optional.concat([
                  {minWidth: _properties.videoDimensions.width},
                  {maxWidth: _properties.videoDimensions.width},
                  {minHeight: _properties.videoDimensions.height},
                  {maxHeight: _properties.videoDimensions.height}
                ]);
            } else {
              
            }
          }
        }

        if (_properties.maxResolution) {
          _setupVideoDefaults();
          if (_properties.maxResolution.width > 1920) {
            OT.warn('Invalid maxResolution passed to the Publisher. maxResolution.width must ' +
              'be less than or equal to 1920');
            _properties.maxResolution.width = 1920;
          }
          if (_properties.maxResolution.height > 1920) {
            OT.warn('Invalid maxResolution passed to the Publisher. maxResolution.height must ' +
              'be less than or equal to 1920');
            _properties.maxResolution.height = 1920;
          }

          _properties.videoDimensions = _properties.maxResolution;

          if (OT.$.env.name === 'Chrome') {
            _setupVideoDefaults();
            _properties.constraints.video.mandatory.maxWidth =
              _properties.videoDimensions.width;
            _properties.constraints.video.mandatory.maxHeight =
              _properties.videoDimensions.height;
          } else {
            
          }
        }

        if (_properties.frameRate !== void 0 &&
          OT.$.arrayIndexOf(_validFrameRates, _properties.frameRate) === -1) {
          OT.warn('Invalid frameRate passed to the publisher got: ' +
          _properties.frameRate + ' expecting one of ' + _validFrameRates.join(','));
          delete _properties.frameRate;
        } else if (_properties.frameRate) {
          _setupVideoDefaults();
          _properties.constraints.video.optional =
            _properties.constraints.video.optional.concat([
              { minFrameRate: _properties.frameRate },
              { maxFrameRate: _properties.frameRate }
            ]);
        }

      }

    } else {
      OT.warn('You have passed your own constraints not using ours');
    }

    if (_properties.style) {
      this.setStyle(_properties.style, null, true);
    }

    if (_properties.name) {
      _properties.name = _properties.name.toString();
    }

    _properties.classNames = 'OT_root OT_publisher';

    
    
    OT.onLoad(function() {
      _widgetView = new OT.WidgetView(targetElement, _properties);
      _publisher.id = _domId = _widgetView.domId();
      _publisher.element = _widgetView.domElement;

      _widgetView.on('videoDimensionsChanged', function(oldValue, newValue) {
        if (_stream) {
          _stream.setVideoDimensions(newValue.width, newValue.height);
        }
        _publisher.dispatchEvent(
          new OT.VideoDimensionsChangedEvent(_publisher, oldValue, newValue)
        );
      });

      _widgetView.on('mediaStopped', function() {
        var event = new OT.MediaStoppedEvent(_publisher);

        _publisher.dispatchEvent(event, function() {
          if(!event.isDefaultPrevented()) {
            if (_session) {
              _publisher._.unpublishFromSession(_session, 'mediaStopped');
            } else {
              _publisher.destroy('mediaStopped');
            }
          }
        });
      });

      OT.$.waterfall([
        function(cb) {
          if (_isScreenSharing) {
            OT.checkScreenSharingCapability(function(response) {
              if (!response.supported) {
                onScreenSharingError(
                  new Error('Screen Sharing is not supported in this browser')
                );
              } else if (response.extensionRegistered === false) {
                onScreenSharingError(
                  new Error('Screen Sharing suppor in this browser requires an extension, but ' +
                    'one has not been registered.')
                );
              } else if (response.extensionInstalled === false) {
                onScreenSharingError(
                  new Error('Screen Sharing suppor in this browser requires an extension, but ' +
                    'the extension is not installed.')
                );
              } else {

                var helper = OT.pickScreenSharingHelper();

                if (helper.proto.getConstraintsShowsPermissionUI) {
                  onAccessDialogOpened();
                }

                helper.instance.getConstraints(options.videoSource, _properties.constraints,
                  function(err, constraints) {
                  if (helper.proto.getConstraintsShowsPermissionUI) {
                    onAccessDialogClosed();
                  }
                  if (err) {
                    if (err.message === 'PermissionDeniedError') {
                      onAccessDenied(err);
                    } else {
                      onScreenSharingError(err);
                    }
                  } else {
                    _properties.constraints = constraints;
                    cb();
                  }
                });
              }
            });
          } else {
            OT.$.shouldAskForDevices(function(devices) {
              if(!devices.video) {
                OT.warn('Setting video constraint to false, there are no video sources');
                _properties.constraints.video = false;
              }
              if(!devices.audio) {
                OT.warn('Setting audio constraint to false, there are no audio sources');
                _properties.constraints.audio = false;
              }
              cb();
            });
          }
        },

        function() {

          if (_state.isDestroyed()) {
            return;
          }

          OT.$.getUserMedia(
            _properties.constraints,
            onStreamAvailable,
            onStreamAvailableError,
            onAccessDialogOpened,
            onAccessDialogClosed,
            onAccessDenied
          );
        }

      ]);

    }, this);

    return this;
  };















  this.publishAudio = function(value) {
    _properties.publishAudio = value;

    if (_microphone) {
      _microphone.muted(!value);
    }

    if (_chrome) {
      _chrome.muteButton.muted(!value);
    }

    if (_session && _stream) {
      _stream.setChannelActiveState('audio', value);
    }

    return this;
  };
















  this.publishVideo = function(value) {
    var oldValue = _properties.publishVideo;
    _properties.publishVideo = value;

    if (_session && _stream && _properties.publishVideo !== oldValue) {
      _stream.setChannelActiveState('video', value);
    }

    
    
    
    if (_webRTCStream) {
      var videoTracks = _webRTCStream.getVideoTracks();
      for (var i=0, num=videoTracks.length; i<num; ++i) {
        videoTracks[i].setEnabled(value);
      }
    }

    setAudioOnly(!value);

    return this;
  };

  









  this.destroy = function( reason, quiet) {
    if (_state.isDestroyed()) return;
    _state.set('Destroyed');

    reset();

    if (quiet !== true) {
      this.dispatchEvent(
        new OT.DestroyedEvent(
          OT.Event.names.PUBLISHER_DESTROYED,
          this,
          reason
        ),
        OT.$.bind(this.off, this)
      );
    }

    return this;
  };

  



  this.disconnect = function() {
    
    for (var fromConnectionId in _peerConnections) {
      this.cleanupSubscriber(fromConnectionId);
    }
  };

  this.cleanupSubscriber = function(fromConnectionId) {
    var pc = _peerConnections[fromConnectionId];

    if (pc) {
      pc.destroy();
      delete _peerConnections[fromConnectionId];

      logAnalyticsEvent('disconnect', 'PeerConnection',
        {subscriberConnection: fromConnectionId});
    }
  };


  this.processMessage = function(type, fromConnection, message) {
    OT.debug('OT.Publisher.processMessage: Received ' + type + ' from ' + fromConnection.id);
    OT.debug(message);

    switch (type) {
      case 'unsubscribe':
        this.cleanupSubscriber(message.content.connection.id);
        break;

      default:
        var peerConnection = createPeerConnectionForRemote(fromConnection);
        peerConnection.processMessage(type, message);
    }
  };

  




















  this.getImgData = function() {
    if (!_loaded) {
      OT.error('OT.Publisher.getImgData: Cannot getImgData before the Publisher is publishing.');

      return null;
    }

    return _targetElement.imgData();
  };


  
  this._ = {
    publishToSession: OT.$.bind(function(session) {
      
      this.session = _session = session;

      _streamId = OT.$.uuid();
      var createStream = function() {

        var streamWidth,
            streamHeight;

        
        
        if (!_session) return;

        _state.set('PublishingToSession');

        var onStreamRegistered = OT.$.bind(function(err, streamId, message) {
          if (err) {
            
            
            var errorCode = OT.ExceptionCodes.UNABLE_TO_PUBLISH;
            var payload = {
              reason: 'Publish',
              code: errorCode,
              message: err.message
            };
            logConnectivityEvent('Failure', payload);
            if (_state.isAttemptingToPublish()) {
              this.trigger('publishComplete', new OT.Error(errorCode, err.message));
            }
            return;
          }

          this.streamId = _streamId = streamId;
          _iceServers = OT.Raptor.parseIceServers(message);
        }, this);

        
        
        if (_properties.videoDimensions) {
          streamWidth = Math.min(_properties.videoDimensions.width,
            _targetElement.videoWidth() || 640);
          streamHeight = Math.min(_properties.videoDimensions.height,
            _targetElement.videoHeight() || 480);
        } else {
          streamWidth = _targetElement.videoWidth() || 640;
          streamHeight = _targetElement.videoHeight() || 480;
        }

        var streamChannels = [];

        if (!(_properties.videoSource === null || _properties.videoSource === false)) {
          streamChannels.push(new OT.StreamChannel({
            id: 'video1',
            type: 'video',
            active: _properties.publishVideo,
            orientation: OT.VideoOrientation.ROTATED_NORMAL,
            frameRate: _properties.frameRate,
            width: streamWidth,
            height: streamHeight,
            source: _isScreenSharing ? 'screen' : 'camera',
            fitMode: _properties.fitMode
          }));
        }

        if (!(_properties.audioSource === null || _properties.audioSource === false)) {
          streamChannels.push(new OT.StreamChannel({
            id: 'audio1',
            type: 'audio',
            active: _properties.publishAudio
          }));
        }

        session._.streamCreate(_properties.name || '', _streamId,
          _properties.audioFallbackEnabled, streamChannels, onStreamRegistered);

      };

      if (_loaded) createStream.call(this);
      else this.on('initSuccess', createStream, this);

      logConnectivityEvent('Attempt', {streamType: 'WebRTC'});

      return this;
    }, this),

    unpublishFromSession: OT.$.bind(function(session, reason) {
      if (!_session || session.id !== _session.id) {
        OT.warn('The publisher ' + _guid + ' is trying to unpublish from a session ' +
          session.id + ' it is not attached to (it is attached to ' +
          (_session && _session.id || 'no session') + ')');
        return this;
      }

      if (session.isConnected() && this.stream) {
        session._.streamDestroy(this.stream.id);
      }

      
      
      this.disconnect();
      this.session = _session = null;

      
      if (!_state.isDestroyed()) _state.set('MediaBound');

      if(_connectivityAttemptPinger) {
        _connectivityAttemptPinger.stop();
      }
      logAnalyticsEvent('unpublish', 'Success', {sessionId: session.id});

      this._.streamDestroyed(reason);

      return this;
    }, this),

    streamDestroyed: OT.$.bind(function(reason) {
      if(OT.$.arrayIndexOf(['reset'], reason) < 0) {
        var event = new OT.StreamEvent('streamDestroyed', _stream, reason, true);
        var defaultAction = OT.$.bind(function() {
          if(!event.isDefaultPrevented()) {
            this.destroy();
          }
        }, this);
        this.dispatchEvent(event, defaultAction);
      }
    }, this),


    archivingStatus: OT.$.bind(function(status) {
      if(_chrome) {
        _chrome.archive.setArchiving(status);
      }

      return status;
    }, this),

    webRtcStream: function() {
      return _webRTCStream;
    },

    


    switchAcquiredWindow: function(windowId) {

      if (OT.$.env.name !== 'Firefox' || OT.$.env.version < 38) {
        throw new Error('switchAcquiredWindow is an experimental method and is not supported by' +
        'the current platform');
      }

      if (typeof windowId !== 'undefined') {
        _properties.constraints.video.browserWindow = windowId;
      }

      return new Promise(function(resolve, reject) {
        OT.$.getUserMedia(
          _properties.constraints,
          function(newStream) {

            cleanupLocalStream();
            _webRTCStream = newStream;

            _microphone = new OT.Microphone(_webRTCStream, !_properties.publishAudio);

            var videoContainerOptions = {
              muted: true,
              error: onVideoError
            };

            _targetElement = _widgetView.bindVideo(_webRTCStream, videoContainerOptions,
              function(err) {
                if (err) {
                  onLoadFailure(err);
                  reject(err);
                }
              });

            if (_audioLevelSampler && _webRTCStream.getAudioTracks().length > 0) {
              _audioLevelSampler.webRTCStream(_webRTCStream);
            }

            var replacePromises = [];

            Object.keys(_peerConnections).forEach(function(connectionId) {
              var peerConnection = _peerConnections[connectionId];
              peerConnection.getSenders().forEach(function(sender) {
                if (sender.track.kind === 'audio' && newStream.getAudioTracks().length) {
                  replacePromises.push(sender.replaceTrack(newStream.getAudioTracks()[0]));
                } else if (sender.track.kind === 'video' && newStream.getVideoTracks().length) {
                  replacePromises.push(sender.replaceTrack(newStream.getVideoTracks()[0]));
                }
              });
            });

            Promise.all(replacePromises).then(resolve, reject);
          },
          function(error) {
            onStreamAvailableError(error);
            reject(error);
          },
          onAccessDialogOpened,
          onAccessDialogClosed,
          function(error) {
            onAccessDenied(error);
            reject(error);
          });
      });
    }
  };

  this.detectDevices = function() {
    OT.warn('Fixme: Haven\'t implemented detectDevices');
  };

  this.detectMicActivity = function() {
    OT.warn('Fixme: Haven\'t implemented detectMicActivity');
  };

  this.getEchoCancellationMode = function() {
    OT.warn('Fixme: Haven\'t implemented getEchoCancellationMode');
    return 'fullDuplex';
  };

  this.setMicrophoneGain = function() {
    OT.warn('Fixme: Haven\'t implemented setMicrophoneGain');
  };

  this.getMicrophoneGain = function() {
    OT.warn('Fixme: Haven\'t implemented getMicrophoneGain');
    return 0.5;
  };

  this.setCamera = function() {
    OT.warn('Fixme: Haven\'t implemented setCamera');
  };

  this.setMicrophone = function() {
    OT.warn('Fixme: Haven\'t implemented setMicrophone');
  };


  

  this.guid = function() {
    return _guid;
  };

  this.videoElement = function() {
    return _targetElement.domElement();
  };

  this.setStream = assignStream;

  this.isWebRTC = true;

  this.isLoading = function() {
    return _widgetView && _widgetView.loading();
  };

  this.videoWidth = function() {
    return _targetElement.videoWidth();
  };

  this.videoHeight = function() {
    return _targetElement.videoHeight();
  };

  

  this.on('styleValueChanged', updateChromeForStyleChange, this);
  _state = new OT.PublishingState(stateChangeFailed);

  this.accessAllowed = false;







































  
















































































};


OT.Publisher.nextId = OT.$.uuid;











































OT.initSession = function(apiKey, sessionId) {

  if(sessionId == null) {
    sessionId = apiKey;
    apiKey = null;
  }

  var session = OT.sessions.get(sessionId);

  if (!session) {
    session = new OT.Session(apiKey, sessionId);
    OT.sessions.add(session);
  }

  return session;
};




















































































































































































































































































OT.initPublisher = function(targetElement, properties, completionHandler) {
  OT.debug('OT.initPublisher('+targetElement+')');

  
  
  if(targetElement != null && !(OT.$.isElementNode(targetElement) ||
    (typeof targetElement === 'string' && document.getElementById(targetElement))) &&
    typeof targetElement !== 'function') {
    targetElement = properties;
    properties = completionHandler;
    completionHandler = arguments[3];
  }

  if(typeof targetElement === 'function') {
    completionHandler = targetElement;
    properties = undefined;
    targetElement = undefined;
  }

  if(typeof properties === 'function') {
    completionHandler = properties;
    properties = undefined;
  }

  var publisher = new OT.Publisher(properties);
  OT.publishers.add(publisher);

  var triggerCallback = function triggerCallback (err) {
    if (err) {
      OT.dispatchError(err.code, err.message, completionHandler, publisher.session);
    } else if (completionHandler && OT.$.isFunction(completionHandler)) {
      completionHandler.apply(null, arguments);
    }
  },

  removeInitSuccessAndCallComplete = function removeInitSuccessAndCallComplete (err) {
    publisher.off('publishComplete', removeHandlersAndCallComplete);
    triggerCallback(err);
  },

  removeHandlersAndCallComplete = function removeHandlersAndCallComplete (err) {
    publisher.off('initSuccess', removeInitSuccessAndCallComplete);

    
    
    if (err) triggerCallback(err);
  };


  publisher.once('initSuccess', removeInitSuccessAndCallComplete);
  publisher.once('publishComplete', removeHandlersAndCallComplete);

  publisher.publish(targetElement);

  return publisher;
};









































OT.getDevices = function(callback) {
  OT.$.getMediaDevices(callback);
};



OT.reportIssue = function(){
  OT.warn('ToDo: haven\'t yet implemented OT.reportIssue');
};

OT.components = {};


















































































































































































































































OT.onUnload(function() {
  OT.publishers.destroy();
  OT.subscribers.destroy();
  OT.sessions.destroy('unloaded');
});

loadCSS(OT.properties.cssURL);







if (typeof define === 'function' && define.amd) {
  define( 'TB', [], function () { return TB; } );
}



})(window, window.OT);


})(window || exports);