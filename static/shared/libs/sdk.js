








(function(window) {
if (!window.OT) window.OT = {};

OT.properties = {
  version: "v2.0.18.1",         
  build: "7333ca3",    

  debug: "false",      
  websiteURL: "http://www.tokbox.com",      

  cdnURL: "http://static.opentok.com",        
  loggingURL: "http://hlg.tokbox.com/prod",   
  apiURL: "http://anvil.opentok.com",          

  messagingProtocol: "wss",         
  messagingPort: 443,               

  supportSSL: "true",           
  cdnURLSSL: "https://static.opentok.com",         
  loggingURLSSL: "https://hlg.tokbox.com/prod",    
  apiURLSSL: "https://anvil.opentok.com"             
};

})(window);



























!(function(window, undefined) {


var OTHelpers = function(domId) {
    return document.getElementById(domId);
};

var previousOTHelpers = window.OTHelpers;

window.OTHelpers = OTHelpers;

OTHelpers.noConflict = function() {
  OTHelpers.noConflict = function() {
    return OTHelpers;
  };
  window.OTHelpers = previousOTHelpers;
  return OTHelpers;
};


OTHelpers.isEmpty = function(obj) {
  if (obj === null || obj === undefined) return true;
  if (Array.isArray(obj) || typeof(obj) === 'string') return obj.length === 0;

  
  for (var key in obj) {
    if (obj.hasOwnProperty(key)) return false;
  }

  return true;
};

OTHelpers.isNone = function(obj) {
  return obj === undefined || obj === null;
};

OTHelpers.isObject = function(obj) {
  return obj === Object(obj);
};


OTHelpers.isFunction = function(obj) {
    return typeof obj === 'function';
};







OTHelpers.extend = function() {
    var sources = Array.prototype.slice.call(arguments),
        dest = sources.shift();

    sources.forEach(function(source) {
        for (var key in source) {
            dest[key] = source[key];
        }
    });

    return dest;
};








OTHelpers.defaults = function() {
    var sources = Array.prototype.slice.call(arguments),
        dest = sources.shift();

    sources.forEach(function(source) {
        for (var key in source) {
            if (dest[key] === void 0) dest[key] = source[key];
        }
    });

    return dest;
};


OTHelpers.clone = function(obj) {
    if (!OTHelpers.isObject(obj)) return obj;
    return Array.isArray(obj) ? obj.slice() : OTHelpers.extend({}, obj);
};




OTHelpers.noop = function() {};


OTHelpers.supportsWebSockets = function() {
    return 'WebSocket' in window;
};






OTHelpers.now = (function() {
    var performance = window.performance || {},
        navigationStart,
        now =  performance.now       ||
               performance.mozNow    ||
               performance.msNow     ||
               performance.oNow      ||
               performance.webkitNow;

    if (now) {
        now = now.bind(performance);
        navigationStart = performance.timing.navigationStart;

        return  function() { return navigationStart + now(); };
    }
    else {
        return function() { return new Date().getTime(); };
    }
})();

OTHelpers.browser = function() {
    var userAgent = window.navigator.userAgent.toLowerCase(),
        navigatorVendor,
        browser = 'Unknown';

    if (userAgent.indexOf('firefox') > -1)   {
        browser = 'Firefox';
    }
    if (userAgent.indexOf('opera') > -1)   {
        browser = 'Opera';
    }
    else if (userAgent.indexOf("msie") > -1) {
        browser = "IE";
    }
    else if (userAgent.indexOf("chrome") > -1) {
        browser = "Chrome";
    }

    if ((navigatorVendor = window.navigator.vendor) && navigatorVendor.toLowerCase().indexOf("apple") > -1) {
        browser = "Safari";
    }

    userAgent = null;
    OTHelpers.browser = function() { return browser; };
    return browser;
};


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

  Object.defineProperties(self, propsDefinition);
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

OTHelpers.setCookie = function(key, value) {
  try {
    localStorage.setItem(key, value);
  } catch (err) {
    
    var date = new Date();
    date.setTime(date.getTime()+(365*24*60*60*1000));
    var expires = "; expires="+date.toGMTString();
    document.cookie = key+"="+value+expires+"; path=/";
  }
};

OTHelpers.getCookie = function(key) {
    var value;

    try {
      value = localStorage.getItem("opentok_client_id");
      return value;
    } catch (err) {
      
      var nameEQ = key + "=";
      var ca = document.cookie.split(';');
      for(var i=0;i < ca.length;i++) {
        var c = ca[i];
        while (c.charAt(0)==' ') c = c.substring(1,c.length);
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





  
  OTHelpers.invert = function(obj) {
    var result = {};
    for (var key in obj) if (obj.hasOwnProperty(key)) result[obj[key]] = key;
    return result;
  };


  
  var entityMap = {
    escape: {
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      '"': '&quot;',
      "'": '&#x27;',
      '/': '&#x2F;'
    }
  };
  entityMap.unescape = OTHelpers.invert(entityMap.escape);

  
  var entityRegexes = {
    escape:   new RegExp('[' + Object.keys(entityMap.escape).join('') + ']', 'g'),
    unescape: new RegExp('(' + Object.keys(entityMap.unescape).join('|') + ')', 'g')
  };

  
  ['escape', 'unescape'].forEach(function(method) {
    OTHelpers[method] = function(string) {
      if (string === null || string === undefined) return '';
      return ('' + string).replace(entityRegexes[method], function(match) {
        return entityMap[method][match];
      });
    };
  });



OTHelpers.templateSettings = {
  evaluate    : /<%([\s\S]+?)%>/g,
  interpolate : /<%=([\s\S]+?)%>/g,
  escape      : /<%-([\s\S]+?)%>/g
};




var noMatch = /(.)^/;



var escapes = {
  "'":      "'",
  '\\':     '\\',
  '\r':     'r',
  '\n':     'n',
  '\t':     't',
  '\u2028': 'u2028',
  '\u2029': 'u2029'
};

var escaper = /\\|'|\r|\n|\t|\u2028|\u2029/g;




OTHelpers.template = function(text, data, settings) {
  var render;
  settings = OTHelpers.defaults({}, settings, OTHelpers.templateSettings);

  
  var matcher = new RegExp([
    (settings.escape || noMatch).source,
    (settings.interpolate || noMatch).source,
    (settings.evaluate || noMatch).source
  ].join('|') + '|$', 'g');

  
  var index = 0;
  var source = "__p+='";
  text.replace(matcher, function(match, escape, interpolate, evaluate, offset) {
    source += text.slice(index, offset)
      .replace(escaper, function(match) { return '\\' + escapes[match]; });

    if (escape) {
      source += "'+\n((__t=(" + escape + "))==null?'':OTHelpers.escape(__t))+\n'";
    }
    if (interpolate) {
      source += "'+\n((__t=(" + interpolate + "))==null?'':__t)+\n'";
    }
    if (evaluate) {
      source += "';\n" + evaluate + "\n__p+='";
    }
    index = offset + match.length;
    return match;
  });
  source += "';\n";

  
  if (!settings.variable) source = 'with(obj||{}){\n' + source + '}\n';

  source = "var __t,__p='',__j=Array.prototype.join," +
    "print=function(){__p+=__j.call(arguments,'');};\n" +
    source + "return __p;\n";


  try {
    
    
    render = new Function(settings.variable || 'obj', source);
  } catch (e) {
    e.source = source;
    throw e;
  }

  if (data) return render(data);
  var template = function(data) {
    return render.call(this, data);
  };

  
  template.source = 'function(' + (settings.variable || 'obj') + '){\n' + source + '}';

  return template;
};

})(window);




(function(window, OTHelpers, undefined) {

OTHelpers.statable = function(self, possibleStates, initialState, stateChanged, stateChangedFailed) {
  var previousState,
      currentState = initialState;

  var setState = function(state) {
    if (currentState !== state) {
        if (possibleStates.indexOf(state) === -1) {
          if (stateChangedFailed && OTHelpers.isFunction(stateChangedFailed)) stateChangedFailed('invalidState', state);
          return;
        }

        previousState = currentState;
        currentState = state;
        if (stateChanged && OTHelpers.isFunction(stateChanged)) stateChanged(state, previousState);
    }
  };


  
  
  
  
  
  
  
  
  self.is = function () {
    return Array.prototype.indexOf.call(arguments, currentState) !== -1;
  };


  
  
  
  
  
  
  
  
  self.isNot = function () {
    return Array.prototype.indexOf.call(arguments, currentState) === -1;
  };

  Object.defineProperties(self, {
    state: {
      get: function() { return currentState; }
    },

    previousState: {
      get: function() { return previousState; }
    }
  });

  return setState;
};

})(window, window.OTHelpers);

















(function(window, OTHelpers, undefined) {

  
  
  
  
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

  
  var BufferClass = typeof(Buffer) == 'function' ? Buffer : Array;

  
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

    if (typeof(options) == 'string') {
      buf = options == 'binary' ? new BufferClass(16) : null;
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

}(window, window.OTHelpers));







(function(window, OTHelpers, undefined) {


function shimXMLForIE8(xml) {
    
    Object.defineProperty(xml.prototype, "firstElementChild", {
        "get" : function() {
            var node = this;
            node = node.firstChild;
            while(node && node.nodeType != 1) node = node.nextSibling;
            return node;
        }
    });

    
    Object.defineProperty(xml.prototype, "lastElementChild", {
        "get" : function() {
            var node = this;
            node = node.lastChild;
            while(node && node.nodeType != 1) node = node.previousSibling;
            return node;
        }
    });

    
    Object.defineProperty(xml.prototype, "nextElementSibling", {
        "get" : function() {
            var node = this;
            while(!OTHelpers.isNone(node = node.nextSibling)) {
                if(node.nodeType == 1) break;
            }

            return node;
        }
    });

    
    Object.defineProperty(xml.prototype, "previousElementSibling", {
        "get" : function() {
            var node = this;
            while(!OTHelpers.isNone(node = node.previousSibling)) {
                if(node.nodeType == 1) break;
            }
            return node;
        }
    });
}

OTHelpers.parseXML = function(xmlString) {
    var root, xml;

    if (window.DOMParser) { 
        xml = (new DOMParser()).parseFromString(xmlString, "text/xml");
    } else { 
        xml = new ActiveXObject("Microsoft.XMLDOM");
        xml.async = "false";
        xml.loadXML(xmlString);

        shimXMLForIE8(xml);
    }

    root = xml.documentElement;

    if (!root || !root.nodeName || root.nodeName === "parsererror") {
        
        return null;
    }

    return xml;
};

})(window, window.OTHelpers);




(function(window, OTHelpers, undefined) {

OTHelpers.useLogHelpers = function(on){

    
    on.DEBUG    = 5;
    on.LOG      = 4;
    on.INFO     = 3;
    on.WARN     = 2;
    on.ERROR    = 1;
    on.NONE     = 0;

    var _logLevel = on.NONE,
        _logs = [];

    
    
    
    
    
    
    
    
    function generateLoggingMethod(method, level, fallback) {
        return function() {
            if (on.shouldLog(level)) {
                var cons = window.console;

                
                if (cons && cons[method]) {
                    
                    
                    
                    if (cons[method].apply || Function.prototype.bind) {
                        if (!cons[method].apply) {
                            cons[method] = Function.prototype.bind.call(cons[method], cons);
                        }

                        cons[method].apply(cons, arguments);
                    }
                    else {
                        
                        
                        cons[method](
                            Array.prototype.slice.apply(arguments).join(' ')
                        );
                    }
                }
                else if (fallback) {
                    fallback.apply(on, arguments);
                }

                appendToLogs(method, arguments);
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
        on.debug("TB.setLogLevel(" + _logLevel + ")");
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


    
    function appendToLogs(level, args) {
        if (!args) return;

        var message;

        try {
            message = JSON.stringify(args);
        } catch(e) {
            message = args.toString();
        }

        if (message.length <= 2) return;

        _logs.push(
            [level, formatDateStamp(), message]
        );
    }

};

OTHelpers.useLogHelpers(OTHelpers);
OTHelpers.setLogLevel(OTHelpers.ERROR);

})(window, window.OTHelpers);




(function(window, OTHelpers, undefined) {

OTHelpers.castToBoolean = function(value, defaultValue) {
    if (value === undefined) return defaultValue;
    return value === 'true' || value === true;
};

OTHelpers.roundFloat = function(value, places) {
    return Number(value.toFixed(places));
};

})(window, window.OTHelpers);





(function(window, OTHelpers, undefined) {

var timeouts = [],
    messageName = "OTHelpers." + OTHelpers.uuid.v4() + ".zero-timeout";

var handleMessage = function(event) {



    if (event.data == messageName) {
        event.stopPropagation();
        if (timeouts.length > 0) {
            var args = timeouts.shift(),
                fn = args.shift();

            fn.apply(null, args);
        }
    }
};

window.addEventListener("message", handleMessage, true);













OTHelpers.callAsync = function () {
    timeouts.push(Array.prototype.slice.call(arguments));
    window.postMessage(messageName, "*");
};





OTHelpers.createAsyncHandler = function(handler) {
    return function() {
        var args = Array.prototype.slice.call(arguments);

        OTHelpers.callAsync(function() {
          handler.apply(null, args);
        });
    };
};

})(window, window.OTHelpers);







(function(window, OTHelpers, undefined) {







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

    listeners.forEach(function(listener) { 
      function filterHandlerAndContext(_listener) {
        return _listener.context === listener.context && _listener.handler === listener.handler;
      }

      
      OTHelpers.callAsync(function() {
        try {
          
          if (_events[name] && _events[name].some(filterHandlerAndContext)) {
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

    listeners.forEach(function(listener) { 
      (listener.closure || listener.handler).apply(listener.context || null, args);
    });
  }

  var executeListeners = syncronous === true ? executeListenersSyncronously : executeListenersAsyncronously;


  var removeAllListenersNamed = function (eventName, context) {
    if (_events[eventName]) {
      if (context) {
        
        
        _events[eventName] = _events[eventName].filter(function(listener){
          return listener.context !== context;
        });
      }
      else {
        delete _events[eventName];
      }
    }
  };

  var addListeners = function (eventNames, handler, context, closure) {
    var listener = {handler: handler};
    if (context) listener.context = context;
    if (closure) listener.closure = closure;

    eventNames.forEach(function(name) {
      if (!_events[name]) _events[name] = [];
      _events[name].push(listener);
    });
  }.bind(self);


  var removeListeners = function (eventNames, handler, context) {
    function filterHandlerAndContext(listener) {
      return !(listener.handler === handler && listener.context === context);
    }

    eventNames.forEach(function(name) {
      if (_events[name]) {
        _events[name] = _events[name].filter(filterHandlerAndContext);
        if (_events[name].length === 0) delete _events[name];
      }
    });
  }.bind(self);


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  self.dispatchEvent = function(event, defaultAction) {
    if (!event.type) {
      OTHelpers.error("OTHelpers.Eventing.dispatchEvent: Event has no type");
      OTHelpers.error(event);

      throw new Error("OTHelpers.Eventing.dispatchEvent: Event has no type");
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

    var args = Array.prototype.slice.call(arguments);

    
    args.shift();

    executeListeners(eventName, args);

    return this;
  };

  self.on = function(eventNames, handler_or_context, context) {
    if (typeof(eventNames) === "string" && handler_or_context) {
      addListeners(eventNames.split(' '), handler_or_context, context);
    }
    else {
      for (var name in eventNames) {
        if (eventNames.hasOwnProperty(name)) {
          addListeners([name], eventNames[name], handler_or_context);
        }
      }
    }

    return this;
  };

 self.off = function(eventNames, handler_or_context, context) {
    if (typeof(eventNames) === "string") {
      if (handler_or_context && OTHelpers.isFunction(handler_or_context)) {
        removeListeners(eventNames.split(' '), handler_or_context, context);
      }
      else {
        eventNames.split(' ').forEach(function(name) {
          removeAllListenersNamed(name, handler_or_context);
        }, this);
      }
    }
    else if (!eventNames) {
      
      _events = {};
    }
    else {
      for (var name in eventNames) {
        if (eventNames.hasOwnProperty(name)) {
          removeListeners([name], eventNames[name], handler_or_context);
        }
      }
    }

    return this;
  };


  self.once = function(eventNames, handler, context) {
    var names = eventNames.split(' '),
        fun = function() {
          var result = handler.apply(context || null, arguments);
          removeListeners(names, handler, context);

          return result;
        }.bind(this);

    addListeners(names, handler, context, fun);
    return this;
  };


  



















  
  
  self.addEventListener = function(eventName, handler, context) {
    addListeners([eventName], handler, context);
  };


  















  
  
  self.removeEventListener = function(eventName, handler, context) {
    removeListeners([eventName], handler, context);
  };





  return self;
};

OTHelpers.eventing.Event = function() {

    return function (type, cancelable) {
        this.type = type;
        this.cancelable = cancelable !== undefined ? cancelable : true;

        var _defaultPrevented = false,
            _target = null;

        this.preventDefault = function() {
            if (this.cancelable) {
                _defaultPrevented = true;
            } else {
                OTHelpers.warn("Event.preventDefault :: Trying to preventDefault on an Event that isn't cancelable");
            }
        };

        this.isDefaultPrevented = function() {
            return _defaultPrevented;
        };

        if (OTHelpers.canDefineProperty) {
            Object.defineProperty(this, 'target', {
                set: function(target) {
                    _target = target;
                },

                get: function() {
                    return _target;
                }
            });
        }
    };

};
})(window, window.OTHelpers);






(function(window, OTHelpers, undefined) {


OTHelpers.supportsClassList = function() {
    var hasSupport = typeof(document !== "undefined") && ("classList" in document.createElement("a"));
    OTHelpers.supportsClassList = function() { return hasSupport; };

    return hasSupport;
};

OTHelpers.removeElement = function(element) {
    if (element && element.parentNode) {
        element.parentNode.removeChild(element);
    }
};

OTHelpers.removeElementById = function(elementId) {
    this.removeElement(OTHelpers(elementId));
};

OTHelpers.removeElementsByType = function(parentElem, type) {
    if (!parentElem) return;

    var elements = parentElem.getElementsByTagName(type);

    
    
    
    
    
    while (elements.length) {
        parentElem.removeChild(elements[0]);
    }
};

OTHelpers.emptyElement = function(element) {
    while (element.firstChild) {
        element.removeChild(element.firstChild);
    }
    return element;
};

OTHelpers.createElement = function(nodeName, attributes, innerHTML) {
    var element = document.createElement(nodeName);

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

    if (innerHTML) {
        element.innerHTML = innerHTML;
    }

    return element;
};

OTHelpers.createButton = function(innerHTML, attributes, events) {
    var button = OTHelpers.createElement('button', attributes, innerHTML);

    if (events) {
        for (var name in events) {
            if (events.hasOwnProperty(name)) {
                OTHelpers.on(button, name, events[name]);
            }
        }

        button._boundEvents = events;
    }

    return button;
};





OTHelpers.on = function(element, eventName,  handler) {
    if (element.addEventListener) {
        element.addEventListener(eventName, handler, false);
    } else if (element.attachEvent) {
        element.attachEvent("on" + eventName, handler);
    } else {
        var oldHandler = element["on"+eventName];
        element["on"+eventName] = function() {
          handler.apply(this, arguments);
          if (oldHandler) oldHandler.apply(this, arguments);
        };
    }
};


OTHelpers.off = function(element, eventName, handler) {
    if (element.removeEventListener) {
        element.removeEventListener (eventName, handler,false);
    }
    else if (element.detachEvent) {
        element.detachEvent("on" + eventName, handler);
    }
};



OTHelpers.isDisplayNone = function(element) {
    if ( (element.offsetWidth === 0 || element.offsetHeight === 0) && OTHelpers.css(element, 'display') === 'none') return true;
    if (element.parentNode && element.parentNode.style) return OTHelpers.isDisplayNone(element.parentNode);
    return false;
};

OTHelpers.findElementWithDisplayNone = function(element) {
    if ( (element.offsetWidth === 0 || element.offsetHeight === 0) && OTHelpers.css(element, 'display') === 'none') return element;
    if (element.parentNode && element.parentNode.style) return OTHelpers.findElementWithDisplayNone(element.parentNode);
    return null;
};

function objectHasProperties(obj) {
    for (var key in obj) {
        if (obj.hasOwnProperty(key)) return true;
    }
    return false;
}
























OTHelpers.observeStyleChanges = function(element, stylesToObserve, onChange) {
    var oldStyles = {};

    var getStyle = function getStyle(style) {
            switch (style) {
            case 'width':
                return OTHelpers.width(element);

            case 'height':
                return OTHelpers.height(element);

            default:
                return OTHelpers.css(element);
            }
        };

    
    stylesToObserve.forEach(function(style) {
        oldStyles[style] = getStyle(style);
    });

    var observer = new MutationObserver(function(mutations) {
        var changeSet = {};

        mutations.forEach(function(mutation) {
            if (mutation.attributeName !== 'style') return;

            var isHidden = OTHelpers.isDisplayNone(element);

            stylesToObserve.forEach(function(style) {
                if(isHidden && (style == 'width' || style == 'height')) return;

                var newValue = getStyle(style);

                if (newValue !== oldStyles[style]) {
                    

                    changeSet[style] = [oldStyles[style], newValue];
                    oldStyles[style] = newValue;
                }
            });
        });

        if (objectHasProperties(changeSet)) {
            
            OTHelpers.callAsync(function() {
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























OTHelpers.observeNodeOrChildNodeRemoval = function(element, onChange) {
    var observer = new MutationObserver(function(mutations) {
        var removedNodes = [];

        mutations.forEach(function(mutation) {
            if (mutation.removedNodes.length) {
                removedNodes = removedNodes.concat(Array.prototype.slice.call(mutation.removedNodes));
            }
        });

        if (removedNodes.length) {
            
            OTHelpers.callAsync(function() {
                onChange(removedNodes);
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

})(window, window.OTHelpers);






(function(window, OTHelpers, undefined) {

OTHelpers.Modal = function(title, body) {
    
    var tmpl = "\
        <header>\
            <h1><%= title %></h1>\
        </header>\
        <div class='OT_dialog-body'>\
            <%= body %>\
        </div>\
    ";

    this.el = OTHelpers.createElement("section", {
            className: "OT_root OT_dialog OT_modal"
        },
        OTHelpers.template(tmpl, {
            title: title,
            body: body
        })
    );

    
    
    
    this.el.style.display = 'none';
    document.body.appendChild(this.el);
    OTHelpers.centerElement(this.el);
    OTHelpers.show(this.el);

    this.close = function() {
        OTHelpers.removeElement(this.el);
        this.el = null;
        return this;
    };
};


OTHelpers.tbAlert = function(title, message) {
    var modal = new OTHelpers.Modal(title, "<div>" + message + "</div>");
    OTHelpers.addClass(modal.el, "OT_tbalert");

    var closeBtn = OTHelpers.createElement("input", {
            className: "OT_closeButton",
            type: "button",
            value: "close"
    });
    modal.el.appendChild(closeBtn);

    closeBtn.onclick = function() {
        if (modal) modal.close();
        modal = null;
    };
};

})(window, window.OTHelpers);








(function(window, OTHelpers, undefined) {

OTHelpers.addClass = function(element, value) {
    
    if (element.nodeType !== 1) {
        return;
    }

    var classNames = value.trim().split(/\s+/),
        i, l;

    if (OTHelpers.supportsClassList()) {
        for (i=0, l=classNames.length; i<l; ++i) {
            element.classList.add(classNames[i]);
        }

        return;
    }

    

    if (!element.className && classNames.length === 1) {
        element.className = value;
    }
    else {
        var setClass = " " + element.className + " ";

        for (i=0, l=classNames.length; i<l; ++i) {
            if ( !~setClass.indexOf( " " + classNames[i] + " ")) {
                setClass += classNames[i] + " ";
            }
        }

        element.className = setClass.trim();
    }

    return this;
};

OTHelpers.removeClass = function(element, value) {
    if (!value) return;

    
    if (element.nodeType !== 1) {
        return;
    }

    var newClasses = value.trim().split(/\s+/),
        i, l;

    if (OTHelpers.supportsClassList()) {
        for (i=0, l=newClasses.length; i<l; ++i) {
            element.classList.remove(newClasses[i]);
        }

        return;
    }

    var className = (" " + element.className + " ").replace(/[\s+]/, ' ');

    for (i=0,l=newClasses.length; i<l; ++i) {
        className = className.replace(' ' + newClasses[i] + ' ', ' ');
    }

    element.className = className.trim();

    return this;
};






var _width = function(element) {
        if (element.offsetWidth > 0) {
            return element.offsetWidth + 'px';
        }

        return OTHelpers.css(element, 'width');
    },

    _height = function(element) {
        if (element.offsetHeight > 0) {
            return element.offsetHeight + 'px';
        }

        return OTHelpers.css(element, 'height');
    };

OTHelpers.width = function(element, newWidth) {
    if (newWidth) {
        OTHelpers.css(element, 'width', newWidth);
        return this;
    }
    else {
        if (OTHelpers.isDisplayNone(element)) {
            
            return OTHelpers.makeVisibleAndYield(element, function() {
                return _width(element);
            });
        }
        else {
            return _width(element);
        }
    }
};

OTHelpers.height = function(element, newHeight) {
    if (newHeight) {
        OTHelpers.css(element, 'height', newHeight);
        return this;
    }
    else {
        if (OTHelpers.isDisplayNone(element)) {
            
            return OTHelpers.makeVisibleAndYield(element, function() {
                return _height(element);
            });
        }
        else {
            return _height(element);
        }
    }
};



OTHelpers.centerElement = function(element, width, height) {
    if (!width) width = parseInt(OTHelpers.width(element), 10);
    if (!height) height = parseInt(OTHelpers.height(element), 10);

    var marginLeft = -0.5 * width + "px";
    var marginTop = -0.5 * height + "px";
    OTHelpers.css(element, "margin", marginTop + " 0 0 " + marginLeft);
    OTHelpers.addClass(element, "OT_centered");
};

})(window, window.OTHelpers);







(function(window, OTHelpers, undefined) {

var displayStateCache = {},
    defaultDisplays = {};

var defaultDisplayValueForElement = function(element) {
    if (defaultDisplays[element.ownerDocument] && defaultDisplays[element.ownerDocument][element.nodeName]) {
        return defaultDisplays[element.ownerDocument][element.nodeName];
    }

    if (!defaultDisplays[element.ownerDocument]) defaultDisplays[element.ownerDocument] = {};

    
    
    var testNode = element.ownerDocument.createElement(element.nodeName),
        defaultDisplay;

    element.ownerDocument.body.appendChild(testNode);
    defaultDisplay = defaultDisplays[element.ownerDocument][element.nodeName] = OTHelpers.css(testNode, 'display');

    OTHelpers.removeElement(testNode);
    testNode = null;

    return defaultDisplay;
};

var isHidden = function(element) {
    var computedStyle = element.ownerDocument.defaultView.getComputedStyle(element, null);
    return computedStyle.getPropertyValue('display') === 'none';
};

OTHelpers.show = function(element) {
    var display = element.style.display;
        
        
        

    if (display === '' || display === 'none') {
        element.style.display = displayStateCache[element] || '';
        delete displayStateCache[element];
    }

    if (isHidden(element)) {
        
        
        displayStateCache[element] = 'none';

        element.style.display = defaultDisplayValueForElement(element);
    }

    return this;
};

OTHelpers.hide = function(element) {
    if (element.style.display === 'none') return;

    displayStateCache[element] = element.style.display;
    element.style.display = 'none';

    return this;
};

OTHelpers.css = function(element, nameOrHash, value) {
    if (typeof(nameOrHash) !== 'string') {
        var style = element.style;

        for (var cssName in nameOrHash) {
            style[cssName] = nameOrHash[cssName];
        }

        return this;
    }
    else if (value !== undefined) {
        element.style[nameOrHash] = value;
        return this;
    }
    else {
        
        
        var name = nameOrHash.replace( /([A-Z]|^ms)/g, "-$1" ).toLowerCase(),
            computedStyle = element.ownerDocument.defaultView.getComputedStyle(element, null),
            currentValue = computedStyle.getPropertyValue(name);

        if (currentValue === '') {
            currentValue = element.style[name];
        }

        return currentValue;
    }
};




OTHelpers.applyCSS = function(element, styles, callback) {
    var oldStyles = {},
        name,
        ret;

    
    for (name in styles) {
        if (styles.hasOwnProperty(name)) {
            
            
            
            oldStyles[name] = element.style[name];

            OTHelpers.css(element, name, styles[name]);
        }
    }

    ret = callback();

    
    for (name in styles) {
        if (styles.hasOwnProperty(name)) {
            OTHelpers.css(element, name, oldStyles[name] || '');
        }
    }

    return ret;
};


OTHelpers.makeVisibleAndYield = function(element, callback) {
    
    
    var targetElement = OTHelpers.findElementWithDisplayNone(element);
    if (!targetElement) return;

    return OTHelpers.applyCSS(targetElement, {
            display: "block",
            visibility: "hidden"
        },
        callback
    );
};

})(window, window.OTHelpers);







(function(window, OTHelpers, undefined) {

function formatPostData(data) { 
    
    if (typeof(data) === 'string') return data;

    var queryString = [];

    for (var key in data) {
        queryString.push(
            encodeURIComponent(key) + '=' + encodeURIComponent(data[key])
        );
    }

    return queryString.join('&').replace(/\+/g, "%20");
}

OTHelpers.getXML = function(url, options) {
    var callerSuccessCb = options && options.success,

        isValidXMLDocument = function(xmlDocument) {
            var root;

            if (!xmlDocument) {
                
                return false;
            }

            
            root = xmlDocument.documentElement;

            if (!root || !root.nodeName || root.nodeName === "parsererror") {
                
                return false;
            }

            return true;
        },

        onSuccess = function(event) {
            var response = event.target.responseXML;

            if (isValidXMLDocument(response)) {
                if (callerSuccessCb) callerSuccessCb(response, event, event.target);
            }
            else if (options && options.error) {
                options.error(event, event.target);
            }
        };

    var extendedHeaders = OTHelpers.extend(options.headers || {}, {
            'Content-Type': 'application/xml'
        });

    OTHelpers.get(url, OTHelpers.extend(options || {}, {
        success: onSuccess,
        headers: extendedHeaders
    }));
};

OTHelpers.getJSON = function(url, options) {
    var callerSuccessCb = options && options.success,
        onSuccess = function(event) {
            var response;

            try {
                response = JSON.parse(event.target.responseText);
            } catch(e) {
                
                if (options && options.error) options.error(event, event.target);
                return;
            }

            if (callerSuccessCb) callerSuccessCb(response, event, event.target);
        };

    OTHelpers.get(url, OTHelpers.extend(options || {}, {
        success: onSuccess,
        headers: {
            'Content-Type': 'application/json'
        }
    }));
};

OTHelpers.get = function(url, options) {
    var request = new XMLHttpRequest(),
        _options = options || {};

    bindToSuccessAndErrorCallbacks(request, _options.success, _options.error);
    if (_options.process) request.addEventListener("progress", _options.progress, false);
    if (_options.cancelled) request.addEventListener("abort", _options.cancelled, false);


    request.open('GET', url, true);

    if (!_options.headers) _options.headers = {};

    for (var name in _options.headers) {
        request.setRequestHeader(name, _options.headers[name]);
    }

    request.send();
};

OTHelpers.post = function(url, options) {
    var request = new XMLHttpRequest(),
        _options = options || {};

    bindToSuccessAndErrorCallbacks(request, _options.success, _options.error);

    if (_options.process) request.addEventListener("progress", _options.progress, false);
    if (_options.cancelled) request.addEventListener("abort", _options.cancelled, false);

    request.open('POST', url, true);

    if (!_options.headers) _options.headers = {};

    for (var name in _options.headers) {
        request.setRequestHeader(name, _options.headers[name]);
    }

    request.send(formatPostData(_options.data));
};

OTHelpers.postFormData = function(url, data, options) {
    if (!data) {
        throw new Error("OTHelpers.postFormData must be passed a data options.");
    }

    var formData = new FormData();

    for (var key in data) {
        formData.append(key, data[key]);
    }

    OTHelpers.post(url, OTHelpers.extend(options || {}, {
        data: formData
    }));
};











OTHelpers.getJSONP = function(url, options) {
    var _loadTimeout = 30000,
        _script,
        _head = document.head || document.getElementsByTagName('head')[0],
        _waitUntilTimeout,
        _urlWithCallback = url,
        _options = OTHelpers.extend(options || {}, {
            callbackParameter: 'callback'
        }),

        _clearTimeout = function() {
            if (_waitUntilTimeout) {
                clearTimeout(_waitUntilTimeout);
                _waitUntilTimeout = null;
            }
        },

        _cleanup = function() {
            _clearTimeout();

            if (_script) {
                _script.onload = _script.onreadystatechange = null;

                OTHelpers.removeElement( _script );

                _script = undefined;
            }
        },

        _onLoad = function() {
            if (_script.readyState && !/loaded|complete/.test( _script.readyState )) {
                
                return;
            }

            _clearTimeout();
        },

        _onLoadTimeout = function() {
            _cleanup();

            OTHelpers.error("The JSONP request to " + _urlWithCallback + " timed out after " + _loadTimeout + "ms.");
            if (_options.error) _options.error("The JSONP request to " + url + " timed out after " + _loadTimeout + "ms.", _urlWithCallback,  _options);
        },

        _generateCallbackName = function() {
            return 'jsonp_callback_' + (+new Date());
        };


    _options.callbackName = _generateCallbackName();
    this.jsonp_callbacks[_options.callbackName] = function(response) {
        _cleanup();

        if (_options.success) _options.success(response);
    };

    
    
    _urlWithCallback += ((/\?/).test(_urlWithCallback) ? "&" : "?") + _options.callbackParameter + '=' + _options.callbackName;

    _script = OTHelpers.createElement('script', {
        async: 'async',
        src: _urlWithCallback,
        onload: _onLoad,
        onreadystatechange: _onLoad
    });
    _head.appendChild(_script);

    _waitUntilTimeout = setTimeout(function() { _onLoadTimeout(); }, _loadTimeout);
};





var bindToSuccessAndErrorCallbacks = function(request, success, error) {
    request.addEventListener("load", function(event) {
        var status = event.target.status;

        
        
        if ( status >= 200 && status < 300 || status === 304 ) {
            if (success) success.apply(null, arguments);
        }
        else if (error) {
            error(event);
        }
    }, false);


    if (error) {
        request.addEventListener("error", error, false);
    }
};

})(window, window.OTHelpers);
!(function(window) {

if (!window.OT) window.OT = {};


OT.$ = OTHelpers.noConflict();


OT.$.eventing(OT);


OT.Modal = OT.$.Modal;


OT.$.useLogHelpers(OT);
var _debugHeaderLogged = false;
var _setLogLevel = OT.setLogLevel;


OT.setLogLevel = function(level) {
    
    OT.$.setLogLevel(level);
    var retVal = _setLogLevel.call(OT, level);
    if (OT.shouldLog(OT.DEBUG) && !_debugHeaderLogged) {
        OT.debug("OpenTok JavaScript library " + OT.properties.version);
        OT.debug("Release notes: " + OT.properties.websiteURL  +"/opentok/webrtc/docs/js/release-notes.html");
        OT.debug("Known issues: " + OT.properties.websiteURL + "/opentok/webrtc/docs/js/release-notes.html#knownIssues");
        _debugHeaderLogged = true;
    }
    OT.debug("TB.setLogLevel(" + retVal + ")");
    return retVal;
};

OT.setLogLevel(OT.properties.debug ? OT.DEBUG : OT.ERROR);




































































})(window);
(function(window) {


if (!window.OT) window.OT = {};

if (!OT.properties) {
	throw new Error("OT.properties does not exist, please ensure that you include a valid properties file.");
}


OT.properties = function(properties) {
    var props = OT.$.clone(properties);

    props.debug = properties.debug === 'true' || properties.debug === true;
    props.supportSSL = properties.supportSSL === 'true' || properties.supportSSL === true;

    if (props.supportSSL && (window.location.protocol.indexOf("https") >= 0 || window.location.protocol.indexOf("chrome-extension") >= 0)) {
        props.assetURL = props.cdnURLSSL + "/webrtc/" + props.version;
        props.loggingURL = props.loggingURLSSL;
        props.apiURL = props.apiURLSSL;
    } else {
        props.assetURL = props.cdnURL + "/webrtc/" + props.version;
    }

    props.configURL = props.assetURL + "/js/dynamic_config.min.js";
    props.cssURL = props.assetURL + "/css/ot.min.css";

    return props;
}(OT.properties);

})(window);
(function(window) {






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

        _getModule = function(moduleName, apiKey) {
            if (apiKey && _partners[apiKey] && _partners[apiKey][moduleName]) {
                return _partners[apiKey][moduleName];
            }

            return _global[moduleName];
        },

        _this = {
            
            loadTimeout: 4000,

            load: function(configUrl) {
                if (!configUrl) throw new Error("You must pass a valid configUrl to Config.load");

                _loaded = false;

                setTimeout(function() {
                    _script = document.createElement( "script" );
                    _script.async = "async";
                    _script.src = configUrl;
                    _script.onload = _script.onreadystatechange = _onLoad.bind(this);
                    _head.appendChild(_script);
                },1);

                _loadTimer = setTimeout(function() {
                    _this._onLoadTimeout();
                }, this.loadTimeout);
            },

            _onLoadTimeout: function() {
                _cleanup();

                OT.warn("TB DynamicConfig failed to load in " + _this.loadTimeout + " ms");
                this.trigger('dynamicConfigLoadFailed');
            },

            isLoaded: function() {
                return _loaded;
            },

            reset: function() {
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

})(window);
(function(window) {

var defaultAspectRatio = 4.0/3.0;





function fixAspectRatio(element, width, height, desiredAspectRatio, rotated) {
    if (!width) width = parseInt(OT.$.width(element.parentNode), 10);
    else width = parseInt(width, 10);

    if (!height) height = parseInt(OT.$.height(element.parentNode), 10);
    else height = parseInt(height, 10);

    if (width === 0 || height === 0) return;

    if (!desiredAspectRatio) desiredAspectRatio = defaultAspectRatio;

    var actualRatio = (width + 0.0)/height,
        props = {
            width: '100%',
            height: '100%',
            left: 0,
            top: 0
        };

    if (actualRatio > desiredAspectRatio) {
        
        var newHeight = (actualRatio / desiredAspectRatio) * 100;

        props.height = newHeight + '%';
        props.top = '-' + ((newHeight - 100) / 2) + '%';
    } else if (actualRatio < desiredAspectRatio) {
        
        var newWidth = (desiredAspectRatio / actualRatio) * 100;

        props.width = newWidth + '%';
        props.left = '-' + ((newWidth - 100) / 2) + '%';
    }

    OT.$.css(element, props);

    var video = element.querySelector('video');
    if(video) {
        if(rotated) {
            var w = element.offsetWidth,
                h = element.offsetHeight,
                props = { width: h + 'px', height: w + 'px', marginTop: '', marginLeft: '' },
                diff = w - h;
                props.marginLeft = (diff / 2) + 'px';
                props.marginTop = -(diff / 2) + 'px';
                OT.$.css(video, props);
        } else {
            OT.$.css(video, { width: '', height: '', marginTop: '', marginLeft: ''});
        }
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
    }
    else {
        
        container = OT.$(elementOrDomId);
        domId = elementOrDomId || ('OT_' + OT.$.uuid());
    }

    if (!container) {
        container = OT.$.createElement('div', {id: domId});
        container.style.backgroundColor = "#000000";
        document.body.appendChild(container);
    }
    else {
        OT.$.emptyElement(container);
    }

    if(!(insertMode == null || insertMode == 'replace')) {
      var placeholder = document.createElement('div');
      placeholder.id = ('OT_' + OT.$.uuid());
      if(insertMode == 'append') {
        container.appendChild(placeholder);
        container = placeholder;
      } else if(insertMode == 'before') {
        container.parentNode.insertBefore(placeholder, container);
        container = placeholder;
      } else if(insertMode == 'after') {
        container.parentNode.insertBefore(placeholder, container.nextSibling);
        container = placeholder;
      }
    }

    return container;
};



OT.WidgetView = function(targetElement, properties) {
    var container = getOrCreateContainer(targetElement, properties && properties.insertMode),
        videoContainer = document.createElement('div'),
        oldContainerStyles = {},
        dimensionsObserver,
        videoElement,
        videoObserver,
        posterContainer,
        loadingContainer,
        loading = true;

    if (properties) {
        width = properties.width;
        height = properties.height;

        if (width) {
            if (typeof(width) == "number") {
                width = width + "px";
            }
        }

        if (height) {
            if (typeof(height) == "number") {
                height = height + "px";
            }
        }

        container.style.width = width ? width : "264px";
        container.style.height = height ? height : "198px";
        container.style.overflow = "hidden";

        if (properties.mirror === undefined || properties.mirror) OT.$.addClass(container, 'OT_mirrored');
    }

    if (properties.classNames) OT.$.addClass(container, properties.classNames);
    OT.$.addClass(container, 'OT_loading');


    OT.$.addClass(videoContainer, 'OT_video-container');
    videoContainer.style.width = container.style.width;
    videoContainer.style.height = container.style.height;
    container.appendChild(videoContainer);
    fixAspectRatio(videoContainer, container.offsetWidth, container.offsetHeight);

    loadingContainer = document.createElement("div");
    OT.$.addClass(loadingContainer, "OT_video-loading");
    videoContainer.appendChild(loadingContainer);

    posterContainer = document.createElement("div");
    OT.$.addClass(posterContainer, "OT_video-poster");
    videoContainer.appendChild(posterContainer);

    oldContainerStyles.width = container.offsetWidth;
    oldContainerStyles.height = container.offsetHeight;

    
    dimensionsObserver = OT.$.observeStyleChanges(container, ['width', 'height'], function(changeSet) {
        var width = changeSet.width ? changeSet.width[1] : container.offsetWidth,
            height = changeSet.height ? changeSet.height[1] : container.offsetHeight;

        fixAspectRatio(videoContainer, width, height, videoElement ? videoElement.aspectRatio : null);
    });


    
    videoObserver = OT.$.observeNodeOrChildNodeRemoval(container, function(removedNodes) {
        if (!videoElement) return;

        
        
        var videoRemoved = removedNodes.some(function(node) { return node === videoContainer || node.nodeName === 'VIDEO'; });

        if (videoRemoved) {
            videoElement.destroy();
            videoElement = null;
        }

        if (videoContainer) {
            OT.$.removeElement(videoContainer);
            videoContainer = null;
        }

        if (dimensionsObserver) {
            dimensionsObserver.disconnect();
            dimensionsObserver = null;
        }

        if (videoObserver) {
            videoObserver.disconnect();
            videoObserver = null;
        }
    });

    this.destroy = function() {
        if (dimensionsObserver) {
            dimensionsObserver.disconnect();
            dimensionsObserver = null;
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

    Object.defineProperties(this, {

        showPoster: {
            get: function() {
                return !OT.$.isDisplayNone(posterContainer);
            },
            set: function(shown) {
                if(shown) {
                    OT.$.show(posterContainer);
                }
                else {
                    OT.$.hide(posterContainer);
                }
            }
        },

        poster: {
            get: function() {
                return OT.$.css(posterContainer, "backgroundImage");
            },
            set: function(src) {
                OT.$.css(posterContainer, "backgroundImage", "url(" + src + ")");
            }
        },

        loading: {
            get: function() { return loading; },
            set: function(l) {
                loading = l;

                if (loading) {
                    OT.$.addClass(container, 'OT_loading');
                }
                else {
                    OT.$.removeClass(container, 'OT_loading');
                }
            }
        },

        video: {
            get: function() { return videoElement; },
            set: function(video) {
                
                
                if (videoElement) videoElement.destroy();

                video.appendTo(videoContainer);
                videoElement = video;

                videoElement.on({
                    orientationChanged: function(){
                        fixAspectRatio(videoContainer, container.offsetWidth, container.offsetHeight, videoElement.aspectRatio, videoElement.isRotated);
                    }
                });

                fixAspectRatio(videoContainer, container.offsetWidth, container.offsetHeight, videoElement ? videoElement.aspectRatio : null, videoElement ? videoElement.isRotated : null);
            }
        },

        domElement: {
            get: function() { return container; }
        },

        domId: {
          get: function() { return container.getAttribute('id'); }
        }
    });

    this.addError = function(errorMsg) {
        container.innerHTML = "<p>" + errorMsg + "<p>";
        OT.$.addClass(container, "OT_subscriber_error");
    };
};

})(window);

(function(window) {


var getUserMedia = (function() {
    if (navigator.getUserMedia) {
        return navigator.getUserMedia.bind(navigator);
    } else if (navigator.mozGetUserMedia) {
        return navigator.mozGetUserMedia.bind(navigator);
    } else if (navigator.webkitGetUserMedia) {
        return navigator.webkitGetUserMedia.bind(navigator);
    }
})();


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
}
else if (navigator.mozGetUserMedia) {
    
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




var mozToW3CErrors = {
    PERMISSION_DENIED: 'PERMISSION_DENIED',
    NOT_SUPPORTED_ERROR: "A constraint specified is not supported by the browser.",
    MANDATORY_UNSATISFIED_ERROR: 'CONSTRAINT_NOT_SATISFIED'
};


var chromeToW3CErrors = {
    1: 'PERMISSION_DENIED'
};


var gumNamesToMessages = {
    PERMISSION_DENIED: "User denied permission for scripts from this origin to access the media device.",
    CONSTRAINT_NOT_SATISFIED: "One of the mandatory constraints could not be satisfied."
};


var mapVendorErrorName = function mapVendorErrorName (vendorErrorName, vendorErrors) {
    var errorName = vendorErrors[vendorErrorName],
        errorMessage = gumNamesToMessages[errorName];

    if (!errorMessage) {
        
        
        errorMessage = eventName;
        errorName = vendorErrorName;
    }

    return {
        name: errorName,
        message: errorMessage
    };
};





var parseErrorEvent = function parseErrorObject (event) {
    var error;

    if (OT.$.isObject(event) && event.name) {
        error = {
            name: event.name,
            message: event.message,
            constraintName: event.constraintName
        };
    }
    else if (OT.$.isObject(event)) {
        error = mapVendorErrorName(event.code, chromeToW3CErrors);

        
        
        if (event.message) error.message = event.message;
        if (event.constraintName) error.constraintName = event.constraintName;
    }
    else if (event && mozToW3CErrors.hasOwnProperty(event)) {
        error = mapVendorErrorName(event, mozToW3CErrors);
    }
    else {
        error = {
            message: "Unknown Error while getting user media"
        };
    }


    return error;
};





var areInvalidConstraints = function(constraints) {
    if (!constraints || !OT.$.isObject(constraints)) return true;

    for (var key in constraints) {
        if (constraints[key]) return false;
    }

    return true;
};
















OT.$.supportsWebRTC = function() {
    var _supportsWebRTC = false;

    if (navigator.webkitGetUserMedia) {
        _supportsWebRTC = typeof(webkitRTCPeerConnection) === 'function' && !!webkitRTCPeerConnection.prototype.addStream;
    }
    else if (navigator.mozGetUserMedia) {
        var firefoxVersion = window.navigator.userAgent.toLowerCase().match(/Firefox\/([0-9\.]+)/i);
        _supportsWebRTC = typeof(mozRTCPeerConnection) === 'function' && (firefoxVersion !== null && parseFloat(firefoxVersion[1], 10) > 20.0);
        if (_supportsWebRTC) {
            try {
                new mozRTCPeerConnection();
                _supportsWebRTC = true;
            } catch (err) {
                _supportsWebRTC = false;
            }
        }
    }

    OT.$.supportsWebRTC = function() {
        return _supportsWebRTC;
    };

    return _supportsWebRTC;
};









OT.$.supportedCryptoScheme = function() {
    if (!OT.$.supportsWebRTC()) return 'NONE';

    var chromeVersion = window.navigator.userAgent.toLowerCase().match(/chrome\/([0-9\.]+)/i);
    return chromeVersion && parseFloat(chromeVersion[1], 10) < 25 ? 'SDES_SRTP' : 'DTLS_SRTP';
};







OT.$.supportsBundle = function() {
    return OT.$.supportsWebRTC() && OT.$.browser() === 'Chrome';
};








OT.$.supportsRtcpMux = function() {
    return OT.$.supportsWebRTC() && OT.$.browser() === 'Chrome';
};



























OT.$.getUserMedia = function(constraints, success, failure, accessDialogOpened, accessDialogClosed, accessDenied) {
    
    
    if (areInvalidConstraints(constraints)) {
        OT.error("Couldn't get UserMedia: All constraints were false");
        
        failure.call(null, {
            name: 'NO_VALID_CONSTRAINTS',
            message: "Video and Audio was disabled, you need to enabled at least one"
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

            if (error.name === 'PERMISSION_DENIED' || error.name === 'PermissionDeniedError') {
                accessDenied.call(null, error);
            }
            else {
                failure.call(null, error);
            }
        };

    try {
        getUserMedia(constraints, onStream, onError);
    } catch (e) {
        OT.error("Couldn't get UserMedia: " + e.toString());
        onError();
        return;
    }

    
    
    
    
    
    
    
    
    
    
    
    if (location.protocol.indexOf('https') === -1) {
        
        
        triggerOpenedTimer = setTimeout(triggerOpened, 100);
    }
    else {
        
        triggerOpenedTimer = setTimeout(triggerOpened, 500);
    }
};

OT.$.createPeerConnection = function (config, options) {
  var NativeRTCPeerConnection = (window.webkitRTCPeerConnection || window.mozRTCPeerConnection);
  return new NativeRTCPeerConnection(config, options);
};


})(window);
(function(window) {

OT.VideoOrientation = {
    ROTATED_NORMAL: "OTVideoOrientationRotatedNormal",
    ROTATED_LEFT: "OTVideoOrientationRotatedLeft",
    ROTATED_RIGHT: "OTVideoOrientationRotatedRight",
    ROTATED_UPSIDE_DOWN: "OTVideoOrientationRotatedUpsideDown"
};




























OT.VideoElement = function(options) {
    var _stream,
        _domElement,
        _parentElement,
        _streamBound = false,
        _videoElementMovedWarning = false,
        _options = OT.$.defaults(options || {}, {
            fallbackText: 'Sorry, Web RTC is not available in your browser'
        });


    OT.$.eventing(this);

    
    var _onVideoError = function(event) {
            var reason = "There was an unexpected problem with the Video Stream: " + videoElementErrorCodeToStr(event.target.error.code);
            this.trigger('error', null, reason, this);
        }.bind(this),

        _onStreamBound = function() {
            _streamBound = true;
            _domElement.addEventListener('error', _onVideoError, false);
            this.trigger('streamBound', this);
        }.bind(this),

        _onStreamBoundError = function(reason) {
            this.trigger('loadError', OT.ExceptionCodes.P2P_CONNECTION_FAILED, reason, this);
        }.bind(this),

        
        
        
        _playVideoOnPause = function() {
            if(!_videoElementMovedWarning) {
                OT.warn("Video element paused, auto-resuming. If you intended to do this, use publishVideo(false) or subscribeToVideo(false) instead.");
                _videoElementMovedWarning = true;
            }
            _domElement.play();
        };


    _domElement = createVideoElement(_options.fallbackText, _options.attributes);

    _domElement.addEventListener('pause', _playVideoOnPause);

    
    Object.defineProperties(this, {
        stream: {get: function() {return _stream; }},
        domElement: {get: function() {return _domElement; }},
        parentElement: {get: function() {return _parentElement; }},
        isBoundToStream: {get: function() { return _streamBound; }},
        poster: {
            get: function() { return _domElement.getAttribute('poster'); },
            set: function(src) { _domElement.setAttribute('poster', src); }
        }
    });


    

    
    this.appendTo = function(parentDomElement) {
        _parentElement = parentDomElement;
        _parentElement.appendChild(_domElement);

        return this;
    };

    
    this.bindToStream = function(webRtcStream) {
        _streamBound = false;
        _stream = webRtcStream;

        bindStreamToVideoElement(_domElement, _stream, _onStreamBound, _onStreamBoundError);

        return this;
    };

    
    this.unbindStream = function() {
        if (!_stream) return this;

        if (_domElement) {
            if (!navigator.mozGetUserMedia) {
                
                
                window.URL.revokeObjectURL(_domElement.src);
            }
            else {
                _domElement.mozSrcObject = null;
            }
        }

        _stream = null;

        return this;
    };

    this.setAudioVolume = function(value) {
        if (_domElement) _domElement.volume = OT.$.roundFloat(value / 100, 2);
    };

    this.getAudioVolume = function() {
        
        if (_domElement) return parseInt(_domElement.volume * 100, 10);
        return 50;
    };

    this.whenTimeIncrements = function(callback, context) {
        if(_domElement) {
            var lastTime, handler = function() {
                if(!lastTime || lastTime >= _domElement.currentTime) {
                    lastTime = _domElement.currentTime;
                } else {
                    _domElement.removeEventListener('timeupdate', handler, false);
                    callback.call(context, this);
                }
            }.bind(this);
            _domElement.addEventListener('timeupdate', handler, false);
        }
    };

    this.destroy = function() {
        
        this.off();

        this.unbindStream();

        if (_domElement) {
            
            
            _domElement.removeEventListener('pause', _playVideoOnPause);

            OT.$.removeElement(_domElement);
            _domElement = null;
        }

        _parentElement = null;

        return undefined;
    };
};



if (OT.$.canDefineProperty) {
    
    Object.defineProperties(OT.VideoElement.prototype, {
        imgData: {
            get: function() {
                var canvas = OT.$.createElement('canvas', {
                        width: this.domElement.videoWidth,
                        height: this.domElement.videoHeight,
                        style: {
                            display: 'none'
                        }
                    });

                document.body.appendChild(canvas);
                try {
                    canvas.getContext('2d').drawImage(this.domElement, 0, 0, canvas.width, canvas.height);
                } catch(err) {
                    OT.warn("Cannot get image data yet");
                    return null;
                }
                var imgData = canvas.toDataURL('image/png');

                OT.$.removeElement(canvas);

                return imgData.replace("data:image/png;base64,", "").trim();
            }
        },

        videoWidth: {
            get: function() {
                if(this._orientation && this._orientation.width) {
                    return this._orientation.width;
                }
                return this.domElement['video' + (this.isRotated ? 'Height' : 'Width')];
            }
        },

        videoHeight: {
            get: function() {
                if(this._orientation && this._orientation.height) {
                    return this._orientation.height;
                }
                return this.domElement['video' + (this.isRotated ? 'Width' : 'Height')];
            }
        },

        aspectRatio: {
            get: function() {
                return (this.videoWidth + 0.0) / this.videoHeight;
            }
        },

        isRotated: {
            get: function() {
                return this._orientation && (this._orientation.videoOrientation == 'OTVideoOrientationRotatedLeft' || this._orientation.videoOrientation == 'OTVideoOrientationRotatedRight');
            }
        }
    });
}


var VideoOrientationTransforms = {
    OTVideoOrientationRotatedNormal: "rotate(0deg)",
    OTVideoOrientationRotatedLeft: "rotate(90deg)",
    OTVideoOrientationRotatedRight: "rotate(-90deg)",
    OTVideoOrientationRotatedUpsideDown: "rotate(180deg)"
};


if (OT.$.canDefineProperty) {
    Object.defineProperty(OT.VideoElement.prototype,'orientation', {
        get: function() {
            return this._orientation;
        },
        set: function(orientation) {
            this._orientation = orientation;

            var transform = VideoOrientationTransforms[orientation.videoOrientation] || VideoOrientationTransforms.ROTATED_NORMAL;

            switch(OT.$.browser()) {
                case 'Chrome':
                case 'Safari':
                    this.domElement.style.webkitTransform = transform;
                    break;

                case 'IE':
                    this.domElement.style.msTransform = transform;
                    break;

                default:
                    
                    this.domElement.style.transform = transform;
            }

            this.trigger('orientationChanged');
        }
    });
}





function createVideoElement(fallbackText, attributes) {
    var videoElement = document.createElement('video');
    videoElement.setAttribute('autoplay', '');
    videoElement.innerHTML = fallbackText;

    if (attributes) {
        if (attributes.muted === true) {
            delete attributes.muted;
            videoElement.muted = 'true';
        }

        for (var key in attributes) {
            videoElement.setAttribute(key, attributes[key]);
        }
    }

    return videoElement;
}



var _videoErrorCodes = {};

if (window.MediaError) {
    _videoErrorCodes[window.MediaError.MEDIA_ERR_ABORTED] = "The fetching process for the media resource was aborted by the user agent at the user's request.";
    _videoErrorCodes[window.MediaError.MEDIA_ERR_NETWORK] = "A network error of some description caused the user agent to stop fetching the media resource, after the resource was established to be usable.";
    _videoErrorCodes[window.MediaError.MEDIA_ERR_DECODE] = "An error of some description occurred while decoding the media resource, after the resource was established to be usable.";
    _videoErrorCodes[window.MediaError.MEDIA_ERR_SRC_NOT_SUPPORTED] = "The media resource indicated by the src attribute was not suitable. ";
}

function videoElementErrorCodeToStr(errorCode) {
    return _videoErrorCodes[parseInt(errorCode, 10)] || "An unknown error occurred.";
}


function bindStreamToVideoElement(videoElement, webRTCStream, onStreamBound, onStreamBoundError) {
    
    if (navigator.mozGetUserMedia || (webRTCStream.getVideoTracks().length > 0 && webRTCStream.getVideoTracks()[0].enabled)) {

        var cleanup = function cleanup () {
                clearTimeout(timeout);
                videoElement.removeEventListener('loadedmetadata', onLoad, false);
                videoElement.removeEventListener('error', onError, false);
            },

            onLoad = function onLoad (event) {
                cleanup();
                onStreamBound();
            },

            onError = function onError (event) {
                cleanup();
                onStreamBoundError("There was an unexpected problem with the Video Stream: " + videoElementErrorCodeToStr(event.target.error.code));
            },

            onStoppedLoading = function onStoppedLoading () {
                
                
                cleanup();
                onStreamBoundError("Stream ended while trying to bind it to a video element.");
            },

            
            timeout = setTimeout(function() {
                if (videoElement.currentTime === 0) {
                    onStreamBoundError("The video stream failed to connect. Please notify the site owner if this continues to happen.");
                } else {
                    
                    OT.warn("Never got the loadedmetadata event but currentTime > 0");
                    onStreamBound();
                }
            }.bind(this), 30000);


        videoElement.addEventListener('loadedmetadata', onLoad, false);
        videoElement.addEventListener('error', onError, false);
        webRTCStream.onended = onStoppedLoading;
    } else {
        onStreamBound();
    }

    
    if (videoElement.srcObject !== void 0) {
        videoElement.srcObject = webRTCStream;
    }
    else if (videoElement.mozSrcObject !== void 0) {
        videoElement.mozSrcObject = webRTCStream;
    }
    else {
        videoElement.src = window.URL.createObjectURL(webRTCStream);
    }

    videoElement.play();
}


})(window);
(function(window) {


var logQueue = [],
    queueRunning = false;


OT.Analytics = function() {

    var endPoint = OT.properties.loggingURL + "/logging/ClientEvent",
        endPointQos = OT.properties.loggingURL + "/logging/ClientQos",

        reportedErrors = {},

        
        camelCasedKeys = {
            payloadType: 'payload_type',
            partnerId: 'partner_id',
            streamId: 'stream_id',
            sessionId: 'session_id',
            connectionId: 'connection_id',
            widgetType: 'widget_type',
            widgetId: 'widget_id',
            avgAudioBitrate: 'avg_audio_bitrate',
            avgVideoBitrate: 'avg_video_bitrate',
            localCandidateType: 'local_candidate_type',
            remoteCandidateType: 'remote_candidate_type',
            transportType: 'transport_type'
        },

        send = function(data, isQos, onSuccess, onError) {
            OT.$.post(isQos ? endPointQos : endPoint, {
                success: onSuccess,
                error: onError,
                data: data,
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded'
                }
            });
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
                    send(curr.data, curr.isQos, function() {
                        curr.onComplete();
                        setTimeout(processNextItem, 50);
                    }, function() {
                        OT.debug("Failed to send ClientEvent, moving on to the next item.");

                        
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

            if (OT.Config.get('exceptionLogging', 'enabled', partnerId) !== true) {
                return;
            }

            if (shouldThrottleError(code, type, partnerId)) {
                
                return;
            }

            var errKey = [partnerId, type, code].join('_'),

                payload = this.escapePayload(OT.$.extend(details || {}, {
                    message: payload,
                    userAgent: navigator.userAgent
                }));


            reportedErrors[errKey] = typeof(reportedErrors[errKey]) !== 'undefined' ?
                                            reportedErrors[errKey] + 1 :
                                            1;

            return this.logEvent(OT.$.extend(options, {
                action: type + '.' + code,
                payloadType: payload[0],
                payload: payload[1]
            }));
        };

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        this.logEvent = function(options) {
            var partnerId = options.partnerId;

            if (!options) options = {};

            
            var data = OT.$.extend({
                    "variation" : "",
                    'guid' : this.getClientGuid(),
                    'widget_id' : "",
                    'session_id': '',
                    'connection_id': '',
                    'stream_id' : "",
                    'partner_id' : partnerId,
                    'source' : window.location.href,
                    'section' : "",
                    'build' : ""
                }, options),

                onComplete = function(){
                
                };

            
            
            for (var key in camelCasedKeys) {
                if (camelCasedKeys.hasOwnProperty(key) && data[key]) {
                    data[camelCasedKeys[key]] = data[key];
                    delete data[key];
                }
            }

            post(data, onComplete, false);
        };

        
        
        this.logQOS = function(options) {
            var partnerId = options.partnerId;

            if (!options) options = {};

            
            var data = OT.$.extend({
                    'guid' : this.getClientGuid(),
                    'widget_id' : "",
                    'session_id': '',
                    'connection_id': '',
                    'stream_id' : "",
                    'partner_id' : partnerId,
                    'source' : window.location.href,
                    'build' : "",
                    'duration' : 0 
                }, options),

                onComplete = function(){
                    
                };

            
            
            for (var key in camelCasedKeys) {
                if (camelCasedKeys.hasOwnProperty(key) && data[key]) {
                    data[camelCasedKeys[key]] = data[key];
                    delete data[key];
                }
            }

            post(data, onComplete, true);
        };

        
        
        
        
        this.escapePayload = function(payload) {
            var escapedPayload = [],
                escapedPayloadDesc = [];

            for (var key in payload) {
                if (payload.hasOwnProperty(key) && payload[key] !== null && payload[key] !== undefined) {
                    escapedPayload.push( payload[key] ? payload[key].toString().replace('|', '\\|') : '' );
                    escapedPayloadDesc.push( key.toString().replace('|', '\\|') );
                }
            }

            return [
                escapedPayloadDesc.join('|'),
                escapedPayload.join('|')
            ];
        };
        
        this.getClientGuid = function() {
            var  guid = OT.$.getCookie("opentok_client_id");
            if (!guid) {
                guid = OT.$.uuid();
                OT.$.setCookie("opentok_client_id", guid);
            }
            return guid;
        };
}

})(window);
(function(window) {




if (location.protocol === 'file:') {
  alert("You cannot test a page using WebRTC through the file system due to browser permissions. You must run it over a web server.");
}

if (!window.OT) window.OT = {};

if (!window.URL && window.webkitURL) {
    window.URL = window.webkitURL;
}

var _publisherCount = 0,

    
    _intervalId,
    _lastHash = document.location.hash,

    
    _deviceManager;































OT.initSession = function(sessionId) {
    var session = OT.sessions.get(sessionId);

    if (!session) {
        session = new OT.Session(sessionId);
        OT.sessions.add(session);
    }

    return session;
};















































































OT.initPublisher = function(apiKey, targetElement, properties, completionHandler) {
    OT.debug("TB.initPublisher("+targetElement+")");

    var publisher = new OT.Publisher();;
    OT.publishers.add(publisher);

    if (completionHandler && OT.$.isFunction(completionHandler)) {
        var removeHandlersAndCallComplete = function removeHandlersAndCallComplete (error) {
            publisher.off('initSuccess', removeHandlersAndCallComplete);
            publisher.off('publishComplete', removeHandlersAndCallComplete);

            completionHandler.apply(null, arguments);
        };

        publisher.once('initSuccess', removeHandlersAndCallComplete);
        publisher.once('publishComplete', removeHandlersAndCallComplete);
    }

    publisher.publish(targetElement, properties);

    return publisher;
};










OT.checkSystemRequirements = function() {
    OT.debug("TB.checkSystemRequirements()");

    var systemRequirementsMet = OT.$.supportsWebSockets() && OT.$.supportsWebRTC() ? this.HAS_REQUIREMENTS : this.NOT_HAS_REQUIREMENTS;

    OT.checkSystemRequirements = function() {
      OT.debug("TB.checkSystemRequirements()");
      return systemRequirementsMet;
    };

    return systemRequirementsMet;
};













OT.upgradeSystemRequirements = function(){
    
    OT.onLoad( function() {
        var id = '_upgradeFlash';

         
         document.body.appendChild((function(){
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
                 d.style.backgroundColor = "rgba(0,0,0,0.2)";
             } catch (err) {
                 
                 
                 d.style.backgroundColor = "transparent";
                 d.setAttribute("allowTransparency", "true");
             }
             d.setAttribute("frameBorder", "0");
             d.frameBorder = "0";
             d.scrolling = "no";
             d.setAttribute("scrolling", "no");
             d.src = OT.properties.assetURL + "/html/upgradeFlash.html#"+encodeURIComponent(document.location.href);
             return d;
         })());

         
         
         
         if (_intervalId) clearInterval(_intervalId);
         _intervalId = setInterval(function(){
             var hash = document.location.hash,
                 re = /^#?\d+&/;
             if (hash !== _lastHash && re.test(hash)) {
                 _lastHash = hash;
                 if( hash.replace(re, '') === 'close_window'){
                     document.body.removeChild(document.getElementById(id));
                     document.location.hash = '';
                 }
             }
         }, 100);
    });
};


OT.reportIssue = function(){
    OT.warn("ToDo: haven't yet implemented TB.reportIssue");
};

OT.components = {};
OT.sessions = {};


OT.rtc = {};


OT.APIKEY = (function(){
    
    var script_src = (function(){
        var s = document.getElementsByTagName('script');
        s = s[s.length - 1];
        s = s.getAttribute('src') || s.src;
        return s;
    })();

    var m = script_src.match(/[\?\&]apikey=([^&]+)/i);
    return m ? m[1] : '';
})();

OT.HAS_REQUIREMENTS = 1;
OT.NOT_HAS_REQUIREMENTS = 0;



























































if (!window.OT) window.OT = OT;
if (!window.TB) window.TB = OT;

})(window);
(function(global) {

OT.Collection = function(idField) {
  var _models = [],
      _byId = {},
      _idField = idField || 'id';

  OT.$.eventing(this, true);

  var onModelUpdate = function onModelUpdate (event) {
        this.trigger('update', event);
        this.trigger('update:'+event.target.id, event);
      }.bind(this),

      onModelIdUpdate = function onModelIdUpdate (oldId, newId) {
        if (_byId[oldId] === void 0) return;

        
        
        var index = _byId[oldId];
        delete _byId[oldId];
        _byId[newId] = index;
      }.bind(this),

      onModelDestroy = function onModelDestroyed (event) {
        this.remove(event.target, event.reason);
      }.bind(this);


  this.reset = function() {
    
    _models.forEach(function(model) {
      model.off('updated', onModelUpdate, this);
      model.off('destroyed', onModelDestroy, this)
      model.off('idUpdated', onModelIdUpdate, this);
    }, this);

    _models = [];
    _byId = {};
  };

  this.destroy = function() {
    _models.forEach(function(model) {
      model.destroy(void 0, true);
    });

    this.reset();
    this.off();
  };

  this.get = function(id) { return id && _byId[id] !== void 0 ? _models[_byId[id]] : void 0; };
  this.has = function(id) { return id && _byId[id] !== void 0; };

  this.toString = function() { return _models.toString(); };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  this.where = function(attrsOrFilterFn, context) {
    if (OT.$.isFunction(attrsOrFilterFn)) return _models.filter(attrsOrFilterFn, context);

    return _models.filter(function(model) {
      for (var key in attrsOrFilterFn) {
        if (model[key] !== attrsOrFilterFn[key]) return false;
      }

      return true;
    });
  };

  
  
  this.find = function(attrsOrFilterFn, context) {
    var filterFn;

    if (OT.$.isFunction(attrsOrFilterFn)) {
      filterFn = attrsOrFilterFn;
    }
    else {
      filterFn = function(model) {
        for (var key in attrsOrFilterFn) {
          if (model[key] !== attrsOrFilterFn[key]) return false;
        }

        return true;
      };
    }

    filterFn = filterFn.bind(context);

    for (var i=0; i<_models.length; ++i) {
      if (filterFn(_models[i]) === true) return _models[i];
    }

    return null;
  };

  this.add = function(model) {
    var id = model[_idField];

    if (this.has(id)) {
      OT.warn("Model " + id + ' is already in the collection', _models);
      return this;
    }

    _byId[id] = _models.push(model) - 1;

    model.on('updated', onModelUpdate, this);
    model.on('destroyed', onModelDestroy, this)
    model.on('idUpdated', onModelIdUpdate, this);

    this.trigger('add', model);
    this.trigger('add:'+id, model);

    return this;
  };

  this.remove = function(model, reason) {
    var id = model[_idField];

    _models.splice(_byId[id], 1);

    
    for (var i=_byId[id]; i<_models.length; ++i) {
      _byId[_models[i][_idField]] = i
    }

    delete _byId[id];

    model.off('updated', onModelUpdate, this);
    model.off('destroyed', onModelDestroy, this)
    model.off('idUpdated', onModelIdUpdate, this);

    this.trigger('remove', model, reason);
    this.trigger('remove:'+id, model, reason);

    return this;
  };

  OT.$.defineGetters(this, {
    length: function() { return _models.length; }
  });
};

}(this));
(function(window) {


























OT.Event = OT.$.eventing.Event();































OT.Event.names = {
    
    ACTIVE: "active",
    INACTIVE: "inactive",
    UNKNOWN: "unknown",

    
    PER_SESSION: "perSession",
    PER_STREAM: "perStream",

    
    EXCEPTION: "exception",
    ISSUE_REPORTED: "issueReported",

    
    SESSION_CONNECTED: "sessionConnected",
    SESSION_DISCONNECTED: "sessionDisconnected",
    STREAM_CREATED: "streamCreated",
    STREAM_DESTROYED: "streamDestroyed",
    CONNECTION_CREATED: "connectionCreated",
    CONNECTION_DESTROYED: "connectionDestroyed",
    SIGNAL: "signal",
    STREAM_PROPERTY_CHANGED: "streamPropertyChanged",
    MICROPHONE_LEVEL_CHANGED: "microphoneLevelChanged",


    
    RESIZE: "resize",
    SETTINGS_BUTTON_CLICK: "settingsButtonClick",
    DEVICE_INACTIVE: "deviceInactive",
    INVALID_DEVICE_NAME: "invalidDeviceName",
    ACCESS_ALLOWED: "accessAllowed",
    ACCESS_DENIED: "accessDenied",
    ACCESS_DIALOG_OPENED: 'accessDialogOpened',
    ACCESS_DIALOG_CLOSED: 'accessDialogClosed',
    ECHO_CANCELLATION_MODE_CHANGED: "echoCancellationModeChanged",
    PUBLISHER_DESTROYED: 'destroyed',

    
    SUBSCRIBER_DESTROYED: 'destroyed',

    
    DEVICES_DETECTED: "devicesDetected",

    
    DEVICES_SELECTED: "devicesSelected",
    CLOSE_BUTTON_CLICK: "closeButtonClick",

    MICLEVEL : 'microphoneActivityLevel',
    MICGAINCHANGED : 'microphoneGainChanged',

    
    ENV_LOADED: 'envLoaded'
};

OT.ValueEvent = function (type,value){
    OT.Event.call(this, type);
    this.value = value;

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
  UNABLE_TO_PUBLISH: 1500,
  UNABLE_TO_SIGNAL: 1510,
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













































































OT.ConnectionEvent = function (type, connections, reason) {
    OT.Event.call(this, type);

    this.connections = connections;
    this.reason = reason;
};











































































































OT.StreamEvent = function (type, streams, reason, cancelable) {
    OT.Event.call(this, type, cancelable);

    this.streams = streams;
    this.reason = reason;
};










































OT.SessionConnectEvent = function (type, connections, streams, archives) {
    OT.Event.call(this, type);

    this.connections = connections;
    this.streams = streams;
    this.archives = archives;
    this.groups = []; 
};











































OT.SessionDisconnectEvent = function (type, reason, cancelable) {
    OT.Event.call(this, type, cancelable);

    this.reason = reason;
};

















OT.VolumeEvent = function (type, streamId, volume) {
    OT.Event.call(this, type);

    this.streamId = streamId;
    this.volume = volume;
};


OT.DeviceEvent = function (type, camera, microphone) {
    OT.Event.call(this, type);

    this.camera = camera;
    this.microphone = microphone;
};

OT.DeviceStatusEvent = function (type, cameras, microphones, selectedCamera, selectedMicrophone) {
    OT.Event.call(this, type);

    this.cameras = cameras;
    this.microphones = microphones;
    this.selectedCamera = selectedCamera;
    this.selectedMicrophone = selectedMicrophone;
};

OT.ResizeEvent = function (type, widthFrom, widthTo, heightFrom, heightTo) {
    OT.Event.call(this, type);

    this.widthFrom = widthFrom;
    this.widthTo = widthTo;
    this.heightFrom = heightFrom;
    this.heightTo = heightTo;
};



























OT.StreamPropertyChangedEvent = function (type, stream, changedProperty, oldValue, newValue) {
    OT.Event.call(this, type);

    this.type = type;
    this.stream = stream;
    this.changedProperty = changedProperty;
    this.oldValue = oldValue;
    this.newValue = newValue;
};

OT.ArchiveEvent = function (type, archives) {
    OT.Event.call(this, type);

    this.archives = archives;
};

OT.ArchiveStreamEvent = function (type, archive, streams) {
    OT.Event.call(this, type);

    this.archive = archive;
    this.streams = streams;
};

OT.StateChangedEvent = function (type, changedValues) {
    OT.Event.call(this, type);

    this.changedValues = changedValues;
};

OT.ChangeFailedEvent = function (type, reasonCode, reason, failedValues) {
    OT.Event.call(this, type);

    this.reasonCode = reasonCode;
    this.reason = reason;
    this.failedValues = failedValues;
};














OT.SignalEvent = function(type, data, from) {
    OT.Event.call(this, type ? "signal:" + type : OT.Event.names.SIGNAL, false);

    this.data = data;
    this.from = from;
};


OT.StreamUpdatedEvent = function (stream, key, oldValue, newValue) {
    OT.Event.call(this, 'updated');

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


})(window);



(function(global) {
  'use strict';

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

  




  function indexGB18030CodePointFor(pointer) {
    if ((pointer > 39419 && pointer < 189000) || (pointer > 1237575)) {
      return null;
    }
    var  offset = 0,
         code_point_offset = 0,
         index = indexes['gb18030'];
    var i;
    for (i = 0; i < index.length; ++i) {
      var entry = index[i];
      if (entry[0] <= pointer) {
        offset = entry[0];
        code_point_offset = entry[1];
      } else {
        break;
      }
    }
    return code_point_offset + pointer - offset;
  }

  




  function indexGB18030PointerFor(code_point) {
    var  offset = 0,
         pointer_offset = 0,
         index = indexes['gb18030'];
    var i;
    for (i = 0; i < index.length; ++i) {
      var entry = index[i];
      if (entry[1] <= code_point) {
        offset = entry[1];
        pointer_offset = entry[0];
      } else {
        break;
      }
    }
    return pointer_offset + code_point - offset;
  }

  
  
  

  

  



  function UTF8Decoder(options) {
    var fatal = options.fatal;
    var  utf8_code_point = 0,
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

  
  
  

  




  function SingleByteDecoder(index, options) {
    var fatal = options.fatal;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte) {
        return EOF_code_point;
      }
      byte_pointer.offset(1);
      if (inRange(bite, 0x00, 0x7F)) {
        return bite;
      }
      var code_point = index[bite - 0x80];
      if (code_point === null) {
        return decoderError(fatal);
      }
      return code_point;
    };
  }

  




  function SingleByteEncoder(index, options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      var pointer = indexPointerFor(code_point, index);
      if (pointer === null) {
        encoderError(code_point);
      }
      return output_byte_stream.emit(pointer + 0x80);
    };
  }

  (function() {
    ['ibm864', 'ibm866', 'iso-8859-2', 'iso-8859-3', 'iso-8859-4',
     'iso-8859-5', 'iso-8859-6', 'iso-8859-7', 'iso-8859-8', 'iso-8859-10',
     'iso-8859-13', 'iso-8859-14', 'iso-8859-15', 'iso-8859-16', 'koi8-r',
     'koi8-u', 'macintosh', 'windows-874', 'windows-1250', 'windows-1251',
     'windows-1252', 'windows-1253', 'windows-1254', 'windows-1255',
     'windows-1256', 'windows-1257', 'windows-1258', 'x-mac-cyrillic'
    ].forEach(function(name) {
      var encoding = name_to_encoding[name];
      var index = indexes[name];
      encoding.getDecoder = function(options) {
        return new SingleByteDecoder(index, options);
      };
      encoding.getEncoder = function(options) {
        return new SingleByteEncoder(index, options);
      };
    });
  }());

  
  
  

  

  




  function GBKDecoder(gb18030, options) {
    var fatal = options.fatal;
    var  gbk_first = 0x00,
         gbk_second = 0x00,
         gbk_third = 0x00;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte && gbk_first === 0x00 &&
          gbk_second === 0x00 && gbk_third === 0x00) {
        return EOF_code_point;
      }
      if (bite === EOF_byte &&
          (gbk_first !== 0x00 || gbk_second !== 0x00 || gbk_third !== 0x00)) {
        gbk_first = 0x00;
        gbk_second = 0x00;
        gbk_third = 0x00;
        decoderError(fatal);
      }
      byte_pointer.offset(1);
      var code_point;
      if (gbk_third !== 0x00) {
        code_point = null;
        if (inRange(bite, 0x30, 0x39)) {
          code_point = indexGB18030CodePointFor(
              (((gbk_first - 0x81) * 10 + (gbk_second - 0x30)) * 126 +
               (gbk_third - 0x81)) * 10 + bite - 0x30);
        }
        gbk_first = 0x00;
        gbk_second = 0x00;
        gbk_third = 0x00;
        if (code_point === null) {
          byte_pointer.offset(-3);
          return decoderError(fatal);
        }
        return code_point;
      }
      if (gbk_second !== 0x00) {
        if (inRange(bite, 0x81, 0xFE)) {
          gbk_third = bite;
          return null;
        }
        byte_pointer.offset(-2);
        gbk_first = 0x00;
        gbk_second = 0x00;
        return decoderError(fatal);
      }
      if (gbk_first !== 0x00) {
        if (inRange(bite, 0x30, 0x39) && gb18030) {
          gbk_second = bite;
          return null;
        }
        var lead = gbk_first;
        var pointer = null;
        gbk_first = 0x00;
        var offset = bite < 0x7F ? 0x40 : 0x41;
        if (inRange(bite, 0x40, 0x7E) || inRange(bite, 0x80, 0xFE)) {
          pointer = (lead - 0x81) * 190 + (bite - offset);
        }
        code_point = pointer === null ? null :
            indexCodePointFor(pointer, indexes['gbk']);
        if (pointer === null) {
          byte_pointer.offset(-1);
        }
        if (code_point === null) {
          return decoderError(fatal);
        }
        return code_point;
      }
      if (inRange(bite, 0x00, 0x7F)) {
        return bite;
      }
      if (bite === 0x80) {
        return 0x20AC;
      }
      if (inRange(bite, 0x81, 0xFE)) {
        gbk_first = bite;
        return null;
      }
      return decoderError(fatal);
    };
  }

  




  function GBKEncoder(gb18030, options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      var pointer = indexPointerFor(code_point, indexes['gbk']);
      if (pointer !== null) {
        var lead = div(pointer, 190) + 0x81;
        var trail = pointer % 190;
        var offset = trail < 0x3F ? 0x40 : 0x41;
        return output_byte_stream.emit(lead, trail + offset);
      }
      if (pointer === null && !gb18030) {
        return encoderError(code_point);
      }
      pointer = indexGB18030PointerFor(code_point);
      var byte1 = div(div(div(pointer, 10), 126), 10);
      pointer = pointer - byte1 * 10 * 126 * 10;
      var byte2 = div(div(pointer, 10), 126);
      pointer = pointer - byte2 * 10 * 126;
      var byte3 = div(pointer, 10);
      var byte4 = pointer - byte3 * 10;
      return output_byte_stream.emit(byte1 + 0x81,
                                     byte2 + 0x30,
                                     byte3 + 0x81,
                                     byte4 + 0x30);
    };
  }

  name_to_encoding['gbk'].getEncoder = function(options) {
    return new GBKEncoder(false, options);
  };
  name_to_encoding['gbk'].getDecoder = function(options) {
    return new GBKDecoder(false, options);
  };

  
  name_to_encoding['gb18030'].getEncoder = function(options) {
    return new GBKEncoder(true, options);
  };
  name_to_encoding['gb18030'].getDecoder = function(options) {
    return new GBKDecoder(true, options);
  };

  

  



  function HZGB2312Decoder(options) {
    var fatal = options.fatal;
    var  hzgb2312 = false,
         hzgb2312_lead = 0x00;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte && hzgb2312_lead === 0x00) {
        return EOF_code_point;
      }
      if (bite === EOF_byte && hzgb2312_lead !== 0x00) {
        hzgb2312_lead = 0x00;
        return decoderError(fatal);
      }
      byte_pointer.offset(1);
      if (hzgb2312_lead === 0x7E) {
        hzgb2312_lead = 0x00;
        if (bite === 0x7B) {
          hzgb2312 = true;
          return null;
        }
        if (bite === 0x7D) {
          hzgb2312 = false;
          return null;
        }
        if (bite === 0x7E) {
          return 0x007E;
        }
        if (bite === 0x0A) {
          return null;
        }
        byte_pointer.offset(-1);
        return decoderError(fatal);
      }
      if (hzgb2312_lead !== 0x00) {
        var lead = hzgb2312_lead;
        hzgb2312_lead = 0x00;
        var code_point = null;
        if (inRange(bite, 0x21, 0x7E)) {
          code_point = indexCodePointFor((lead - 1) * 190 +
                                         (bite + 0x3F), indexes['gbk']);
        }
        if (bite === 0x0A) {
          hzgb2312 = false;
        }
        if (code_point === null) {
          return decoderError(fatal);
        }
        return code_point;
      }
      if (bite === 0x7E) {
        hzgb2312_lead = 0x7E;
        return null;
      }
      if (hzgb2312) {
        if (inRange(bite, 0x20, 0x7F)) {
          hzgb2312_lead = bite;
          return null;
        }
        if (bite === 0x0A) {
          hzgb2312 = false;
        }
        return decoderError(fatal);
      }
      if (inRange(bite, 0x00, 0x7F)) {
        return bite;
      }
      return decoderError(fatal);
    };
  }

  



  function HZGB2312Encoder(options) {
    var fatal = options.fatal;
    var hzgb2312 = false;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x007F) && hzgb2312) {
        code_point_pointer.offset(-1);
        hzgb2312 = false;
        return output_byte_stream.emit(0x7E, 0x7D);
      }
      if (code_point === 0x007E) {
        return output_byte_stream.emit(0x7E, 0x7E);
      }
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      if (!hzgb2312) {
        code_point_pointer.offset(-1);
        hzgb2312 = true;
        return output_byte_stream.emit(0x7E, 0x7B);
      }
      var pointer = indexPointerFor(code_point, indexes['gbk']);
      if (pointer === null) {
        return encoderError(code_point);
      }
      var lead = div(pointer, 190) + 1;
      var trail = pointer % 190 - 0x3F;
      if (!inRange(lead, 0x21, 0x7E) || !inRange(trail, 0x21, 0x7E)) {
        return encoderError(code_point);
      }
      return output_byte_stream.emit(lead, trail);
    };
  }

  name_to_encoding['hz-gb-2312'].getEncoder = function(options) {
    return new HZGB2312Encoder(options);
  };
  name_to_encoding['hz-gb-2312'].getDecoder = function(options) {
    return new HZGB2312Decoder(options);
  };

  
  
  

  

  



  function Big5Decoder(options) {
    var fatal = options.fatal;
    var  big5_lead = 0x00,
         big5_pending = null;

    




    this.decode = function(byte_pointer) {
      
      if (big5_pending !== null) {
        var pending = big5_pending;
        big5_pending = null;
        return pending;
      }
      var bite = byte_pointer.get();
      if (bite === EOF_byte && big5_lead === 0x00) {
        return EOF_code_point;
      }
      if (bite === EOF_byte && big5_lead !== 0x00) {
        big5_lead = 0x00;
        return decoderError(fatal);
      }
      byte_pointer.offset(1);
      if (big5_lead !== 0x00) {
        var lead = big5_lead;
        var pointer = null;
        big5_lead = 0x00;
        var offset = bite < 0x7F ? 0x40 : 0x62;
        if (inRange(bite, 0x40, 0x7E) || inRange(bite, 0xA1, 0xFE)) {
          pointer = (lead - 0x81) * 157 + (bite - offset);
        }
        if (pointer === 1133) {
          big5_pending = 0x0304;
          return 0x00CA;
        }
        if (pointer === 1135) {
          big5_pending = 0x030C;
          return 0x00CA;
        }
        if (pointer === 1164) {
          big5_pending = 0x0304;
          return 0x00EA;
        }
        if (pointer === 1166) {
          big5_pending = 0x030C;
          return 0x00EA;
        }
        var code_point = (pointer === null) ? null :
            indexCodePointFor(pointer, indexes['big5']);
        if (pointer === null) {
          byte_pointer.offset(-1);
        }
        if (code_point === null) {
          return decoderError(fatal);
        }
        return code_point;
      }
      if (inRange(bite, 0x00, 0x7F)) {
        return bite;
      }
      if (inRange(bite, 0x81, 0xFE)) {
        big5_lead = bite;
        return null;
      }
      return decoderError(fatal);
    };
  }

  



  function Big5Encoder(options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      var pointer = indexPointerFor(code_point, indexes['big5']);
      if (pointer === null) {
        return encoderError(code_point);
      }
      var lead = div(pointer, 157) + 0x81;
      
      
      
      var trail = pointer % 157;
      var offset = trail < 0x3F ? 0x40 : 0x62;
      return output_byte_stream.emit(lead, trail + offset);
    };
  }

  name_to_encoding['big5'].getEncoder = function(options) {
    return new Big5Encoder(options);
  };
  name_to_encoding['big5'].getDecoder = function(options) {
    return new Big5Decoder(options);
  };


  
  
  

  

  



  function EUCJPDecoder(options) {
    var fatal = options.fatal;
    var  eucjp_first = 0x00,
         eucjp_second = 0x00;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte) {
        if (eucjp_first === 0x00 && eucjp_second === 0x00) {
          return EOF_code_point;
        }
        eucjp_first = 0x00;
        eucjp_second = 0x00;
        return decoderError(fatal);
      }
      byte_pointer.offset(1);

      var lead, code_point;
      if (eucjp_second !== 0x00) {
        lead = eucjp_second;
        eucjp_second = 0x00;
        code_point = null;
        if (inRange(lead, 0xA1, 0xFE) && inRange(bite, 0xA1, 0xFE)) {
          code_point = indexCodePointFor((lead - 0xA1) * 94 + bite - 0xA1,
                                         indexes['jis0212']);
        }
        if (!inRange(bite, 0xA1, 0xFE)) {
          byte_pointer.offset(-1);
        }
        if (code_point === null) {
          return decoderError(fatal);
        }
        return code_point;
      }
      if (eucjp_first === 0x8E && inRange(bite, 0xA1, 0xDF)) {
        eucjp_first = 0x00;
        return 0xFF61 + bite - 0xA1;
      }
      if (eucjp_first === 0x8F && inRange(bite, 0xA1, 0xFE)) {
        eucjp_first = 0x00;
        eucjp_second = bite;
        return null;
      }
      if (eucjp_first !== 0x00) {
        lead = eucjp_first;
        eucjp_first = 0x00;
        code_point = null;
        if (inRange(lead, 0xA1, 0xFE) && inRange(bite, 0xA1, 0xFE)) {
          code_point = indexCodePointFor((lead - 0xA1) * 94 + bite - 0xA1,
                                         indexes['jis0208']);
        }
        if (!inRange(bite, 0xA1, 0xFE)) {
          byte_pointer.offset(-1);
        }
        if (code_point === null) {
          return decoderError(fatal);
        }
        return code_point;
      }
      if (inRange(bite, 0x00, 0x7F)) {
        return bite;
      }
      if (bite === 0x8E || bite === 0x8F || (inRange(bite, 0xA1, 0xFE))) {
        eucjp_first = bite;
        return null;
      }
      return decoderError(fatal);
    };
  }

  



  function EUCJPEncoder(options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      if (code_point === 0x00A5) {
        return output_byte_stream.emit(0x5C);
      }
      if (code_point === 0x203E) {
        return output_byte_stream.emit(0x7E);
      }
      if (inRange(code_point, 0xFF61, 0xFF9F)) {
        return output_byte_stream.emit(0x8E, code_point - 0xFF61 + 0xA1);
      }

      var pointer = indexPointerFor(code_point, indexes['jis0208']);
      if (pointer === null) {
        return encoderError(code_point);
      }
      var lead = div(pointer, 94) + 0xA1;
      var trail = pointer % 94 + 0xA1;
      return output_byte_stream.emit(lead, trail);
    };
  }

  name_to_encoding['euc-jp'].getEncoder = function(options) {
    return new EUCJPEncoder(options);
  };
  name_to_encoding['euc-jp'].getDecoder = function(options) {
    return new EUCJPDecoder(options);
  };

  

  



  function ISO2022JPDecoder(options) {
    var fatal = options.fatal;
    
    var state = {
      ASCII: 0,
      escape_start: 1,
      escape_middle: 2,
      escape_final: 3,
      lead: 4,
      trail: 5,
      Katakana: 6
    };
    var  iso2022jp_state = state.ASCII,
         iso2022jp_jis0212 = false,
         iso2022jp_lead = 0x00;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite !== EOF_byte) {
        byte_pointer.offset(1);
      }
      switch (iso2022jp_state) {
        default:
        case state.ASCII:
          if (bite === 0x1B) {
            iso2022jp_state = state.escape_start;
            return null;
          }
          if (inRange(bite, 0x00, 0x7F)) {
            return bite;
          }
          if (bite === EOF_byte) {
            return EOF_code_point;
          }
          return decoderError(fatal);

        case state.escape_start:
          if (bite === 0x24 || bite === 0x28) {
            iso2022jp_lead = bite;
            iso2022jp_state = state.escape_middle;
            return null;
          }
          if (bite !== EOF_byte) {
            byte_pointer.offset(-1);
          }
          iso2022jp_state = state.ASCII;
          return decoderError(fatal);

        case state.escape_middle:
          var lead = iso2022jp_lead;
          iso2022jp_lead = 0x00;
          if (lead === 0x24 && (bite === 0x40 || bite === 0x42)) {
            iso2022jp_jis0212 = false;
            iso2022jp_state = state.lead;
            return null;
          }
          if (lead === 0x24 && bite === 0x28) {
            iso2022jp_state = state.escape_final;
            return null;
          }
          if (lead === 0x28 && (bite === 0x42 || bite === 0x4A)) {
            iso2022jp_state = state.ASCII;
            return null;
          }
          if (lead === 0x28 && bite === 0x49) {
            iso2022jp_state = state.Katakana;
            return null;
          }
          if (bite === EOF_byte) {
            byte_pointer.offset(-1);
          } else {
            byte_pointer.offset(-2);
          }
          iso2022jp_state = state.ASCII;
          return decoderError(fatal);

        case state.escape_final:
          if (bite === 0x44) {
            iso2022jp_jis0212 = true;
            iso2022jp_state = state.lead;
            return null;
          }
          if (bite === EOF_byte) {
            byte_pointer.offset(-2);
          } else {
            byte_pointer.offset(-3);
          }
          iso2022jp_state = state.ASCII;
          return decoderError(fatal);

        case state.lead:
          if (bite === 0x0A) {
            iso2022jp_state = state.ASCII;
            return decoderError(fatal, 0x000A);
          }
          if (bite === 0x1B) {
            iso2022jp_state = state.escape_start;
            return null;
          }
          if (bite === EOF_byte) {
            return EOF_code_point;
          }
          iso2022jp_lead = bite;
          iso2022jp_state = state.trail;
          return null;

        case state.trail:
          iso2022jp_state = state.lead;
          if (bite === EOF_byte) {
            return decoderError(fatal);
          }
          var code_point = null;
          var pointer = (iso2022jp_lead - 0x21) * 94 + bite - 0x21;
          if (inRange(iso2022jp_lead, 0x21, 0x7E) &&
              inRange(bite, 0x21, 0x7E)) {
            code_point = (iso2022jp_jis0212 === false) ?
                indexCodePointFor(pointer, indexes['jis0208']) :
                indexCodePointFor(pointer, indexes['jis0212']);
          }
          if (code_point === null) {
            return decoderError(fatal);
          }
          return code_point;

        case state.Katakana:
          if (bite === 0x1B) {
            iso2022jp_state = state.escape_start;
            return null;
          }
          if (inRange(bite, 0x21, 0x5F)) {
            return 0xFF61 + bite - 0x21;
          }
          if (bite === EOF_byte) {
            return EOF_code_point;
          }
          return decoderError(fatal);
      }
    };
  }

  



  function ISO2022JPEncoder(options) {
    var fatal = options.fatal;
    
    var state = {
      ASCII: 0,
      lead: 1,
      Katakana: 2
    };
    var  iso2022jp_state = state.ASCII;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if ((inRange(code_point, 0x0000, 0x007F) ||
           code_point === 0x00A5 || code_point === 0x203E) &&
          iso2022jp_state !== state.ASCII) {
        code_point_pointer.offset(-1);
        iso2022jp_state = state.ASCII;
        return output_byte_stream.emit(0x1B, 0x28, 0x42);
      }
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      if (code_point === 0x00A5) {
        return output_byte_stream.emit(0x5C);
      }
      if (code_point === 0x203E) {
        return output_byte_stream.emit(0x7E);
      }
      if (inRange(code_point, 0xFF61, 0xFF9F) &&
          iso2022jp_state !== state.Katakana) {
        code_point_pointer.offset(-1);
        iso2022jp_state = state.Katakana;
        return output_byte_stream.emit(0x1B, 0x28, 0x49);
      }
      if (inRange(code_point, 0xFF61, 0xFF9F)) {
        return output_byte_stream.emit(code_point - 0xFF61 - 0x21);
      }
      if (iso2022jp_state !== state.lead) {
        code_point_pointer.offset(-1);
        iso2022jp_state = state.lead;
        return output_byte_stream.emit(0x1B, 0x24, 0x42);
      }
      var pointer = indexPointerFor(code_point, indexes['jis0208']);
      if (pointer === null) {
        return encoderError(code_point);
      }
      var lead = div(pointer, 94) + 0x21;
      var trail = pointer % 94 + 0x21;
      return output_byte_stream.emit(lead, trail);
    };
  }

  name_to_encoding['iso-2022-jp'].getEncoder = function(options) {
    return new ISO2022JPEncoder(options);
  };
  name_to_encoding['iso-2022-jp'].getDecoder = function(options) {
    return new ISO2022JPDecoder(options);
  };

  

  



  function ShiftJISDecoder(options) {
    var fatal = options.fatal;
    var  shiftjis_lead = 0x00;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte && shiftjis_lead === 0x00) {
        return EOF_code_point;
      }
      if (bite === EOF_byte && shiftjis_lead !== 0x00) {
        shiftjis_lead = 0x00;
        return decoderError(fatal);
      }
      byte_pointer.offset(1);
      if (shiftjis_lead !== 0x00) {
        var lead = shiftjis_lead;
        shiftjis_lead = 0x00;
        if (inRange(bite, 0x40, 0x7E) || inRange(bite, 0x80, 0xFC)) {
          var offset = (bite < 0x7F) ? 0x40 : 0x41;
          var lead_offset = (lead < 0xA0) ? 0x81 : 0xC1;
          var code_point = indexCodePointFor((lead - lead_offset) * 188 +
                                             bite - offset, indexes['jis0208']);
          if (code_point === null) {
            return decoderError(fatal);
          }
          return code_point;
        }
        byte_pointer.offset(-1);
        return decoderError(fatal);
      }
      if (inRange(bite, 0x00, 0x80)) {
        return bite;
      }
      if (inRange(bite, 0xA1, 0xDF)) {
        return 0xFF61 + bite - 0xA1;
      }
      if (inRange(bite, 0x81, 0x9F) || inRange(bite, 0xE0, 0xFC)) {
        shiftjis_lead = bite;
        return null;
      }
      return decoderError(fatal);
    };
  }

  



  function ShiftJISEncoder(options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x0080)) {
        return output_byte_stream.emit(code_point);
      }
      if (code_point === 0x00A5) {
        return output_byte_stream.emit(0x5C);
      }
      if (code_point === 0x203E) {
        return output_byte_stream.emit(0x7E);
      }
      if (inRange(code_point, 0xFF61, 0xFF9F)) {
        return output_byte_stream.emit(code_point - 0xFF61 + 0xA1);
      }
      var pointer = indexPointerFor(code_point, indexes['jis0208']);
      if (pointer === null) {
        return encoderError(code_point);
      }
      var lead = div(pointer, 188);
      var lead_offset = lead < 0x1F ? 0x81 : 0xC1;
      var trail = pointer % 188;
      var offset = trail < 0x3F ? 0x40 : 0x41;
      return output_byte_stream.emit(lead + lead_offset, trail + offset);
    };
  }

  name_to_encoding['shift_jis'].getEncoder = function(options) {
    return new ShiftJISEncoder(options);
  };
  name_to_encoding['shift_jis'].getDecoder = function(options) {
    return new ShiftJISDecoder(options);
  };

  
  
  

  

  



  function EUCKRDecoder(options) {
    var fatal = options.fatal;
    var  euckr_lead = 0x00;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte && euckr_lead === 0) {
        return EOF_code_point;
      }
      if (bite === EOF_byte && euckr_lead !== 0) {
        euckr_lead = 0x00;
        return decoderError(fatal);
      }
      byte_pointer.offset(1);
      if (euckr_lead !== 0x00) {
        var lead = euckr_lead;
        var pointer = null;
        euckr_lead = 0x00;

        if (inRange(lead, 0x81, 0xC6)) {
          var temp = (26 + 26 + 126) * (lead - 0x81);
          if (inRange(bite, 0x41, 0x5A)) {
            pointer = temp + bite - 0x41;
          } else if (inRange(bite, 0x61, 0x7A)) {
            pointer = temp + 26 + bite - 0x61;
          } else if (inRange(bite, 0x81, 0xFE)) {
            pointer = temp + 26 + 26 + bite - 0x81;
          }
        }

        if (inRange(lead, 0xC7, 0xFD) && inRange(bite, 0xA1, 0xFE)) {
          pointer = (26 + 26 + 126) * (0xC7 - 0x81) + (lead - 0xC7) * 94 +
              (bite - 0xA1);
        }

        var code_point = (pointer === null) ? null :
            indexCodePointFor(pointer, indexes['euc-kr']);
        if (pointer === null) {
          byte_pointer.offset(-1);
        }
        if (code_point === null) {
          return decoderError(fatal);
        }
        return code_point;
      }

      if (inRange(bite, 0x00, 0x7F)) {
        return bite;
      }

      if (inRange(bite, 0x81, 0xFD)) {
        euckr_lead = bite;
        return null;
      }

      return decoderError(fatal);
    };
  }

  



  function EUCKREncoder(options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      var pointer = indexPointerFor(code_point, indexes['euc-kr']);
      if (pointer === null) {
        return encoderError(code_point);
      }
      var lead, trail;
      if (pointer < ((26 + 26 + 126) * (0xC7 - 0x81))) {
        lead = div(pointer, (26 + 26 + 126)) + 0x81;
        trail = pointer % (26 + 26 + 126);
        var offset = trail < 26 ? 0x41 : trail < 26 + 26 ? 0x47 : 0x4D;
        return output_byte_stream.emit(lead, trail + offset);
      }
      pointer = pointer - (26 + 26 + 126) * (0xC7 - 0x81);
      lead = div(pointer, 94) + 0xC7;
      trail = pointer % 94 + 0xA1;
      return output_byte_stream.emit(lead, trail);
    };
  }

  name_to_encoding['euc-kr'].getEncoder = function(options) {
    return new EUCKREncoder(options);
  };
  name_to_encoding['euc-kr'].getDecoder = function(options) {
    return new EUCKRDecoder(options);
  };

  

  



  function ISO2022KRDecoder(options) {
    var fatal = options.fatal;
    
    var state = {
      ASCII: 0,
      escape_start: 1,
      escape_middle: 2,
      escape_end: 3,
      lead: 4,
      trail: 5
    };
    var  iso2022kr_state = state.ASCII,
         iso2022kr_lead = 0x00;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite !== EOF_byte) {
        byte_pointer.offset(1);
      }
      switch (iso2022kr_state) {
        default:
        case state.ASCII:
          if (bite === 0x0E) {
            iso2022kr_state = state.lead;
            return null;
          }
          if (bite === 0x0F) {
            return null;
          }
          if (bite === 0x1B) {
            iso2022kr_state = state.escape_start;
            return null;
          }
          if (inRange(bite, 0x00, 0x7F)) {
            return bite;
          }
          if (bite === EOF_byte) {
            return EOF_code_point;
          }
          return decoderError(fatal);
        case state.escape_start:
          if (bite === 0x24) {
            iso2022kr_state = state.escape_middle;
            return null;
          }
          if (bite !== EOF_byte) {
            byte_pointer.offset(-1);
          }
          iso2022kr_state = state.ASCII;
          return decoderError(fatal);
        case state.escape_middle:
          if (bite === 0x29) {
            iso2022kr_state = state.escape_end;
            return null;
          }
          if (bite === EOF_byte) {
            byte_pointer.offset(-1);
          } else {
            byte_pointer.offset(-2);
          }
          iso2022kr_state = state.ASCII;
          return decoderError(fatal);
        case state.escape_end:
          if (bite === 0x43) {
            iso2022kr_state = state.ASCII;
            return null;
          }
          if (bite === EOF_byte) {
            byte_pointer.offset(-2);
          } else {
            byte_pointer.offset(-3);
          }
          iso2022kr_state = state.ASCII;
          return decoderError(fatal);
        case state.lead:
          if (bite === 0x0A) {
            iso2022kr_state = state.ASCII;
            return decoderError(fatal, 0x000A);
          }
          if (bite === 0x0E) {
            return null;
          }
          if (bite === 0x0F) {
            iso2022kr_state = state.ASCII;
            return null;
          }
          if (bite === EOF_byte) {
            return EOF_code_point;
          }
          iso2022kr_lead = bite;
          iso2022kr_state = state.trail;
          return null;
        case state.trail:
          iso2022kr_state = state.lead;
          if (bite === EOF_byte) {
            return decoderError(fatal);
          }
          var code_point = null;
          if (inRange(iso2022kr_lead, 0x21, 0x46) &&
              inRange(bite, 0x21, 0x7E)) {
            code_point = indexCodePointFor((26 + 26 + 126) *
                (iso2022kr_lead - 1) +
                26 + 26 + bite - 1,
                indexes['euc-kr']);
          } else if (inRange(iso2022kr_lead, 0x47, 0x7E) &&
              inRange(bite, 0x21, 0x7E)) {
            code_point = indexCodePointFor((26 + 26 + 126) * (0xC7 - 0x81) +
                (iso2022kr_lead - 0x47) * 94 +
                (bite - 0x21),
                indexes['euc-kr']);
          }
          if (code_point !== null) {
            return code_point;
          }
          return decoderError(fatal);
      }
    };
  }

  



  function ISO2022KREncoder(options) {
    var fatal = options.fatal;
    
    var state = {
      ASCII: 0,
      lead: 1
    };
    var  iso2022kr_initialization = false,
         iso2022kr_state = state.ASCII;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      if (!iso2022kr_initialization) {
        iso2022kr_initialization = true;
        output_byte_stream.emit(0x1B, 0x24, 0x29, 0x43);
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0x0000, 0x007F) &&
          iso2022kr_state !== state.ASCII) {
        code_point_pointer.offset(-1);
        iso2022kr_state = state.ASCII;
        return output_byte_stream.emit(0x0F);
      }
      if (inRange(code_point, 0x0000, 0x007F)) {
        return output_byte_stream.emit(code_point);
      }
      if (iso2022kr_state !== state.lead) {
        code_point_pointer.offset(-1);
        iso2022kr_state = state.lead;
        return output_byte_stream.emit(0x0E);
      }
      var pointer = indexPointerFor(code_point, indexes['euc-kr']);
      if (pointer === null) {
        return encoderError(code_point);
      }
      var lead, trail;
      if (pointer < (26 + 26 + 126) * (0xC7 - 0x81)) {
        lead = div(pointer, (26 + 26 + 126)) + 1;
        trail = pointer % (26 + 26 + 126) - 26 - 26 + 1;
        if (!inRange(lead, 0x21, 0x46) || !inRange(trail, 0x21, 0x7E)) {
          return encoderError(code_point);
        }
        return output_byte_stream.emit(lead, trail);
      }
      pointer = pointer - (26 + 26 + 126) * (0xC7 - 0x81);
      lead = div(pointer, 94) + 0x47;
      trail = pointer % 94 + 0x21;
      if (!inRange(lead, 0x47, 0x7E) || !inRange(trail, 0x21, 0x7E)) {
        return encoderError(code_point);
      }
      return output_byte_stream.emit(lead, trail);
    };
  }

  name_to_encoding['iso-2022-kr'].getEncoder = function(options) {
    return new ISO2022KREncoder(options);
  };
  name_to_encoding['iso-2022-kr'].getDecoder = function(options) {
    return new ISO2022KRDecoder(options);
  };


  
  
  

  

  




  function UTF16Decoder(utf16_be, options) {
    var fatal = options.fatal;
    var  utf16_lead_byte = null,
         utf16_lead_surrogate = null;
    




    this.decode = function(byte_pointer) {
      var bite = byte_pointer.get();
      if (bite === EOF_byte && utf16_lead_byte === null &&
          utf16_lead_surrogate === null) {
        return EOF_code_point;
      }
      if (bite === EOF_byte && (utf16_lead_byte !== null ||
                                utf16_lead_surrogate !== null)) {
        return decoderError(fatal);
      }
      byte_pointer.offset(1);
      if (utf16_lead_byte === null) {
        utf16_lead_byte = bite;
        return null;
      }
      var code_point;
      if (utf16_be) {
        code_point = (utf16_lead_byte << 8) + bite;
      } else {
        code_point = (bite << 8) + utf16_lead_byte;
      }
      utf16_lead_byte = null;
      if (utf16_lead_surrogate !== null) {
        var lead_surrogate = utf16_lead_surrogate;
        utf16_lead_surrogate = null;
        if (inRange(code_point, 0xDC00, 0xDFFF)) {
          return 0x10000 + (lead_surrogate - 0xD800) * 0x400 +
              (code_point - 0xDC00);
        }
        byte_pointer.offset(-2);
        return decoderError(fatal);
      }
      if (inRange(code_point, 0xD800, 0xDBFF)) {
        utf16_lead_surrogate = code_point;
        return null;
      }
      if (inRange(code_point, 0xDC00, 0xDFFF)) {
        return decoderError(fatal);
      }
      return code_point;
    };
  }

  




  function UTF16Encoder(utf16_be, options) {
    var fatal = options.fatal;
    




    this.encode = function(output_byte_stream, code_point_pointer) {
      function convert_to_bytes(code_unit) {
        var byte1 = code_unit >> 8;
        var byte2 = code_unit & 0x00FF;
        if (utf16_be) {
          return output_byte_stream.emit(byte1, byte2);
        }
        return output_byte_stream.emit(byte2, byte1);
      }
      var code_point = code_point_pointer.get();
      if (code_point === EOF_code_point) {
        return EOF_byte;
      }
      code_point_pointer.offset(1);
      if (inRange(code_point, 0xD800, 0xDFFF)) {
        encoderError(code_point);
      }
      if (code_point <= 0xFFFF) {
        return convert_to_bytes(code_point);
      }
      var lead = div((code_point - 0x10000), 0x400) + 0xD800;
      var trail = ((code_point - 0x10000) % 0x400) + 0xDC00;
      convert_to_bytes(lead);
      return convert_to_bytes(trail);
    };
  }

  name_to_encoding['utf-16'].getEncoder = function(options) {
    return new UTF16Encoder(false, options);
  };
  name_to_encoding['utf-16'].getDecoder = function(options) {
    return new UTF16Decoder(false, options);
  };

  
  name_to_encoding['utf-16be'].getEncoder = function(options) {
    return new UTF16Encoder(true, options);
  };
  name_to_encoding['utf-16be'].getDecoder = function(options) {
    return new UTF16Decoder(true, options);
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
(function(global) {










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

}(this));
(function(global) {

var BUFFER_DRAIN_INTERVAL = 100,        
    BUFFER_DRAIN_MAX_RETRIES = 10,      
    WEB_SOCKET_KEEP_ALIVE_INTERVAL = 9000,

    
    
    
    WEB_SOCKET_CONNECTIVITY_TIMEOUT = 5*WEB_SOCKET_KEEP_ALIVE_INTERVAL - 100





var wsCloseErrorCodes = {
  1002:  "The endpoint is terminating the connection due to a protocol error. (CLOSE_PROTOCOL_ERROR)",
  1003:  "The connection is being terminated because the endpoint received data of a type it cannot accept (for example, a text-only endpoint received binary data). (CLOSE_UNSUPPORTED)",
  1004:  "The endpoint is terminating the connection because a data frame was received that is too large. (CLOSE_TOO_LARGE)",
  1005:  "Indicates that no status code was provided even though one was expected. (CLOSE_NO_STATUS)",
  1006:  "Used to indicate that a connection was closed abnormally (that is, with no close frame being sent) when a status code is expected. (CLOSE_ABNORMAL)",
  1007: "Indicates that an endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [RFC3629] data within a text message)",
  1008: "Indicates that an endpoint is terminating the connection because it has received a message that violates its policy.  This is a generic status code that can be returned when there is no other more suitable status code (e.g., 1003 or 1009) or if there is a need to hide specific details about the policy",
  1009: "Indicates that an endpoint is terminating the connection because it has received a message that is too big for it to process",
  1011: "Indicates that a server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request",

  
  4001:   "Connectivity loss was detected as it was too long since the socket received the last PONG message"
};

OT.Rumor.SocketError = function(code, message) {
  this.code = code;
  this.message = message;
};



OT.Rumor.Socket = function(messagingServer, notifyDisconnectAddress, NativeSocket) {
  var server = messagingServer,
      webSocket,
      id,
      onOpen,
      onError,
      onClose,
      onMessage,
      connectCallback,
      bufferDrainTimeout,           
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

      setState = OT.$.statable(this, ['disconnected',  'error', 'connected', 'connecting', 'disconnecting'], 'disconnected', stateChanged),

      validateCallback = function validateCallback (name, callback) {
        if (callback === null || !OT.$.isFunction(callback) ) {
          throw new Error("The Rumor.Socket " + name + " callback must be a valid function or null");
        }
      },

      error = function error (errorMessage) {
        OT.error("Rumor.Socket: " + errorMessage);

        var error = new OT.Rumor.SocketError(null, errorMessage || "Unknown Socket Error");

        if (connectTimeout) clearTimeout(connectTimeout);

        setState('error');

        if (this.previousState === 'connecting' && connectCallback) {
          connectCallback(error, null);
          connectCallback = null;
        }

        if (onError) onError(error);
      }.bind(this),

      
      close = function close() {
        setState('disconnecting');

        if (bufferDrainTimeout) {
          clearTimeout(bufferDrainTimeout);
          bufferDrainTimeout = null;
        }
        console.info("CALLED CLOSE ON WEBSOCKET");
        webSocket.close();
      },

      
      
      
      disconnectWhenSendBufferIsDrained = function disconnectWhenSendBufferIsDrained (bufferDrainRetries) {
        if (!webSocket) return;

        if (bufferDrainRetries === void 0) bufferDrainRetries = 0;
        if (bufferDrainTimeout) clearTimeout(bufferDrainTimeout);

        if (webSocket.bufferedAmount > 0 && (bufferDrainRetries + 1) <= BUFFER_DRAIN_MAX_RETRIES) {
          bufferDrainTimeout = setTimeout(disconnectWhenSendBufferIsDrained, BUFFER_DRAIN_INTERVAL, bufferDrainRetries+1);
        }
        else {
          close();
        }
      },

      hasLostConnectivity = function hasLostConnectivity () {
        if (!lastMessageTimestamp) return false;

        return (OT.$.now() - lastMessageTimestamp) >= WEB_SOCKET_CONNECTIVITY_TIMEOUT;
      },

      sendKeepAlive = function sendKeepAlive () {
        if (!this.is('connected')) return;

        if ( hasLostConnectivity() ) {
          webSocketDisconnected({code: 4001});
        }
        else  {
          webSocket.send(OT.Rumor.Message.Ping().serialize());
          keepAliveTimer = setTimeout(sendKeepAlive.bind(this), WEB_SOCKET_KEEP_ALIVE_INTERVAL);
        }
      }.bind(this);


  
  var webSocketConnected = function webSocketConnected () {
        if (connectTimeout) clearTimeout(connectTimeout);

        
        
        
        
        webSocket.send(OT.Rumor.Message.Connect(id, notifyDisconnectAddress).serialize());

        setState('connected');
        if (connectCallback) {
          connectCallback(null, id);
          connectCallback = null;
        }

        if (onOpen) onOpen(id);

        setTimeout(function() {
          lastMessageTimestamp = OT.$.now();
          sendKeepAlive();
        }, WEB_SOCKET_KEEP_ALIVE_INTERVAL);
      },

      webSocketConnectTimedOut = function webSocketConnectTimedOut () {
        error("Timed out while waiting for the Rumor socket to connect.");
      },

      webSocketError = function webSocketError (errorEvent) {
        var errorMessage = "Unknown Socket Error";      

        
        
        
        
        
        
      },

      webSocketDisconnected = function webSocketDisconnected (closeEvent) {
        if (connectTimeout) clearTimeout(connectTimeout);
        if (keepAliveTimer) clearTimeout(keepAliveTimer);

        if (closeEvent.code !== 1000 && closeEvent.code !== 1001) {
          var reason = closeEvent.reason || closeEvent.message;
          if (!reason && wsCloseErrorCodes.hasOwnProperty(closeEvent.code)) reason = wsCloseErrorCodes[closeEvent.code];

          error("Rumor Socket Disconnected: " + reason);
        }

        if (this.isNot('error')) setState('disconnected');
      }.bind(this),

      webSocketReceivedMessage = function webSocketReceivedMessage (message) {
        lastMessageTimestamp = OT.$.now();

        if (onMessage) {
          var msg = OT.Rumor.Message.deserialize(message.data);

          if (msg.type !== OT.Rumor.MessageType.PONG) {
            onMessage(msg.toAddress, msg.data);
          }
        }
      };


  

  this.publish = function (topics, message) {
    webSocket.send(OT.Rumor.Message.Publish(topics, message).serialize());
  };

  this.subscribe = function(topics) {
    webSocket.send(OT.Rumor.Message.Subscribe(topics).serialize());
  };

  this.unsubscribe = function(topics) {
    webSocket.send(OT.Rumor.Message.Unsubscribe(topics).serialize());
  };

  this.connect = function (connectionId, complete) {
    if (this.is('connecting', 'connected')) {
      complete(new OT.Rumor.SocketError(null, "Rumor.Socket cannot connect when it is already connecting or connected."));
      return;
    }

    id = connectionId;
    connectCallback = complete;

    try {
      setState('connecting');

      webSocket = new (NativeSocket || WebSocket)(server);
      webSocket.binaryType = 'arraybuffer';

      webSocket.onopen = webSocketConnected;
      webSocket.onclose = webSocketDisconnected;
      webSocket.onerror = webSocketError;
      webSocket.onmessage = webSocketReceivedMessage;

      connectTimeout = setTimeout(webSocketConnectTimedOut, OT.Rumor.Socket.CONNECT_TIMEOUT);
    }
    catch(e) {
      OT.error(e);

      
      error("Could not connect to the Rumor socket, possibly because of a blocked port.")
    }
  };

  this.disconnect = function() {
    if (connectTimeout) clearTimeout(connectTimeout);
    if (keepAliveTimer) clearTimeout(keepAliveTimer);

    if (!webSocket) {
      if (this.isNot('error')) setState('disconnected');
      return;
    }

    if (webSocket.readyState === 3) {
      if (this.isNot('error')) setState('disconnected');
    }
    else {
      if (this.is('connected')) {
        
        webSocket.send(OT.Rumor.Message.Disconnect().serialize());
      }

      
      disconnectWhenSendBufferIsDrained();
    }
  };



  Object.defineProperties(this, {
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

}(this));
(function(global) {







OT.Rumor.Message = function (type, toAddress, headers, data) {
  this.type = type;
  this.toAddress = toAddress;
  this.headers = headers;
  this.data = data;
};


OT.Rumor.Message.prototype.serialize = function () {
  var bitStream = '',
      str = "",
      offset = 8,
      cBuf = 7,
      address = new Array(this.toAddress.length),
      headerKey = new Array(this.headers.length),
      headerVal = new Array(this.headers.length),
      dataView;

  
  cBuf++;

  
  for (var i = 0; i < this.toAddress.length; i++) {
    address[i] = new TextEncoder('utf-8').encode(this.toAddress[i]);
    cBuf += 2;
    cBuf += address[i].length;
  }

  
  cBuf++;

  
  for (var i = 0; i < this.headers.length; i++) {
    headerKey[i] = new TextEncoder('utf-8').encode(this.headers[i].key);
    headerVal[i] = new TextEncoder('utf-8').encode(this.headers[i].val);
    cBuf += 4;
    cBuf += headerKey[i].length;
    cBuf += headerVal[i].length;
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

  
  for (var i = 0; i < address.length; i++) {
    strArray = address[i];
    uint8View[offset++] = strArray.length >> 8 & 0xFF;
    uint8View[offset++] = strArray.length >> 0 & 0xFF;
    for (var j = 0; j < strArray.length; j++) {
      uint8View[offset++] = strArray[j];
    }
  }

  uint8View[offset++] = headerKey.length;

  
  for (var i = 0; i < headerKey.length; i++) {
    strArray = headerKey[i];
    uint8View[offset++] = strArray.length >> 8 & 0xFF;
    uint8View[offset++] = strArray.length >> 0 & 0xFF;
    for (var j = 0; j < strArray.length; j++) {
      uint8View[offset++] = strArray[j];
    }

    strArray = headerVal[i];
    uint8View[offset++] = strArray.length >> 8 & 0xFF;
    uint8View[offset++] = strArray.length >> 0 & 0xFF;
    for (var j = 0; j < strArray.length; j++) {
      uint8View[offset++] = strArray[j];
    }
  }

  
  for (var i = 0; i < dataView.length; i++) {
    uint8View[offset++] = dataView[i];
  }

  return buffer;
};

OT.Rumor.Message.deserialize = function (buffer) {
  var cBuf = 0;
  var type;
  var offset = 8;
  var uint8View = new Uint8Array(buffer);

  
  cBuf += uint8View[0] << 24;
  cBuf += uint8View[1] << 16;
  cBuf += uint8View[2] <<  8;
  cBuf += uint8View[3] <<  0;

  type = uint8View[6];
  var address = [];

  for (var i = 0; i < uint8View[7]; i++) {
    length = uint8View[offset++] << 8;
    length += uint8View[offset++];
    var strView = new Uint8Array(buffer, offset, length);
    address[i] = new TextDecoder('utf-8').decode(strView);
    offset += length;
  }

  var headerlen = uint8View[offset++];
  var headers = [];

  for (var i = 0; i < headerlen; i++) {
    length = uint8View[offset++] << 8;
    length += uint8View[offset++];
    var strView = new Uint8Array(buffer, offset, length);
    var keyStr = new TextDecoder('utf-8').decode(strView);
    offset += length;

    length = uint8View[offset++] << 8;
    length += uint8View[offset++];
    strView = new Uint8Array(buffer, offset, length);
    var valStr = new TextDecoder('utf-8').decode(strView);
    headers[i] =  { key : keyStr, val : valStr };
    offset += length;
  }

  var dataView = new Uint8Array(buffer, offset);
  var data = new TextDecoder('utf-8').decode(dataView);

 return new OT.Rumor.Message(type, address, headers, data);
};


OT.Rumor.Message.Connect = function (uniqueId, notifyDisconnectAddress) {
  var headers = [
    {key: 'uniqueId', val: uniqueId},
    {key: 'notifyDisconnectAddress', val: notifyDisconnectAddress}
  ];

  return new OT.Rumor.Message(OT.Rumor.MessageType.CONNECT, [], headers, "");
};

OT.Rumor.Message.Disconnect = function () {
  return new OT.Rumor.Message(OT.Rumor.MessageType.DISCONNECT, [], [], "");
};

OT.Rumor.Message.Subscribe = function(topics) {
  return new OT.Rumor.Message(OT.Rumor.MessageType.SUBSCRIBE, topics, [], "");
};

OT.Rumor.Message.Unsubscribe = function(topics) {
  return new OT.Rumor.Message(OT.Rumor.MessageType.UNSUBSCRIBE, topics, [], "");
};

OT.Rumor.Message.Publish = function(topics, message, headers) {
  return new OT.Rumor.Message(OT.Rumor.MessageType.MESSAGE, topics, [], message);
};





OT.Rumor.Message.Ping = function() {
  return new OT.Rumor.Message(OT.Rumor.MessageType.PING, [], [], "");
};

}(this));
(function(global) {































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


}(this));
(function(global) {



var typeToName = {},
    actionToName = {};


var messageTypes = OT.Raptor.Types;
for (var name in messageTypes) {
  typeToName[messageTypes[name]] = name;
}


var messageActions = OT.Raptor.Actions;
for (var name in messageActions) {
  actionToName[messageActions[name]] = name;
}


OT.Raptor.serializeMessage = function (message) {
    return JSON.stringify(message);
};











OT.Raptor.deserializeMessage = function (msg) {
  var message = JSON.parse(msg);

  if (message.type) {
    message.type = parseInt(message.type, 10);
    message.typeName = typeToName[message.type] || null;
  }

  if (message.action) {
    message.action = parseInt(message.action, 10);
    message.actionName = actionToName[message.action] || null;
  }

  message.signature = message.typeName + ':' + message.actionName;

  return message;
};


OT.Raptor.Message = {};
OT.Raptor.Message.connect = function (widgetId, connectionId, sessionId, apiKey, token, p2pEnabled) {
  var payload = {
        credentials: {
          connectionId: connectionId,
          soAccessState: 2,
          supportsWebRTC: true,
          p2pEnabled: p2pEnabled,
          GUID: widgetId,
          widgetId: widgetId,
          partnerId: apiKey
        },
        sessionId: sessionId,
        params: {
            tokenPermissions: {
                apiKey: apiKey
            },
            token: token
        },
        uniqueId: connectionId
      };

  return OT.Raptor.serializeMessage({
    id: OT.$.uuid(),
    type: OT.Raptor.Types.RPC_REQUEST,
    action: OT.Raptor.Actions.CONNECT,
    payload: payload,
    replyTo: connectionId
  });
};


OT.Raptor.Message.getSessionState = function (connectionId, sessionId, connectionsRequired) {
  return OT.Raptor.serializeMessage({
    id: OT.$.uuid(),
    type: OT.Raptor.Types.RPC_REQUEST,
    action: OT.Raptor.Actions.STATE,
    payload: {
      sessionId: sessionId,
      connectionsRequired: connectionsRequired || true
    },
    replyTo: connectionId
  });
};


OT.Raptor.Message.forceDisconnect = function (fromConnectionId, connectionIdToDisconnect, sessionId) {
  return OT.Raptor.serializeMessage({
    id: fromConnectionId,
    type: OT.Raptor.Types.RPC_REQUEST,
    action: OT.Raptor.Actions.FORCE_DISCONNECT,
    payload: {
      connectionId: connectionIdToDisconnect,
      sessionId: sessionId
    },
    replyTo: fromConnectionId
  });
};



OT.Raptor.Message.streamCreate = function (connectionId, sessionId, publisherId, name, videoOrientation, videoWidth, videoHeight, hasAudio, hasVideo, p2pEnabled) {
  var payload = {
    key: sessionId,
    value: {
      p2pEnabled: p2pEnabled || false,
      publisherId: publisherId,
      connection: {
          connectionId: connectionId
      },
      type: "WebRTC",
      name: name || '',
      creationTime: Date.now(),   
      orientation: {
          width: videoWidth,
          height: videoHeight,
          videoOrientation: videoOrientation || "OTVideoOrientationRotatedNormal"
      },
      hasAudio: hasAudio !== void 0 ? hasAudio : true,
      hasVideo: hasVideo !== void 0 ? hasVideo : true
    }
  };

  return OT.Raptor.serializeMessage({
    id: connectionId, 
    type: OT.Raptor.Types.STREAM,
    action: OT.Raptor.Actions.CREATE,
    payload: payload,
    replyTo: ''
  });
};


OT.Raptor.Message.streamDestroy = function (connectionId, sessionId, streamId) {
  return OT.Raptor.serializeMessage({
    id: connectionId,
    type: OT.Raptor.Types.STREAM,
    action: OT.Raptor.Actions.DELETE,
    payload: {
      key: sessionId + "/STREAMS/" + streamId
    },
    replyTo: connectionId
  });
};


OT.Raptor.Message.streamModify = function (connectionId, sessionId, streamId, key, value) {
  return OT.Raptor.serializeMessage({
    id: connectionId,
    type: OT.Raptor.Types.STREAM,
    action: OT.Raptor.Actions.UPDATE,
    payload: {
      key: [sessionId, 'STREAMS', streamId, key].join('/'),
      value: value
    },
    replyTo: connectionId
  });
};

OT.Raptor.Message.forceUnpublish = function (fromConnectionId, sessionId, streamIdToUnpublish) {
  return OT.Raptor.serializeMessage({
    id: fromConnectionId,
    type: OT.Raptor.Types.RPC_REQUEST,
    action: OT.Raptor.Actions.FORCE_UNPUBLISH,
    payload: {
      sessionId: sessionId,
      connectionId: fromConnectionId,
      streamId: streamIdToUnpublish,
      webRTCStream: true
    },
    replyTo: fromConnectionId
  });
};


OT.Raptor.Message.jsepOffer = function (fromConnectionId, toConnectionId, streamId, sdp) {
  return OT.Raptor.serializeMessage({
    id: fromConnectionId,
    type: OT.Raptor.Types.JSEP,
    action: OT.Raptor.Actions.OFFER,
    payload: {
      fromAddress: fromConnectionId,
      toAddresses: toConnectionId,
      sdp: sdp,
      streamId: streamId
    },
    replyTo: fromConnectionId
  });
};


OT.Raptor.Message.jsepAnswer = function (fromConnectionId, toConnectionId, streamId, sdp) {
  return OT.Raptor.serializeMessage({
    id: fromConnectionId,
    type: OT.Raptor.Types.JSEP,
    action: OT.Raptor.Actions.ANSWER,
    payload: {
      fromAddress: fromConnectionId,
      toAddresses: toConnectionId,
      sdp: sdp,
      streamId: streamId
    },
    replyTo: fromConnectionId
  });
};

OT.Raptor.Message.jsepSubscribe = function (fromConnectionId, toConnectionId, streamId, subscribeToVideo, subscribeToAudio) {
  return OT.Raptor.serializeMessage({
    id: fromConnectionId,
    type: OT.Raptor.Types.JSEP,
    action: OT.Raptor.Actions.SUBSCRIBE,
    payload: {
      keyManagemenMethod: OT.$.supportedCryptoScheme(),
      bundleSupport: OT.$.supportsBundle(),
      rtcpMuxSupport: OT.$.supportsRtcpMux(),
      fromAddress: fromConnectionId,
      toAddresses: toConnectionId,
      streamId: streamId,
      hasVideo: subscribeToVideo,
      hasAudio: subscribeToAudio
    },
    replyTo: fromConnectionId
  });
};



OT.Raptor.Message.jsepUnsubscribe = function (fromConnectionId, toConnectionId, streamId) {
  return OT.Raptor.serializeMessage({
    id: fromConnectionId,
    type: OT.Raptor.Types.JSEP,
    action: OT.Raptor.Actions.UNSUBSCRIBE,
    payload: {
      fromAddress: fromConnectionId,
      toAddresses: toConnectionId,
      streamId: streamId
    },
    replyTo: fromConnectionId
  });
};

OT.Raptor.Message.jsepCandidate = function (fromConnectionId, toConnectionId, streamId, candidate) {
  return OT.Raptor.serializeMessage({
    id: fromConnectionId,
    type: OT.Raptor.Types.JSEP,
    action: OT.Raptor.Actions.CANDIDATE,
    payload: {
      fromAddress: fromConnectionId,
      toAddresses: toConnectionId,
      candidate: candidate,
      streamId: streamId
    },
    replyTo: fromConnectionId
  });
};

OT.Raptor.Message.subscriberModify = function (connectionId, sessionId, streamId, subscriberId, key, value) {
  return OT.Raptor.serializeMessage({
    id: OT.$.uuid(),
    type: OT.Raptor.Types.SUBSCRIBER,
    action: OT.Raptor.Actions.UPDATE,
    payload: {
      key: [sessionId, 'SUBSCRIBER', streamId, connectionId, key].join('/'),
      value: value
    },
    replyTo: connectionId
  });
};


OT.Raptor.Message.signal = function(connectionId, sessionId, type, to, data) {
  var payload = {
    id: OT.$.uuid(),
    fromAddress: connectionId
  };

  
  
  if (to && !Array.isArray(to)) {
    payload.toAddresses = [to];
  }
  else if (!to || to.length === 0) {
    
    payload.toAddresses = [sessionId];
  }
  else {
    payload.toAddresses = to;
  }

  if (type !== void 0) payload.type = type;
  if (data !== void 0) payload.data = data;

  return OT.Raptor.serializeMessage({
    id: connectionId,
    type: OT.Raptor.Types.SIGNAL,
    action: OT.Raptor.Actions.SIGNAL,
    payload: payload,
    replyTo: connectionId
  });
};

}(this));
(function(global) {

var MAX_SIGNAL_DATA_LENGTH = 8192;
var MAX_SIGNAL_TYPE_LENGTH = 128;












OT.Signal = function(sessionId, fromConnectionId, options) {
  var isInvalidType = function(type) {
      
      return !/^[a-zA-Z0-9\-\._~]+$/.exec(type);
    },

    validateTo = function(toAddress) {
      if (!toAddress) {
        return {code: 400, reason: "The signal type was null or an empty String. Either set it to a non-empty String value or omit it"};
      }
      else if ( !Array.isArray(toAddress) ) {
        return {code: 400, reason: "The To field was invalid"};
      }

      for (var i=0; i<toAddress.length; i++) {
        if ( !(toAddress[i] instanceof OT.Connection || toAddress[i] instanceof OT.Session) ) {
          return {code: 400, reason: "The To field was invalid"};
        }
      }

      return null;
    },

    validateType = function(type) {
      var error = null;

      if (type === null || type === void 0) {
        error = {code: 400, reason: "The signal type was null or undefined. Either set it to a String value or omit it"};
      }
      else if (type.length > MAX_SIGNAL_TYPE_LENGTH) {
        error = {code: 413, reason: "The signal type was too long, the maximum length of it is " + MAX_SIGNAL_TYPE_LENGTH + " characters"};
      }
      else if ( isInvalidType(type) ) {
        error = {code: 400, reason: "The signal type was invalid, it can only contain letters, numbers, '-', '_', and '~'."};
      }

      return error;
    },

    validateData = function(data) {
      var error = null;
      if (data === null || data === void 0) {
        error = {code: 400, reason: "The signal data was null or undefined. Either set it to a String value or omit it"};
      }
      else {
        try {
          if (JSON.stringify(data).length > MAX_SIGNAL_DATA_LENGTH) {
            error = {code: 413, reason: "The data field was too long, the maximum size of it is " + MAX_SIGNAL_DATA_LENGTH + " characters"};
          }
        }
        catch(e) {
          error = {code: 400, reason: "The data field was not valid JSON"};
        }
      }

      return error;
    };


  this.toRaptorMessage = function() {
    var to;

    if (this.to) {
      to = this.to.map(function(thing) {
        return typeof(thing) === 'string' ? thing : thing.id;
      });
    }

    return OT.Raptor.Message.signal(fromConnectionId, sessionId, this.type, to, this.data);
  };

  this.toHash = function() {
    var h = OT.$.clone(options);
    if (h.to === void 0) h.to = null;
    if (h.data === void 0) h.data = null;

    return h;
  };


  this.error = null;

  if (options) {
    if (options.hasOwnProperty('data')) {
      this.data = OT.$.clone(options.data);
      this.error = validateData(this.data);
    }

    if (options.hasOwnProperty('to')) {
      if (!Array.isArray(options.to)) {
        this.to = [options.to];
      }
      else {
        this.to = OT.$.clone(options.to);
      }

      if (!this.error) this.error = validateTo(this.to)
    }

    if (options.hasOwnProperty('type')) {
      if (!this.error) this.error = validateType(options.type)
      this.type = options.type;
    }
  }

  this.valid = this.error === null;
};

}(this));
(function(global) {


function SignalError(code, reason, signal) {
    this.code = code;
    this.reason = reason;
    this.signal = signal;
}



OT.Raptor.Socket = function(widgetId, messagingServer, Dispatcher) {
  var _socketUrl = OT.properties.messagingProtocol + "://" + messagingServer + ":" + OT.properties.messagingPort + "/rumorwebsocketsv2",
      _symphony = "symphony." + messagingServer,
      _sessionId,
      _rumor,
      _dispatcher,
      _completion,
      _capabilities = new OT.Capabilities([]),
      _analytics = new OT.Analytics();


  
  var setState = OT.$.statable(this, ['disconnected', 'connecting', 'connected', 'error', 'disconnecting'], 'disconnected'),

      logAnalyticsEvent = function logAnalyticsEvent (variation, payloadType, payload, options) {
        var event = {
          action: 'Connect',
          variation: variation,
          payload_type: payloadType,
          payload: payload,
          session_id: _sessionId,
          partner_id: OT.APIKEY,
          widget_id: widgetId,
          widget_type: 'Controller'
        };

        if (options) event = OT.$.extend(options, event);
        _analytics.logEvent(event);
      },

      onConnectComplete = function onConnectComplete (error, sessionState, prefix) {
        if (error) {
          logAnalyticsEvent('Failure', "reason|webSocketServerUrl", prefix + error.code + ':' + error.message + '|' + _socketUrl);

          setState('error');
        }
        else {
          logAnalyticsEvent('Success', "webSocketServerUrl", _socketUrl, {connectionId: _rumor.id});

          setState('connected');
          _capabilities = new OT.Capabilities(sessionState.permissions);
        }

        _completion.apply(null, arguments);
      },

      onClose = function onClose (err) {
        var session = OT.sessions.get(_sessionId),
            connection = session.connection,
            reason = this.is('disconnecting') ? "clientDisconnected" : "networkDisconnected";

        if(err && err.code == 4001) {
          reason = "networkTimedout";
        }

        setState('disconnected');

        if (!connection) return;

        if (connection.destroyedReason) {
          console.debug("OT.Raptor.Socket: Socket was closed but the connection had already been destroyed. Reason: " + connection.destroyedReason);
          return;
        }

        connection.destroy( reason );
      }.bind(this),

      onError = function onError () {
        
      };


  


  
  
  this.permittedTo = function (action) {
      return _capabilities[action] === 1;
  };

  this.connect = function (token, sessionInfo, completion) {
    if (!this.is('disconnected', 'error')) {
      OT.warn("Cannot connect the Raptor Socket as it is currently connected. You should disconnect first.");
      return;
    }

    setState('connecting');
    _sessionId = sessionInfo.sessionId;
    _completion = completion;

    var connectionId = OT.$.uuid(),
        session = OT.sessions.get(_sessionId);


    var analyticsPayload = [_socketUrl, navigator.userAgent, OT.properties.version, window.externalHost ? "yes" : "no"];
    logAnalyticsEvent(
      'Attempt',
      "webSocketServerUrl|userAgent|sdkVersion|chromeFrame",
      analyticsPayload.map(function(e) { return e.replace('|', '\\|'); }).join('|')
    );

    _rumor = new OT.Rumor.Socket(_socketUrl, _symphony);
    _rumor.onClose = onClose;
    _rumor.onMessage = _dispatcher.dispatch.bind(_dispatcher);

    _rumor.connect(connectionId, function(error) {
      if (error) {
        onConnectComplete(error, null, "RumorConnection:");
        return;
      }

      
      _rumor.onError = onError;

      OT.debug("Raptor Socket connected to " + _sessionId + " on " + messagingServer);

      _rumor.subscribe([_sessionId]);

      
      var connectMessage = OT.Raptor.Message.connect(widgetId, _rumor.id, _sessionId, OT.APIKEY, token, sessionInfo.p2pEnabled);
      this.publish(connectMessage);
    }.bind(this));
  };


  this.disconnect = function () {
    if (this.is('disconnected')) return;

    setState('disconnecting');
    _rumor.disconnect();
  };

  
  this.publish = function (message) {
    if (_rumor.isNot('connected')) {
      OT.error("OT.Raptor.Socket: cannot publish until the socket is connected." + message);
      return;
    }

    OT.debug("OT.Raptor.Socket Publish: " + message);
    _rumor.publish([_symphony], message);
  };

  
  this.createStream = function(publisherId, name, orientation, encodedWidth, encodedHeight, hasAudio, hasVideo) {
    var session = OT.sessions.get(_sessionId),
        message = OT.Raptor.Message.streamCreate( _rumor.id,
                                                  _sessionId,
                                                  publisherId,
                                                  name,
                                                  orientation,
                                                  encodedWidth,
                                                  encodedHeight,
                                                  hasAudio,
                                                  hasVideo,
                                                  session.sessionInfo.p2pEnabled );

    this.publish(message);
  };

  this.updateStream = function(streamId, key, value) {
    this.publish( OT.Raptor.Message.streamModify(_rumor.id, _sessionId, streamId, key, value) );
  };

  this.destroyStream = function(streamId) {
    this.publish( OT.Raptor.Message.streamDestroy(_rumor.id, _sessionId, streamId) );
  };

  this.modifySubscriber = function(subscriber, key, value) {
    this.publish( OT.Raptor.Message.subscriberModify(_rumor.id, _sessionId, subscriber.streamId, subscriber.widgetId, key, value) );
  };

  this.forceDisconnect = function(connectionIdToDisconnect) {
    this.publish( OT.Raptor.Message.forceDisconnect(_rumor.id, connectionIdToDisconnect, _sessionId) );
  };

  this.forceUnpublish = function(streamId) {
    this.publish( OT.Raptor.Message.forceUnpublish(_rumor.id, _sessionId, streamId) );
  };

  this.jsepSubscribe = function(toConnectionId, streamId, subscribeToVideo, subscribeToAudio) {
    this.publish( OT.Raptor.Message.jsepSubscribe(_rumor.id, toConnectionId, streamId, subscribeToVideo, subscribeToAudio) );
  };

  this.jsepUnsubscribe = function(toConnectionId, streamId) {
    this.publish( OT.Raptor.Message.jsepUnsubscribe(_rumor.id, toConnectionId, streamId) );
  };

  this.jsepCandidate = function(toConnectionId, streamId, candidate) {
    this.publish( OT.Raptor.Message.jsepCandidate(_rumor.id, toConnectionId, streamId, candidate) );
  };

  this.jsepOffer = function(toConnectionId, streamId, offerSDP) {
    this.publish( OT.Raptor.Message.jsepOffer(_rumor.id, toConnectionId, streamId, offerSDP) );
  };

  this.jsepAnswer = function(toConnectionId, streamId, answerSDP) {
    this.publish( OT.Raptor.Message.jsepAnswer(_rumor.id, toConnectionId, streamId, answerSDP) );
  };


  this.signal = function(options, completion) {
    var signal = new OT.Signal(_sessionId, _rumor.id, options || {});

    if (!signal.valid) {
      if (completion && OT.$.isFunction(completion)) {
        completion( new SignalError(signal.error.code, signal.error.reason, signal.toHash()) );
      }

      return;
    }

    this.publish( signal.toRaptorMessage() );
    if (completion && OT.$.isFunction(completion)) completion(null, signal.toHash());
  };

  OT.$.defineGetters(this, {
    id: function() { return _rumor.id; },
    capabilities: function() { return _capabilities; },
    sessionId: function() { return _sessionId; }
  });

  _dispatcher = new (Dispatcher || OT.Raptor.Dispatcher)(this, function (error, sessionState) {
    onConnectComplete.call(this, error, sessionState, "ConnectToSession:");
  });
};


}(this));
(function(global) {


var connectErrorReasons = {
  409: "This P2P session already has 2 participants.",
  410: "The session already has four participants.",
  1004: "The token passed is invalid."
};



OT.publishers = new OT.Collection('guid');          
OT.subscribers = new OT.Collection('widgetId');     
OT.sessions = new OT.Collection();

function parseStream(dict, session) {
  var stream = new OT.Stream( dict.streamId,
                              session.connections.get(dict.connection.connectionId),
                              dict.name,
                              dict.streamData,
                              dict.type,
                              dict.creationTime,
                              dict.hasAudio,
                              dict.hasVideo,
                              dict.orientation ? dict.orientation.videoOrientation : null,
                              dict.peerId,
                              dict.quality,
                              dict.orientation ? dict.orientation.width : null,
                              dict.orientation ? dict.orientation.height : null );

  
  
  
  if (dict.publisherId) {
    stream.publisherId = dict.publisherId;
  }

  return stream;
}

function parseAndAddStreamToSession(dict, session) {
  if (session.streams.has(dict.streamId)) return;

  var stream = parseStream(dict, session);
  session.streams.add( stream );

  return stream;
}


OT.Raptor.Dispatcher = function (socket, connectionCompletion) {
  this.socket = socket;
  this.connectionCompletion = connectionCompletion;
};

OT.Raptor.Dispatcher.prototype.dispatch = function(addresses, encodedMessage) {
  var message = OT.Raptor.deserializeMessage(encodedMessage);

  if (!message.typeName) {
    OT.error("OT.Raptor.dispatch: Invalid message type (" + message.type + ")");
    return;
  }

  if (!message.actionName) {
    OT.error("OT.Raptor.dispatch: Invalid action (" + message.action + ") for " + message.typeName);
    OT.error(message);
    return;
  }

  OT.debug("OT.Raptor.dispatch " + message.signature + ": " + encodedMessage);

  switch(message.type) {
    case OT.Raptor.Types.RPC_RESPONSE:
      this.dispatchRPCResponse(message);
      break;

    case OT.Raptor.Types.CONNECTION:
      this.dispatchConnection(message);
      break;

    case OT.Raptor.Types.CONNECTIONCOUNT:
      this.dispatchConnectionCount(message);
      break;

    case OT.Raptor.Types.STREAM:
      this.dispatchStream(message);
      break;

    case OT.Raptor.Types.SUBSCRIBER:
      this.dispatchSubscriber(message);
      break;

    case OT.Raptor.Types.MODERATION:
      this.dispatchModeration(message);
      break;

    case OT.Raptor.Types.JSEP:
      this.dispatchJsep(message);
      break;

    case OT.Raptor.Types.SIGNAL:
      this.dispatchSignal(message);
      break;



    default:
      OT.warn("OT.Raptor.dispatch: Type " + message.typeName + " is not currently implemented");
  }
};


OT.Raptor.Dispatcher.prototype.dispatchRPCResponse = function (message) {
  switch (message.action) {
    case OT.Raptor.Actions.CONNECT:
      if (message.payload.connectSuccess == false) {
        var error = new OT.Error(OT.ExceptionCodes.CONNECT_REJECTED, connectErrorReasons[message.payload.reason] || "Failed to connect");
        this.connectionCompletion.call(null, error);
      }
      else {
        this.socket.publish(OT.Raptor.Message.getSessionState(this.socket.id, message.payload.sessionId, true));
      }
      break;


    case OT.Raptor.Actions.STATE:
      var state = message.payload.value,
          session = OT.sessions.get(message.payload.key),
          connection;

      state.streams = [];
      state.connections = [];
      state.archives = [];

      if (state.hasOwnProperty("CONNECTIONS")) {
        for (var id in state.CONNECTIONS) {
          var connection = OT.Connection.fromHash(state.CONNECTIONS[id]);
          state.connections.push(connection);
          session.connections.add(connection)
        }

        delete state.CONNECTIONS;
      }

      if (state.hasOwnProperty("STREAMS")) {
        for (var id in state.STREAMS) {
          state.streams.push( parseAndAddStreamToSession(state.STREAMS[id], session) );
        }

        delete state.STREAMS;
      }


      this.connectionCompletion.call(null, null, state);
      break;


    default:
      OT.warn("OT.Raptor.dispatch: " + message.signature + " is not currently implemented");
  }
};


OT.Raptor.Dispatcher.prototype.dispatchConnection = function (message) {
  var session = OT.sessions.get(message.payload.value.sessionId),
      connection;

  if (!session) {
    OT.error("OT.Raptor.dispatch: Unable to determine session for " + message.payload.value.sessionId + ' on ' + message.signature + " message!");
    
    return;
  }

  switch (message.action) {
    case OT.Raptor.Actions.CREATE:
      connection = OT.Connection.fromHash(message.payload.value);
      if (session.connection && connection.id !== session.connection.id) session.connections.add( connection );
      break;


    case OT.Raptor.Actions.DELETE:
      connection = session.connections.get(message.payload.value.connectionId);
      connection.destroy(message.payload.reason);
      break;


    default:
      OT.warn("OT.Raptor.dispatch: " + message.signature + " is not currently implemented");
  }
};

OT.Raptor.Dispatcher.prototype.dispatchConnectionCount = function (message) {
  

  
  
  
  
  


  
  
  
};


OT.Raptor.Dispatcher.prototype.dispatchStream = function (message) {
  var key = message.payload.key.split('/'),
      sessionId = key[0],
      session,
      stream;


  if (sessionId) session = OT.sessions.get(sessionId);

  if (!session) {
    OT.error("OT.Raptor.dispatch: Unable to determine sessionId, or the session does not exist, for " + message.signature + " message!");
    
    return;
  }

  switch (message.action) {
    case OT.Raptor.Actions.REGISTER:
      stream = parseStream(message.payload.value, session);

      
      if (stream.publisherId) {
        var publisher = OT.publishers.get(stream.publisherId);

        if (publisher) {
          publisher._.streamRegisteredHandler(stream);
        }
        else {
          OT.warn("OT.Raptor.dispatch: Could find a publisher " + stream.publisherId + " for " + message.signature);
        }
      }
      break;


    case OT.Raptor.Actions.CREATE:
      parseAndAddStreamToSession(message.payload.value, session)
      break;


    case OT.Raptor.Actions.UPDATE:
      if (key[1]) stream = session.streams.get(key[1]);

      if (!stream) {
        OT.error("OT.Raptor.dispatch: Unable to determine streamId, or the stream does not exist, for " + message.signature + " message!");
        
        return;
      }

      stream.update(key[2], message.payload.value);
      break;


    case OT.Raptor.Actions.DELETE:
      if (key[2]) stream = session.streams.get(key[2]);

      if (!stream) {
        OT.error("OT.Raptor.dispatch: Unable to determine streamId, or the stream does not exist, for " + message.signature + " message!");
        
        return;
      }

      stream.destroy(message.payload.reason);
      break;


    default:
      OT.warn("OT.Raptor.dispatch: " + message.signature + " is not currently implemented");
  }
};



OT.Raptor.Dispatcher.prototype.dispatchModeration = function (message) {
  
  return;

  

  
  
  
  

  
  
  
  
  
  

  
  
  


  
  
  
  
  

  
  
  


  
  
  
};



OT.Raptor.Dispatcher.prototype.dispatchJsep = function (message) {
  var fromConnection,
      streamId = message.payload.streamId,
      actors;

  switch (message.action) {
    
    case OT.Raptor.Actions.OFFER:
      actors = [];
      var subscriber = OT.subscribers.find({streamId: streamId});
      if (subscriber) actors.push(subscriber);
      break;


    
    case OT.Raptor.Actions.ANSWER:
    case OT.Raptor.Actions.PRANSWER:
    case OT.Raptor.Actions.SUBSCRIBE:
    case OT.Raptor.Actions.UNSUBSCRIBE:
      actors = OT.publishers.where({streamId: streamId});
      break;


    
    case OT.Raptor.Actions.CANDIDATE:
      
      actors = OT.publishers.where({streamId: streamId}).concat(OT.subscribers.where({streamId: streamId}));
      break;


    default:
      OT.warn("OT.Raptor.dispatch: " + message.signature + " is not currently implemented");
      return;
  }

  if (actors.length) {
    
    
    
    fromConnection = actors[0].session.connections.get(message.payload.fromAddress);
    if(!fromConnection && message.payload.fromAddress.match(/^symphony\./)) {
      fromConnection = new OT.Connection(message.payload.fromAddress,
          Date.now(), null, { supportsWebRTC: true });
      actors[0].session.connections.add(fromConnection);
    } else if(!fromConnection) {
      OT.warn("Messsage comes from a connection (" + message.payload.fromAddress + ") that we do not know about.");
    }
  }

  actors.forEach(function(actor) {
    actor.processMessage(message.action, fromConnection, message.payload);
  });
};








OT.Raptor.Dispatcher.prototype.dispatchSubscriber = function (message) {
  var key = message.payload.key.split('/'),
      session = OT.sessions.get(key[0]),
      subscriber,
      stream;

  if (!session) {
    OT.error("OT.Raptor.dispatch: Unable to determine sessionId, or the session does not exist, for " + message.signature + " message!");
    
    return;
  }

  stream = key[2] ? session.streams.get(key[2]) : null;

  if (!stream) {
    OT.error("OT.Raptor.dispatch: Unable to determine streamId, or the stream does not exist, for " + message.signature + " message!");
    
    return;
  }

  
  subscriber = OT.subscribers.find(function(subscriber) {
                  return subscriber.streamId === stream.id
                          && subscriber.session.id === session.id
                });

  if (!subscriber) {
    OT.error("OT.Raptor.dispatch: Unable to determine subscriberId, or the subscriber does not exist, for " + message.signature + " message!");
    
    return;
  }


  switch (message.action) {
    case OT.Raptor.Actions.QUALITY_CHANGED:
      subscriber.updateQuality(parseInt(message.payload.value, 10));
      break;


    default:
      OT.warn("OT.Raptor.dispatch: " + message.signature + " is not currently implemented");
  }
};


OT.Raptor.Dispatcher.prototype.dispatchSignal = function (message) {
  if (message.action !== OT.Raptor.Actions.SIGNAL) {
    OT.warn("OT.Raptor.dispatch: " + message.signature + " is not currently implemented");
    return;
  }

  var session = OT.sessions.get(this.socket.sessionId);

  if (!session) {
      OT.error("OT.Raptor.dispatch: " + message.signature + " ERROR - sessionId must be provided and be valid");
      return;
  }

  session._.dispatchSignal( session.connections.get(message.payload.fromAddress),
                            message.payload.type,
                            message.payload.data);
};

}(this));
(function(window) {








function EnvironmentLoader() {
    var _configReady = false,
        _domReady = false,

        isReady = function() {
            return _domReady && _configReady;
        },

        onLoaded = function() {
            if (isReady()) {
                OT.dispatchEvent(new OT.EnvLoadedEvent(OT.Event.names.ENV_LOADED));
            }
        },

        onDomReady = function() {
            _domReady = true;

            
            
            OT.$.on(window, "unload", function() {
                OT.publishers.destroy();
                OT.subscribers.destroy();
                OT.sessions.destroy();
            });

            
            OT.Config.load(OT.properties.configURL);

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
    if (document.readyState == "complete" || (document.readyState == "interactive" && document.body)) {
        onDomReady();
    } else {
        if (document.addEventListener) {
            document.addEventListener("DOMContentLoaded", onDomReady, false);
        } else if (document.attachEvent) {
            
            document.attachEvent("onreadystatechange", function() {
                if (document.readyState == "complete") onDomReady();
            });
        }
    }

    this.onLoad = function(cb) {
        if (isReady()) {
            cb();
            return;
        }

        OT.on(OT.Event.names.ENV_LOADED, cb);
    };
}

var EnvLoader = new EnvironmentLoader();

OT.onLoad = function(cb, context) {
    if (!context) {
        EnvLoader.onLoad(cb);
    }
    else {
        EnvLoader.onLoad(
            cb.bind(context)
        );
    }
};

})(window);
(function(window) {

OT.Error = function(code, message) {
    this.code = code;
    this.message = message;
};

var errorsCodesToTitle = {
    1000: "Failed To Load",
    1004: "Authentication error",
    1005: "Invalid Session ID",
    1006: "Connect Failed",
    1007: "Connect Rejected",
    1008: "Connect Time-out",
    1009: "Security Error",
    1010: "Not Connected",
    1011: "Invalid Parameter",
    1012: "Peer-to-peer Stream Play Failed",
    1013: "Connection Failed",
    1014: "API Response Failure",
    1500: "Unable to Publish",
    1510: "Unable to Signal",
    1520: "Unable to Force Disconnect",
    1530: "Unable to Force Unpublish",
    1540: "Unable to record archive",
    1550: "Unable to play back archive",
    1560: "Unable to create archive",
    1570: "Unable to load archive",
    2000: "Internal Error",
    2001: "Embed Failed",
    3000: "Archive load exception",
    3001: "Archive create exception",
    3002: "Playback stop exception",
    3003: "Playback start exception",
    3004: "Record start exception",
    3005: "Record stop exception",
    3006: "Archive load exception",
    3007: "Session recording in progress",
    3008: "Archive recording internal failure",
    4000: "WebSocket Connection Failed",
    4001: "WebSocket Network Disconnected"
};

var analytics;

function _exceptionHandler(component, msg, errorCode, context) {
    var title = errorsCodesToTitle[errorCode],
        contextCopy = context ? OT.$.clone(context) : {};

    OT.error("TB.exception :: title: " + title + " (" + errorCode + ") msg: " + msg);

    if (!contextCopy.partnerId) contextCopy.partnerId = OT.APIKEY;

    try {
        if (!analytics) analytics = new OT.Analytics();
        analytics.logError(errorCode, 'tb.exception', title, {details:msg}, contextCopy);

        OT.dispatchEvent(
            new OT.ExceptionEvent(OT.Event.names.EXCEPTION, msg, title, errorCode, component, component)
        );
    } catch(err) {
        OT.error("TB.exception :: Failed to dispatch exception - " + err.toString());
        
        
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

        if (session.connected) context.connectionId = session.connection.connectionId;
        if (!options.target) options.target = session;
    }
    else if (options.sessionId) {
        context = {
            sessionId: options.sessionId
        };

        if (!options.target) options.target = null;
    }

    _exceptionHandler(options.target, errorMsg, code, context);
};









OT.exceptionHandler = function(componentId, msg, errorTitle, errorCode, context) {
    var target;

    if (componentId) {
        target = OT.components[componentId];

        if (!target) {
            OT.warn("Could not find the component with component ID " + componentId);
        }
    }

    _exceptionHandler(target, msg, errorCode, context);
};

})(window);
(function(window) {

OT.ConnectionCapabilities = function(capabilitiesHash) {
    
    var castCapabilities = function(capabilitiesHash) {
            capabilitiesHash.supportsWebRTC = OT.$.castToBoolean(capabilitiesHash.supportsWebRTC);

            return capabilitiesHash;
        };


    
    var _caps = castCapabilities(capabilitiesHash);


    this.supportsWebRTC = _caps.supportsWebRTC;
};

})(window);
(function(window) {




















OT.Connection = function(id, creationTime, data, capabilitiesHash) {
    var destroyedReason;

    this.id = this.connectionId = id;
    this.creationTime = creationTime ? Number(creationTime) : null;
    this.data = data;
    this.capabilities = new OT.ConnectionCapabilities(capabilitiesHash);
    this.quality = null;

    OT.$.eventing(this);

    this.destroy = function(reason, quiet) {
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
    }.bind(this);

    Object.defineProperties(this, {
        destroyed: {
            get: function() { return destroyedReason !== void 0; },
            enumerable: true
        },

        destroyedReason: {
            get: function() { return destroyedReason; },
            enumerable: true
        }
    });
};

OT.Connection.fromHash = function(hash) {
  return new OT.Connection(hash.connectionId,
                            hash.creationTime,
                            hash.data,
                            { supportsWebRTC: hash.supportsWebRTC } );

};

})(window);
(function(window) {

var validPropertyNames = ['hasAudio', 'hasVideo', 'quality', 'name', 'videoDimensions', 'orientation'];















































OT.Stream = function(id, connection, name, data, type, creationTime, hasAudio, hasVideo, orientation, peerId, quality, width, height) {
  var destroyedReason;

  this.id = this.streamId = id;
  this.connection = connection;
  this.name = name;
  this.data = data;
  this.type = type || 'basic';
  this.creationTime = creationTime ? Number(creationTime) : null;
  this.hasAudio = OT.$.castToBoolean(hasAudio, true);
  this.hasVideo = OT.$.castToBoolean(hasVideo, true);
  this.peerId = peerId;
  this.quality = quality;
  this.videoDimensions = { width: (width || 640), height: (height || 480), orientation: (orientation || OT.VideoOrientation.ROTATED_NORMAL) };

  OT.$.eventing(this);

  
  
  
  
  
  
  
  
  this.update = function(key, value) {
    if (validPropertyNames.indexOf(key) === -1) {
      OT.warn('Unknown stream property "' + key + '" was modified to "' + value + '".');
      return;
    }

    var oldValue = this[key],
        newValue = value;

    switch(key) {
      case 'hasAudio':
      case 'hasVideo':
        newValue = OT.$.castToBoolean(newValue, true);
        this[key] = newValue;
        break;

      case 'quality':
      case 'name':
        this[key] = newValue;
        break;

      case 'orientation':
        this.videoDimensions = { width: newValue.width, height: newValue.height, orientation: newValue.videoOrientation };
    }

    
    
    var event = new OT.StreamUpdatedEvent(this, key, oldValue, newValue);
    this.dispatchEvent(event);
  };

  this.destroy = function(reason, quiet) {
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
  };

  Object.defineProperties(this, {
      destroyed: {
          get: function() { return destroyedReason !== void 0; },
          enumerable: true
      },

      destroyedReason: {
          get: function() { return destroyedReason; },
          enumerable: true
      }
  });
};


})(window);
(function(window) {


var NativeRTCSessionDescription = (window.mozRTCSessionDescription || window.RTCSessionDescription); 
var NativeRTCIceCandidate = (window.mozRTCIceCandidate || window.RTCIceCandidate);


var iceCandidateForwarder = function(messageDelegate) {
    return function(event) {
        OT.debug("IceCandidateForwarder: Ice Candidate");

        if (event.candidate) {
            messageDelegate(OT.Raptor.Actions.CANDIDATE, event.candidate);
        }
        else {
            OT.debug("IceCandidateForwarder: No more ICE candidates.");
        }
    };
};

















var IceCandidateProcessor = function() {
    var _pendingIceCandidates = [],
        _peerConnection = null;


    Object.defineProperty(this, 'peerConnection', {
        set: function(peerConnection) {
            _peerConnection = peerConnection;
        }
    });

    this.process = function(message) {
        var iceCandidate = new NativeRTCIceCandidate(message.candidate);

        if (_peerConnection) {
            _peerConnection.addIceCandidate(iceCandidate);
        }
        else {
            _pendingIceCandidates.push(iceCandidate);
        }
    };

    this.processPending = function() {
        while(_pendingIceCandidates.length) {
            _peerConnection.addIceCandidate(_pendingIceCandidates.shift());
        }
    };
};





var removeComfortNoise = function removeComfortNoise (sdp) {
    
    var matcher = /a=rtpmap:(\d+) CN\/\d+/i,
        payloadTypes = [],
        audioMediaLineIndex,
        sdpLines,
        match;

    
    
    
    
    
    sdpLines = sdp.split("\r\n").filter(function(line, index) {
        if (line.indexOf('m=audio') !== -1) audioMediaLineIndex = index;

        match = line.match(matcher);
        if (match !== null) {
            payloadTypes.push(match[1]);

            
            return false;
        }

        return true;
    });

    if (payloadTypes.length && audioMediaLineIndex) {
        
        sdpLines[audioMediaLineIndex] = sdpLines[audioMediaLineIndex].replace( new RegExp(payloadTypes.join('|'), 'ig') , '').replace(/\s+/g, ' ');
    }

    return sdpLines.join("\r\n");
};









var offerProcessor = function(peerConnection, offer, success, failure) {
    var generateErrorCallback = function(message) {
            return function(errorReason) {
                if (failure) failure(message, errorReason);
            };
        },

        setLocalDescription = function(answer) {
            answer.sdp = removeComfortNoise(answer.sdp);

            peerConnection.setLocalDescription(
                answer,

                
                function() {
                    success(answer);
                },

                
                generateErrorCallback('SetLocalDescription:Error while setting LocalDescription')
            );
        },

        createAnswer = function(onSuccess) {
            peerConnection.createAnswer(
                
                setLocalDescription,

                
                generateErrorCallback('CreateAnswer:Error while setting createAnswer'),

                null, 
                false 
            );
        };

    
    
    if (offer.sdp.indexOf('a=crypto') === -1) {
        var crypto_line = "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:FakeFakeFakeFakeFakeFakeFakeFakeFakeFake\\r\\n";

        
        offer.sdp = offer.sdp.replace(/^c=IN(.*)$/gmi, "c=IN$1\r\n"+crypto_line);
    }

    if (offer.sdp.indexOf('a=rtcp-fb') === -1) {
        var rtcp_fb_line = "a=rtcp-fb:* ccm fir\r\na=rtcp-fb:* nack ";

        
        offer.sdp = offer.sdp.replace(/^m=video(.*)$/gmi, "m=video$1\r\n"+rtcp_fb_line);
    }

    peerConnection.setRemoteDescription(
        offer,

        
        createAnswer,

        
        generateErrorCallback('SetRemoteDescription:Error while setting RemoteDescription')
    );

};








var suscribeProcessor = function(peerConnection, success, failure) {
    var constraints = {
            mandatory: {},
            optional: []
        },

        generateErrorCallback = function(message) {
            return function(errorReason) {
                if (failure) failure(message, errorReason);
            };
        },

        setLocalDescription = function(offer) {
            offer.sdp = removeComfortNoise(offer.sdp);

            peerConnection.setLocalDescription(
                offer,

                
                function() {
                    success(offer);
                },

                
                generateErrorCallback('SetLocalDescription:Error while setting LocalDescription')
            );
        };


    
    if (navigator.mozGetUserMedia) {
        constraints.mandatory.MozDontOfferDataChannel = true;
    }

    peerConnection.createOffer(
        
        setLocalDescription,

        
        generateErrorCallback('CreateOffer:Error while creating Offer'),

        constraints
    );
};










OT.PeerConnection = function(config) {
    var _peerConnection,
        _iceProcessor = new IceCandidateProcessor(),
        _offer,
        _answer,
        _state = 'new',
        _iceCandidatesGathered = false,
        _messageDelegates = [],
        _gettingStats,
        _createTime = OT.$.now();

    OT.$.eventing(this);

    
    
    
    if (!config.iceServers) config.iceServers = [];

    
    var delegateMessage = function(type, messagePayload) {
            if (_messageDelegates.length) {
                
                
                
                
                
                _messageDelegates[0](type, messagePayload);
            }
        }.bind(this),


        setupPeerConnection = function() {
            if (!_peerConnection) {
                try {
                    OT.debug("Creating peer connection config \"" + JSON.stringify(config) + "\".");


                    _peerConnection = OT.$.createPeerConnection(config, {
                        optional: [
                            {DtlsSrtpKeyAgreement: true}
                        ]
                    });
                } catch(e) {
                    triggerError("NewPeerConnection: " + e.message);
                    return null;
                }

                _iceProcessor.peerConnection = _peerConnection;

                _peerConnection.onicecandidate = iceCandidateForwarder(delegateMessage);
                _peerConnection.onaddstream = onRemoteStreamAdded.bind(this);
                _peerConnection.onremovestream = onRemoteStreamRemoved.bind(this);

                if (_peerConnection.onsignalingstatechange !== undefined) {
                    _peerConnection.onsignalingstatechange = routeStateChanged.bind(this);
                } else if (_peerConnection.onstatechange !== undefined) {
                    _peerConnection.onstatechange = routeStateChanged.bind(this);
                }
            }

            return _peerConnection;
        }.bind(this),

        
        
        
        tearDownPeerConnection = function() {
            
            if (_iceProcessor) _iceProcessor.peerConnection = null;

            if (_peerConnection !== null) {
                _peerConnection = null;
                this.trigger('close');
            }
        },

        routeStateChanged = function(event) {
            var newState;

            if (typeof(event) === 'string') {
                
                newState = event;
            }
            else if (event.target && event.target.signalingState) {
                
                newState = event.target.signalingState;
            }
            else {
                
                newState = event.target.readyState;
            }


            OT.debug('PeerConnection.stateChange: ' + newState);
            if (newState && newState.toLowerCase() !== _state) {
                _state = newState.toLowerCase();
                OT.debug('PeerConnection.stateChange: ' + _state);

                switch(_state) {
                    case 'closed':
                        tearDownPeerConnection.call(this);

                        break;
                    case 'failed':
                      triggerError('ICEWorkflow: Ice state failed');
                      break;
                }
            }
        },

        getLocalStreams = function() {
            var streams;

            if (_peerConnection.getLocalStreams) {
                streams = _peerConnection.getLocalStreams();
            }
            else if (_peerConnection.localStreams) {
                streams = _peerConnection.localStreams;
            }
            else {
                throw new Error("Invalid Peer Connection object implements no method for retrieving local streams");
            }

            
            
            
            return Array.prototype.slice.call(streams);
        },

        getRemoteStreams = function() {
            var streams;

            if (_peerConnection.getRemoteStreams) {
                streams = _peerConnection.getRemoteStreams();
            }
            else if (_peerConnection.remoteStreams) {
                streams = _peerConnection.remoteStreams;
            }
            else {
                throw new Error("Invalid Peer Connection object implements no method for retrieving remote streams");
            }

            
            
            
            return Array.prototype.slice.call(streams);
        },

        generateErrorCallback = function(forMethod, message) {
            return function(errorReason) {
                triggerError.call(this, "PeerConnection." + forMethod + ": " + message + ": " + errorReason);
            }.bind(this);
        },

        
        onRemoteStreamAdded = function(event) {
            this.trigger('streamAdded', event.stream);
        },

        onRemoteStreamRemoved = function(event) {
            this.trigger('streamRemoved', event.stream);
        },

        


        
        
        relaySDP = function(messageType, sdp) {
            delegateMessage(messageType, sdp);
        },

        
        processOffer = function(message) {
            var offer = new NativeRTCSessionDescription(message.sdp),

                
                relayAnswer = function(answer) {
                    relaySDP(OT.Raptor.Actions.ANSWER, answer);
                },

                reportError = function(message, errorReason) {
                    triggerError(message + ":" + errorReason + ":PeerConnection.offerProcessor");
                };

            setupPeerConnection();

            _remoteDescriptionType = offer.type;
            _remoteDescription = offer;

            offerProcessor(
                _peerConnection,
                offer,
                relayAnswer,
                reportError
            );
        },

        processAnswer = function(message) {
            if (!message.sdp) {
                OT.error("PeerConnection.processMessage: Weird message, no SDP.");
                return;
            }

            _answer = new NativeRTCSessionDescription(message.sdp);

            _remoteDescriptionType = _answer.type;
            _remoteDescription = _answer;

            _peerConnection.setRemoteDescription(_answer, function () {
              OT.debug("setRemoteDescription succeeded");
            }, function (errReason) {
              triggerError("SetRemoteDescription:Error while setting RemoteDescription: " + errReason);
            });
            _iceProcessor.processPending();
        },

        processSubscribe = function(message) {
            OT.debug("PeerConnection.processSubscribe: Sending offer to subscriber.");

            setupPeerConnection();

            suscribeProcessor(
                _peerConnection,

                
                function(offer) {
                    _offer = offer;
                    relaySDP(OT.Raptor.Actions.OFFER, _offer);
                },

                
                function(message, errorReason) {
                    triggerError(message + ":" + errorReason + ": PeerConnection.suscribeProcessor");
                }
            );
        },

        triggerError = function(errorReason) {
            OT.error(errorReason);
            this.trigger('error', errorReason);
        }.bind(this);

    this.addLocalStream = function(webRTCStream) {
        setupPeerConnection();
        _peerConnection.addStream(webRTCStream);
    };

    this.disconnect = function() {
        _iceProcessor = null;

        if (_peerConnection) {
            var currentState = (_peerConnection.signalingState || _peerConnection.readyState);
            if (currentState && currentState.toLowerCase() !== 'closed') _peerConnection.close();

            
            
            
            tearDownPeerConnection.call(this);
        }

        this.off();
    };

    this.processMessage = function(type, message) {
        OT.debug("PeerConnection.processMessage: Received " + type + " from " + message.fromAddress);
        OT.debug(message);

        switch(type) {
            case OT.Raptor.Actions.SUBSCRIBE:
                processSubscribe.call(this, message);
                break;

            case OT.Raptor.Actions.OFFER:
                processOffer.call(this, message);
                break;

            case OT.Raptor.Actions.ANSWER:
            
                processAnswer.call(this, message);
                break;

            case OT.Raptor.Actions.CANDIDATE:
                _iceProcessor.process(message);
                break;

            default:
                OT.debug("PeerConnection.processMessage: Received an unexpected message of type " + type + " from " + message.fromAddress + ": " + JSON.stringify(message));
        }

        return this;
    };

    this.registerMessageDelegate = function(delegateFn) {
        return _messageDelegates.push(delegateFn);
    };

    this.unregisterMessageDelegate = function(delegateFn) {
        var index = _messageDelegates.indexOf(delegateFn);

        if ( index !== -1 ) {
            _messageDelegates.splice(index, 1);
        }
        return _messageDelegates.length;
    };

    











    this.getStats = function(prevStats, callback) {
        
        if (_gettingStats == true) {
            OT.warn("PeerConnection.getStats: Already getting the stats!");
            return;
        }

        
        _gettingStats = true;

        
        var now = OT.$.now();
        var time_difference = (now - prevStats["timeStamp"]) / 1000; 

        
        prevStats["timeStamp"] = now;

        
        var parseAvgVideoBitrate = function(result) {
            var last_bytesSent = prevStats["videoBytesTransferred"] || 0;

            if (result.stat("googFrameHeightSent")) {
                prevStats["videoBytesTransferred"] = result.stat("bytesSent");
                return Math.round((prevStats["videoBytesTransferred"] - last_bytesSent) * 8 / time_difference);
            } else if (result.stat("googFrameHeightReceived")) {
                prevStats["videoBytesTransferred"] = result.stat("bytesReceived");
                return Math.round((prevStats["videoBytesTransferred"] - last_bytesSent) * 8 / time_difference);
            } else {
                return NaN;
            }
        };

        
        var parseAvgAudioBitrate = function(result) {
            var last_bytesSent = prevStats["audioBytesTransferred"] || 0;

            if (result.stat("audioInputLevel")) {
                prevStats["audioBytesTransferred"] = result.stat("bytesSent");
                return Math.round((prevStats["audioBytesTransferred"] - last_bytesSent) * 8 / time_difference);
            } else if (result.stat("audioOutputLevel")) {
                prevStats["audioBytesTransferred"] = result.stat("bytesReceived");
                return Math.round((prevStats["audioBytesTransferred"] - last_bytesSent) * 8 / time_difference);
            } else {
                return NaN;
            }
        };

        var parsed_stats = {};
        var parseStatsReports = function(stats) {
            if (stats.result) {
                var result_list = stats.result();
                for (var result_index = 0; result_index < result_list.length; result_index++) {
                    var report = {};
                    var result = result_list[result_index];
                    if (result.stat) {

                        if(result.stat("googActiveConnection") === 'true') {
                            parsed_stats.localCandidateType = result.stat('googLocalCandidateType');
                            parsed_stats.remoteCandidateType = result.stat('googRemoteCandidateType');
                            parsed_stats.transportType = result.stat('googTransportType');
                        }

                        var avgVideoBitrate = parseAvgVideoBitrate(result);
                        if (!isNaN(avgVideoBitrate)) {
                            parsed_stats["avgVideoBitrate"] = avgVideoBitrate;
                        }

                        var avgAudioBitrate = parseAvgAudioBitrate(result);
                        if (!isNaN(avgAudioBitrate)) {
                            parsed_stats["avgAudioBitrate"] = avgAudioBitrate;
                        }
                    }
                }
            }

            _gettingStats = false;
            callback(parsed_stats);
        }
        parsed_stats["duration"] = Math.round(now - _createTime);

        var parseStats = function(stats) {
            for (var key in stats) {
                if (stats.hasOwnProperty(key) &&
                    (stats[key].type === 'outboundrtp' || stats[key].type === 'inboundrtp')) {
                    var res = stats[key];
                    
                    if (res.id.indexOf('video') !== -1) {
                        var avgVideoBitrate = parseAvgVideoBitrate(res);
                        if(!isNaN(avgVideoBitrate)) {
                            parsed_stats.avgVideoBitrate = avgVideoBitrate;
                        }

                    } else if (res.id.indexOf('audio') !== -1) {
                        var avgAudioBitrate = parseAvgAudioBitrate(res);
                        if(!isNaN(avgAudioBitrate)) {
                            parsed_stats.avgAudioBitrate = avgAudioBitrate;
                        }
                    }
                }
            }

            _gettingStats = false;
            callback(parsed_stats);
        }


        var needsNewGetStats = function() {
            var firefoxVersion = window.navigator.userAgent.toLowerCase()
            .match(/Firefox\/([0-9\.]+)/i);
            var needs = (firefoxVersion !== null && parseFloat(firefoxVersion[1], 10) >= 27.0);
            needsNewGetStats = function() { return needs; };
            return needs;
        };

        if (_peerConnection && _peerConnection.getStats) {
            if(needsNewGetStats()) {
                _peerConnection.getStats(null, parseStats, function(err) {
                    OT.warn('Error collecting stats', err);
                    _gettingStats = false;
                });
            } else {
                _peerConnection.getStats(parseStatsReports);
            }
        } else {
            
            _gettingStats = false;
            callback(parsed_stats);
        }
    };

    Object.defineProperty(this, 'remoteStreams', {
        get: function() {
            return _peerConnection ? getRemoteStreams() : [];
        }
    });
};

})(window);
(function(window) {

var _peerConnections = {};

OT.PeerConnections = {
    add: function(remoteConnection, stream, config) {
        var key = remoteConnection.id + "_" + stream.id,
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

    remove: function(remoteConnection, stream) {
        var key = remoteConnection.id + "_" + stream.id,
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


})(window);
(function(window) {

















OT.PublisherPeerConnection = function(remoteConnection, session, stream, webRTCStream) {
    var _peerConnection,
        _hasRelayCandidates = false;

    
    var _onPeerClosed = function() {
            this.destroy();
            this.trigger('disconnected', this);
        },

        
        _onPeerError = function(errorReason) {
            this.trigger('error', null, errorReason, this);
            this.destroy();
        },

        _relayMessageToPeer = function(type, payload) {
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
                session._.jsepAnswer(remoteConnection.id, stream, payload);
                break;

            case OT.Raptor.Actions.OFFER:
                this.trigger('connected');
                session._.jsepOffer(remoteConnection.id, stream, payload);
                break;

            case OT.Raptor.Actions.CANDIDATE:
                session._.jsepCandidate(remoteConnection.id, stream, payload);
            }
        }.bind(this);


    OT.$.eventing(this);

    
    this.destroy = function() {
        
        if (_peerConnection) {
            OT.PeerConnections.remove(remoteConnection, stream);
        }

        _peerConnection.off();
        _peerConnection = null;
    };

    this.processMessage = function(type, message) {
        _peerConnection.processMessage(type, message);
    };

    this.getStats = function(prevStats, callback) {
        _peerConnection.getStats(prevStats, callback);
    }

    
    this.init = function() {
        var iceServers = session.sessionInfo.iceServers.map(function(is) {
            var iceServer = OT.$.clone(is);

            if (iceServer.url.trim().substr(0, 5) === 'turn:') {
                
                iceServer.username = session.id + '.' + session.connection.id + '.' + stream.id;
            }

            return iceServer;
        });

        _peerConnection = OT.PeerConnections.add(remoteConnection, stream, {
            iceServers: iceServers
        });

        _peerConnection.on({
            close: _onPeerClosed,
            error: _onPeerError
        }, this);

        _peerConnection.registerMessageDelegate(_relayMessageToPeer);
        _peerConnection.addLocalStream(webRTCStream);

        Object.defineProperty(this, 'remoteConnection', {value: remoteConnection});

        Object.defineProperty(this, 'hasRelayCandidates', {
            get: function() { return _hasRelayCandidates; }
        });
    }
};

})(window);
(function(window) {
























OT.SubscriberPeerConnection = function(remoteConnection, session, stream, properties) {
    var _peerConnection,
        _hasRelayCandidates = false;

    
    var _onPeerClosed = function() {
            this.destroy();
            this.trigger('disconnected', this);
        },

        _onRemoteStreamAdded = function(remoteRTCStream) {
            this.trigger('remoteStreamAdded', remoteRTCStream, this);
        },

        _onRemoteStreamRemoved = function(remoteRTCStream) {
            this.trigger('remoteStreamRemoved', remoteRTCStream, this);
        },

        
        _onPeerError = function(errorReason) {
            this.trigger('error', null, errorReason, this);
        },

        _relayMessageToPeer = function(type, payload) {
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
                session._.jsepAnswer(remoteConnection.id, stream, payload);
                break;

            case OT.Raptor.Actions.OFFER:
                session._.jsepOffer(remoteConnection.id, stream, payload);
                break;

            case OT.Raptor.Actions.CANDIDATE:
                session._.jsepCandidate(remoteConnection.id, stream, payload);
            }
        }.bind(this),

        
        _setEnabledOnStreamTracksCurry = function(isVideo) {
            var method = 'get' + (isVideo ? 'Video' : 'Audio') + 'Tracks';

            return function(enabled) {
                var remoteStreams = _peerConnection.remoteStreams,
                    tracks,
                    stream;

                if (remoteStreams.length === 0 || !remoteStreams[0][method]) {
                    
                    
                    return;
                }

                for (var i=0, num=remoteStreams.length; i<num; ++i) {
                    stream = remoteStreams[i];
                    tracks = stream[method]();

                    for (var k=0, numTracks=tracks.length; k < numTracks; ++k){
                        tracks[k].enabled=enabled;
                    }
                }
            };
        };


    OT.$.eventing(this);

    
    this.destroy = function() {
      if (_peerConnection) {
        var numDelegates = _peerConnection.unregisterMessageDelegate(_relayMessageToPeer);

        
        if (numDelegates === 0) {
          
          if (session && session.connected && stream && !stream.destroyed) {
              
              session._.jsepUnsubscribe(stream);
          }

          
          this.subscribeToAudio(false);
        }
        OT.PeerConnections.remove(remoteConnection, stream.streamId);
      }
      _peerConnection = null;
      this.off();
    };

    this.processMessage = function(type, message) {
        _peerConnection.processMessage(type, message);
    };

    this.getStats = function(prevStats, callback) {
        _peerConnection.getStats(prevStats, callback);
    };

    this.subscribeToAudio = _setEnabledOnStreamTracksCurry(false);
    this.subscribeToVideo = _setEnabledOnStreamTracksCurry(true);

    Object.defineProperty(this, 'hasRelayCandidates', {
        get: function() { return _hasRelayCandidates; }
    });


    
    this.init = function() {
        var iceServers = session.sessionInfo.iceServers.map(function(is) {
            var iceServer = OT.$.clone(is);

            if (iceServer.url.trim().substr(0, 5) === 'turn:') {
                
                iceServer.username = session.id + '.' + session.connection.id + '.' + stream.id;
            }

            return iceServer;
        });

        _peerConnection = OT.PeerConnections.add(remoteConnection, stream.streamId, {
            iceServers: iceServers
        });

        _peerConnection.on({
            close: _onPeerClosed,
            streamAdded: _onRemoteStreamAdded,
            streamRemoved: _onRemoteStreamRemoved,
            error: _onPeerError
        }, this);

        var numDelegates = _peerConnection.registerMessageDelegate(_relayMessageToPeer);

        
        if (_peerConnection.remoteStreams.length > 0) {
            _peerConnection.remoteStreams.forEach(_onRemoteStreamAdded, this);
        }
        else if (numDelegates === 1) {
            
            session._.jsepSubscribe(stream, properties.subscribeToVideo, properties.subscribeToAudio);
        }
    };
};

})(window);
(function(window) {


OT.Chrome = function(properties) {
    var _visible = false,
        _widgets = {},

        
        _set = function(name, widget) {
            widget.parent = this;
            widget.appendTo(properties.parent);

            _widgets[name] = widget;

            Object.defineProperty(this, name, {
                get: function() { return _widgets[name]; }
            });
        };

    if (!properties.parent) {
        
        return;
    }

    OT.$.eventing(this);

    this.destroy = function() {
        this.off();
        this.hide();

        for (var name in _widgets) {
            _widgets[name].destroy();
        }
    };

    this.show = function() {
        _visible = true;

        for (var name in _widgets) {
            _widgets[name].show();
        }
    };

    this.hide = function() {
        _visible = false;

        for (var name in _widgets) {
            _widgets[name].hide();
        }
    };


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    this.set = function(widgetName, widget) {
        if (typeof(widgetName) === "string" && widget) {
            _set.call(this, widgetName, widget);
        }
        else {
          for (var name in widgetName) {
            if (widgetName.hasOwnProperty(name)) {
              _set.call(this, name, widgetName[name]);
            }
          }
        }

        return this;
    };
};

})(window);
(function(window) {

if (!OT.Chrome.Behaviour) OT.Chrome.Behaviour = {};





OT.Chrome.Behaviour.Widget = function(widget, options) {
    var _options = options || {},
        _mode,
        _previousMode;

    
    
    
    
    widget.setDisplayMode = function(mode) {
        var newMode = mode || 'auto';
        if (_mode === newMode) return;

        OT.$.removeClass(this.domElement, 'OT_mode-' + _mode);
        OT.$.addClass(this.domElement, 'OT_mode-' + newMode);

        _previousMode = _mode;
        _mode = newMode;
    };

    widget.show = function() {
        this.setDisplayMode(_previousMode);
        if (_options.onShow) _options.onShow();

        return this;
    };

    widget.hide = function() {
        this.setDisplayMode('off');
        if (_options.onHide) _options.onHide();

        return this;
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

        
        if (_options.mode != "auto") {
            widget.setDisplayMode(_options.mode);
        } else {
            
            
            widget.setDisplayMode('on');
            setTimeout(function() {
                widget.setDisplayMode(_options.mode);
            }, 2000);
        }

        
        parent.appendChild(this.domElement);

        return widget;
    };
};

})(window);
(function(window) {











OT.Chrome.NamePanel = function(options) {
    var _name = options.name;

    if (!_name || _name.trim().length === '') {
        _name = null;

        
        options.mode = 'off';
    }

    
    
    var _domElement;
    Object.defineProperty(this, 'domElement', {
        get: function() { return _domElement; },
        set: function(domElement) { _domElement = domElement; }
    })

    
    OT.Chrome.Behaviour.Widget(this, {
        mode: options.mode,
        nodeName: 'h1',
        htmlContent: _name,
        htmlAttributes: {className: 'OT_name'}
    });

    Object.defineProperty(this, 'name', {
        set: function(name) {
            if (!_name) this.setDisplayMode('auto');

            _name = name;
            _domElement.innerHTML = _name;
        }.bind(this)
    });
};

})(window);
(function(window) {

OT.Chrome.MuteButton = function(options) {
    var _onClickCb,
        _muted = options.muted || false;

    
    
    var _domElement;
    Object.defineProperty(this, 'domElement', {
        get: function() { return _domElement; },
        set: function(domElement) { _domElement = domElement; }
    })

    
    var attachEvents = function(elem) {
            _onClickCb = onClick.bind(this);
            elem.addEventListener('click', _onClickCb, false);
        },

        detachEvents = function(elem) {
            _onClickCb = null;
            elem.removeEventListener('click', _onClickCb, false);
        },

        onClick = function(event) {
            _muted = !_muted;

            if (_muted) {
                OT.$.addClass(_domElement, 'OT_active');
                this.parent.trigger('muted', this);
            }
            else {
                OT.$.removeClass(_domElement, 'OT_active');
                this.parent.trigger('unmuted', this);
            }

            return false;
        };

    
    var classNames = _muted ? 'OT_mute OT_active' : 'OT_mute';
    OT.Chrome.Behaviour.Widget(this, {
        mode: options.mode,
        nodeName: 'button',
        htmlContent: 'Mute',
        htmlAttributes: {className: classNames},
        onCreate: attachEvents.bind(this),
        onDestroy: detachEvents.bind(this)
    });
};


})(window);
(function(window) {

OT.Chrome.MicVolume = function(options) {
    var _onClickCb,
        _muted = options.muted || false;

    
    
    var _domElement;
    Object.defineProperty(this, 'domElement', {
        get: function() { return _domElement; },
        set: function(domElement) { _domElement = domElement; }
    })

    
    var attachEvents = function(elem) {
            _onClickCb = onClick.bind(this);
            elem.addEventListener('click', _onClickCb, false);
        },

        detachEvents = function(elem) {
            _onClickCb = null;
            elem.removeEventListener('click', _onClickCb, false);
        },

        onClick = function(event) {
            _muted = !_muted;

            if (_muted) {
                OT.$.addClass(_domElement, 'active');
                this.parent.trigger('muted', this);
            }
            else {
                OT.$.removeClass(_domElement, 'active');
                this.parent.trigger('unmuted', this);
            }

            return false;
        };

    
    OT.Chrome.Behaviour.Widget(this, {
        mode: options.mode,
        nodeName: 'button',
        htmlContent: 'Mute',
        htmlAttributes: {className: 'OT_mic-volume'},
        onCreate: attachEvents.bind(this),
        onDestroy: detachEvents.bind(this)
    });
};


})(window);
(function(window) {

OT.Chrome.SettingsPanelButton = function(options) {
    var _onClickCb;

    
    var attachEvents = function(elem) {
            _onClickCb = onClick.bind(this);
            elem.addEventListener('click', _onClickCb, false);
        },

        detachEvents = function(elem) {
            _onClickCb = null;
            elem.removeEventListener('click', _onClickCb, false);
        },

        onClick = function(event) {
            this.parent.trigger('SettingsPanel:open', this);
            return false;
        };


    
    
    var _domElement;
    Object.defineProperty(this, 'domElement', {
        get: function() { return _domElement; },
        set: function(domElement) { _domElement = domElement; }
    })

    
    OT.Chrome.Behaviour.Widget(this, {
        mode: options.mode,
        nodeName: 'button',
        htmlContent: 'Settings',
        htmlAttributes: {className: 'OT_settings-panel'},
        onCreate: attachEvents.bind(this),
        onDestroy: detachEvents.bind(this)
    });
};

})(window);
(function(window) {

OT.Chrome.SettingsPanel = function(options) {
    if (!options.stream) {
        
        return;
    }

    var webRTCStream = options.stream;

    
    
    var _domElement;
    Object.defineProperty(this, 'domElement', {
        get: function() { return _domElement; },
        set: function(domElement) { _domElement = domElement; }
    })

    var renderDialog = function() {
            var camLabel = webRTCStream.getVideoTracks().length ? webRTCStream.getVideoTracks()[0].label : "None",
                micLabel = webRTCStream.getAudioTracks().length ? webRTCStream.getAudioTracks()[0].label : "None";

            _domElement.innerHTML = "<dl>\
                                        <dt>Cam</dt>\
                                        <dd>" + camLabel + "</dd>\
                                        <dt>Mic</dt>\
                                        <dd>" + micLabel + "</dd>\
                                    </dl>";


            var closeButton  = OT.$.createButton('Close', {
                className: 'OT_close'
            }, {
                click: onClose.bind(this)
            });

            _domElement.appendChild(closeButton);
        },

        onShow = function() {
            renderDialog.call(this);
        },

        onClose = function() {
            this.parent.trigger('SettingsPanel:close', this);
            return false;
        };


    
    OT.Chrome.Behaviour.Widget(this, {
        mode: options.mode,
        nodeName: 'section',
        htmlContent: 'Settings',
        htmlAttributes: {className: 'OT_settings-panel'},
        onCreate: renderDialog.bind(this),
        onShow: onShow.bind(this)
    });
};

})(window);
(function(window) {

OT.Chrome.OpenTokButton = function(options) {
    
    
    var _domElement;
    this.__defineGetter__("domElement", function() { return _domElement; });
    this.__defineSetter__("domElement", function(domElement) { _domElement = domElement; });

    
    OT.Chrome.Behaviour.Widget(this, {
        mode: options ? options.mode : null,
        nodeName: 'span',
        htmlContent: 'OpenTok',
        htmlAttributes: {
            className: 'OT_opentok'
        }
    });
};

})(window);
(function(window) {





























OT.StylableComponent = function(self, initalStyles) {
    if (!self.trigger) {
        throw new Error("OT.StylableComponent is dependent on the mixin OT.$.eventing. Ensure that this is included in the object before StylableComponent.");
    }

    
    var onStyleChange = function(key, value, oldValue) {
        if (oldValue) {
            self.trigger('styleValueChanged', key, value, oldValue);
        }
        else {
            self.trigger('styleValueChanged', key, value);
        }
    };

    var _style = new Style(initalStyles, onStyleChange);

    










	









    
    self.getStyle = function(key) {
        return _style.get(key);
    };

    









































    







































    self.setStyle = function(keyOrStyleHash, value, silent) {
        if (typeof(keyOrStyleHash) !== 'string') {
            _style.setAll(keyOrStyleHash, silent);
        }
        else {
            _style.set(keyOrStyleHash, value);
        }

        return this;
    };
};

var Style = function(initalStyles, onStyleChange) {
    var _COMPONENT_STYLES = [
            "showMicButton",
            "showSpeakerButton",
            "showSettingsButton",
            "showCameraToggleButton",
            "nameDisplayMode",
            "buttonDisplayMode",
            "showSaveButton",
            "showRecordButton",
            "showRecordStopButton",
            "showReRecordButton",
            "showPauseButton",
            "showPlayButton",
            "showPlayStopButton",
            "showStopButton",
            "backgroundImageURI",
            "showControlPanel",
            "showRecordCounter",
            "showPlayCounter",
            "showControlBar",
            "showPreviewTime"
        ],

        _validStyleValues = {
            buttonDisplayMode: ["auto", "off", "on"],
            nameDisplayMode: ["auto", "off", "on"],
            showSettingsButton: [true, false],
            showMicButton: [true, false],
            showCameraToggleButton: [true, false],
            showSaveButton: [true, false],
            backgroundImageURI: null,
            showControlBar: [true, false],
            showPlayCounter: [true, false],
            showRecordCounter: [true, false],
            showPreviewTime: [true, false]
        },

        _style = {},


        
        isValidStyle = function(key, value) {
            return key === 'backgroundImageURI' ||
                    (   _validStyleValues.hasOwnProperty(key) &&
                        _validStyleValues[key].indexOf(value) !== -1 );
        },

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

        for (var i in style) {
            if (_COMPONENT_STYLES.indexOf(i) < 0) {
                
                delete style[i];
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
            newValue = castValue(newStyles[key]);

            if (isValidStyle(key, newValue)) {
                oldValue = _style[key];

                if (newValue !== oldValue) {
                    _style[key] = newValue;
                    if (!silent) onStyleChange(key, newValue, oldValue);
                }
            }
            else {
                OT.warn("Style.setAll::Invalid style property passed " + key + " : " + newValue);
            }
        }

        return this;
    };

    this.set = function(key, value) {
        OT.debug("Publisher.setStyle: " + key.toString());

        var newValue = castValue(value),
            oldValue;

        if (!isValidStyle(key, newValue)) {
            OT.warn("Style.set::Invalid style property passed " + key + " : " + newValue);
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

})(window);
(function(window) {







OT.Microphone = function(webRTCStream, muted) {
    var _muted,
        _gain = 50;


    Object.defineProperty(this, 'muted', {
        get: function() { return _muted; },
        set: function(muted) {
            if (_muted === muted) return;

            _muted = muted;

            var audioTracks = webRTCStream.getAudioTracks();

            for (var i=0, num=audioTracks.length; i<num; ++i) {
                audioTracks[i].enabled = !_muted;
            }
        }
    });

    Object.defineProperty(this, 'gain', {
        get: function() { return _gain; },

        set: function(gain) {
            OT.warn("OT.Microphone.gain IS NOT YET IMPLEMENTED");

            _gain = gain;
        }
    });

    
    if (muted !== undefined) {
        this.muted = muted === true;
    }
    else if (webRTCStream.getAudioTracks().length) {
        this.muted = !webRTCStream.getAudioTracks()[0].enabled;
    }
    else {
        this.muted = false;
    }
};

})(window);
(function(window) {














OT.generateSimpleStateMachine = function(initialState, states, transitions) {
  var validStates = states.slice(),
      validTransitions = OT.$.clone(transitions);

  var isValidState = function (state) {
    return validStates.indexOf(state) !== -1;
  }

  var isValidTransition = function(fromState, toState) {
    return validTransitions[fromState] && validTransitions[fromState].indexOf(toState) !== -1;
  };

  return function(stateChangeFailed) {
    var currentState = initialState,
        previousState = null;

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
        signalChangeFailed("'" + newState + "' is not a valid state", newState)

        return false;
      }

      if (!isValidTransition(currentState, newState)) {
        signalChangeFailed("'" + currentState + "' cannot transition to '" + newState + "'", newState)

        return false;
      }

      return true;
    }


    this.set = function(newState) {
      if (!handleInvalidStateChanges(newState)) return;

      previousState = currentState;
      currentState = newState;
    };

    Object.defineProperties(this, {
      current: {
        get: function() { return currentState; }
      },

      subscribing: {
        get: function() { return currentState === 'Subscribing'; }
      }
    });
  };
};

})(window);
(function(window) {



















































var validStates = [ 'NotSubscribing', 'Init', 'ConnectingToPeer', 'BindingRemoteStream', 'Subscribing', 'Failed' ],

    validTransitions = {
      NotSubscribing: ['NotSubscribing', 'Init'],
      Init: ['NotSubscribing', 'ConnectingToPeer', 'BindingRemoteStream'],
      ConnectingToPeer: ['NotSubscribing', 'BindingRemoteStream', 'Failed'],
      BindingRemoteStream: ['NotSubscribing', 'Subscribing', 'Failed'],
      Subscribing: ['NotSubscribing', 'Failed'],
      Failed: []
    },

    initialState = 'NotSubscribing';

OT.SubscribingState = OT.generateSimpleStateMachine(initialState, validStates, validTransitions);

Object.defineProperty(OT.SubscribingState.prototype, 'attemptingToSubscribe', {
  get: function() { return [ 'Init', 'ConnectingToPeer', 'BindingRemoteStream' ].indexOf(this.current) !== -1; }
});

})(window);
(function(window) {

















































var validStates = [ 'NotPublishing', 'GetUserMedia', 'BindingMedia', 'MediaBound', 'PublishingToSession', 'Publishing', 'Failed' ],

    validTransitions = {
      NotPublishing: ['NotPublishing', 'GetUserMedia'],
      GetUserMedia: ['BindingMedia', 'Failed', 'NotPublishing'],
      BindingMedia: ['MediaBound', 'Failed', 'NotPublishing'],
      MediaBound: ['NotPublishing', 'PublishingToSession', 'Failed'],
      PublishingToSession: ['NotPublishing', 'Publishing', 'Failed'],
      Publishing: ['NotPublishing', 'MediaBound', 'Failed'],
      Failed: []
    },

    initialState = 'NotPublishing';

OT.PublishingState = OT.generateSimpleStateMachine(initialState, validStates, validTransitions);

Object.defineProperties(OT.PublishingState.prototype, {
  attemptingToPublish: {
    get: function() { return [ 'GetUserMedia', 'BindingMedia', 'MediaBound', 'PublishingToSession' ].indexOf(this.current) !== -1; }
  },

  publishing: {
    get: function() { return this.current === 'Publishing'; }
  }
});


})(window);
(function(window) {


var defaultConstraints = {
    audio: true,
    video: true
};
















































OT.Publisher = function() {
    
    
    if (!OT.checkSystemRequirements()) {
        OT.upgradeSystemRequirements();
        return;
    }

    var _guid = OT.Publisher.nextId(),
        _domId,
        _container,
        _targetElement,
        _stream,
        _webRTCStream,
        _session,
        _peerConnections = {},
        _loaded = false,
        _publishProperties,
        _publishStartTime,
        _streamCreatedTimeout,
        _microphone,
        _chrome,
        _analytics = new OT.Analytics(),
        _validResolutions = [
            {width: 320, height: 240},
            {width: 640, height: 480},
            {width: 1280, height: 720}
        ],
        _qosIntervals = {},
        _gettingStats = 0,
        _prevStats = {
            "timeStamp" : OT.$.now()
        },
        _state;

    OT.$.eventing(this);

    OT.StylableComponent(this, {
        showMicButton: true,
        showSettingsButton: true,
        showCameraToggleButton: true,
        nameDisplayMode: "auto",
        buttonDisplayMode: "auto",
        backgroundImageURI: null
    });

        
    var logAnalyticsEvent = function(action, variation, payloadType, payload) {
            _analytics.logEvent({
                action: action,
                variation: variation,
                payload_type: payloadType,
                payload: payload,
                session_id: _session ? _session.sessionId : null,
                connection_id: _session && _session.connected ? _session.connection.connectionId : null,
                partner_id: _session ? _session.apiKey : OT.APIKEY,
                streamId: _stream ? _stream.id : null,
                widget_id: _guid,
                widget_type: 'Publisher'
            });
        },

        isValidResolution = function(width, height) {
            for (var i=0; i<_validResolutions.length; ++i) {
                if (_validResolutions[i].width == width && _validResolutions[i].height == height) {
                    return true;
                }
            }
            return false;
        },

        recordQOS = function(connection_id) {
            var QoS_blob = {
                widget_type: 'Publisher',
                stream_type : 'WebRTC',
                sessionId: _session ? _session.sessionId : null,
                connectionId: _session && _session.connected ? _session.connection.connectionId : null,
                partnerId: _session ? _session.apiKey : OT.APIKEY,
                streamId: _stream ? _stream.id : null,
                widgetId: _guid,
                version: OT.properties.version,
                media_server_name: _session ? _session.sessionInfo.messagingServer : null,
                p2pFlag: _session ? _session.sessionInfo.p2pEnabled : false,
                duration: new Date().getTime() -_publishStartTime.getTime(),
                remote_connection_id: connection_id
            };

            
            _peerConnections[connection_id].getStats(_prevStats, function(stats) {
                if (stats) {
                    for (var stat_index in stats) {
                        QoS_blob[stat_index] = stats[stat_index];
                    }
                }
                _analytics.logQOS(QoS_blob);
            });
        },

        

        stateChangeFailed = function(changeFailed) {
            OT.error("Publisher State Change Failed: ", changeFailed.message);
            OT.debug(changeFailed);
        },

        onLoaded = function() {
            OT.debug("OT.Publisher.onLoaded");

            _state.set('MediaBound');
            _container.loading = false;
            _loaded = true;

            _createChrome.call(this);

            this.trigger('initSuccess', this);
            this.trigger('loaded', this);
        },

        onLoadFailure = function(reason) {
            logAnalyticsEvent('publish', 'Failure', 'reason', "Publisher PeerConnection Error: " + reason);

            _state.set('Failed');
            this.trigger('publishError', "Publisher PeerConnection Error: " + reason);

            OT.handleJsException("Publisher PeerConnection Error: " + reason, OT.ExceptionCodes.P2P_CONNECTION_FAILED, {
                session: _session,
                target: this
            });
        },

        onStreamAvailable = function(webOTStream) {
            OT.debug("OT.Publisher.onStreamAvailable");

            _state.set('BindingMedia');

            cleanupLocalStream();
            _webRTCStream = webOTStream;

            _microphone = new OT.Microphone(_webRTCStream, !_publishProperties.publishAudio);
            this.publishVideo(_publishProperties.publishVideo);

            this.dispatchEvent(
                new OT.Event(OT.Event.names.ACCESS_ALLOWED, false)
            );

            _targetElement = new OT.VideoElement({
                attributes: {muted:true}
            });

            _targetElement.on({
                    streamBound: onLoaded,
                    loadError: onLoadFailure,
                    error: onVideoError
                }, this)
                .bindToStream(_webRTCStream);

            _container.video = _targetElement;
        },

        onStreamAvailableError = function(error) {
            OT.error('OT.Publisher.onStreamAvailableError ' + error.name + ': ' + error.message);

            _state.set('Failed');
            this.trigger('publishError', error.message);

            if (_container) _container.destroy();

            logAnalyticsEvent('publish', 'Failure', 'reason', "Publisher failed to access camera/mic: " + error.message);

            OT.handleJsException("Publisher failed to access camera/mic: " + error.message, 2000, {
                session: _session,
                target: this
            });
        },

        
        onAccessDenied = function(error) {
            OT.error('OT.Publisher.onStreamAvailableError Permission Denied');

            _state.set('Failed');
            this.trigger('publishError', error.message);

            logAnalyticsEvent('publish', 'Failure', 'reason', 'Publisher Access Denied: Permission Denied');

            var event = new OT.Event(OT.Event.names.ACCESS_DENIED),
                defaultAction = function() {
                    if (!event.isDefaultPrevented() && _container) _container.destroy();
                };

            this.dispatchEvent(event, defaultAction);
        },

        onAccessDialogOpened = function() {
            logAnalyticsEvent('accessDialog', 'Opened', '', '');

            this.dispatchEvent(
                new OT.Event(OT.Event.names.ACCESS_DIALOG_OPENED, false)
            );
        },

        onAccessDialogClosed = function() {
            logAnalyticsEvent('accessDialog', 'Closed', '', '');

            this.dispatchEvent(
                new OT.Event(OT.Event.names.ACCESS_DIALOG_CLOSED, false)
            );
        },

        onVideoError = function(errorCode, errorReason) {
            OT.error('OT.Publisher.onVideoError');

            var message = errorReason + (errorCode ? ' (' + errorCode + ')' : '');
            logAnalyticsEvent('stream', null, 'reason', "Publisher while playing stream: " + message);

            _state.set('Failed');

            if (_state.attemptingToPublish) {
                this.trigger('publishError', message);
            }
            else {
                this.trigger('error', message);
            }

            OT.handleJsException("Publisher error playing stream: " + message, 2000, {
                session: _session,
                target: this
            });
        },

        onPeerDisconnected = function(peerConnection) {
            OT.debug("OT.Subscriber has been disconnected from the Publisher's PeerConnection");

            this.cleanupSubscriber(peerConnection.remoteConnection.id);
        },

        onPeerConnectionFailure = function(code, reason, peerConnection) {
            logAnalyticsEvent('publish', 'Failure', 'reason|hasRelayCandidates', [
                reason + ": Publisher PeerConnection with connection " + (peerConnection && peerConnection.remoteConnection && peerConnection.remoteConnection.id)  + " failed",
                peerConnection.hasRelayCandidates
                ].join('|'));

            OT.handleJsException("Publisher PeerConnection Error: " + reason, 2000, {
                session: _session,
                target: this
            });

            
            
            
            
            if (peerConnection.remoteConnection) {
                clearInterval(_qosIntervals[peerConnection.remoteConnection.id]);
                delete _qosIntervals[peerConnection.remoteConnection.id]

                delete _peerConnections[peerConnection.remoteConnection.id];
            }
            peerConnection.off();
        },

        getStats = function(callback) {
            if (_gettingStats > 0) {
                OT.debug("Still getting stats");
                return;
            }

            var getStatsBlob = {};

            
            for (var conn_id in _peerConnections) {

                
                
                _gettingStats++;

                getStatsBlob[conn_id] = null;

                
                (function(connection_id) {
                    _peerConnections[connection_id].getStats(function(parsed_stats) {

                        
                        _gettingStats--;

                        if (parsed_stats) {
                            getStatsBlob[connection_id] = parsed_stats;
                        }

                        
                        
                        if (_gettingStats == 0) {
                            callback(getStatsBlob);
                        }
                    });
                })(conn_id);
            }
        },

        

        
        cleanupLocalStream = function() {
            if (_webRTCStream) {
                
                
                _webRTCStream.stop();
                _webRTCStream = null;
            }
        },

        createPeerConnectionForRemote = function(remoteConnection) {
            var peerConnection = _peerConnections[remoteConnection.id];

            if (!peerConnection) {
                var startConnectingTime = OT.$.now();

                logAnalyticsEvent('createPeerConnection', 'Attempt', '', '');

                
                remoteConnection.on('destroyed', this.cleanupSubscriber.bind(this, remoteConnection.id));

                peerConnection = _peerConnections[remoteConnection.id] = new OT.PublisherPeerConnection(
                    remoteConnection,
                    _session,
                    _stream,
                    _webRTCStream
                );

                peerConnection.on({
                    connected: function() {
                        logAnalyticsEvent('createPeerConnection', 'Success', 'pcc|hasRelayCandidates', [
                            parseInt(OT.$.now() - startConnectingTime, 10),
                            peerConnection.hasRelayCandidates
                        ].join('|'));

                        
                        _qosIntervals[remoteConnection.id] = setInterval(function() {
                            recordQOS(remoteConnection.id)
                        }, 30000);
                    },
                    disconnected: onPeerDisconnected,
                    error: onPeerConnectionFailure
                }, this);

                peerConnection.init();
            }

            return peerConnection;
        },

        

        
        
        
        chromeButtonMode = function(mode) {
            if (mode === false) return 'off';

            var defaultMode = this.getStyle('buttonDisplayMode');

            
            if (defaultMode === false) return 'on';

            
            return defaultMode;
        },

        updateChromeForStyleChange = function(key, value, oldValue) {
            if (!_chrome) return;

            switch(key) {
                case 'nameDisplayMode':
                    _chrome.name.setDisplayMode(value);
                    break;

                case 'buttonDisplayMode':
                case 'showMicButton':
                case 'showSettingsButton':
                    
                    
                    

                    
                    
                    
            }
        },

        _createChrome = function() {
            _chrome = new OT.Chrome({
                parent: _container.domElement
            }).set({
                name: new OT.Chrome.NamePanel({
                    name: _publishProperties.name,
                    mode: this.getStyle('nameDisplayMode')
                }),

                
                
                

                
                muteButton: new OT.Chrome.MuteButton({
                    muted: _publishProperties.publishAudio === false,
                    mode: chromeButtonMode.call(this, this.getStyle('showMicButton'))
                }),

                opentokButton: new OT.Chrome.OpenTokButton()
            }).on({
                
                
                
                
                
                
                
                
                
                
                
                
                
                

                
                

                
                

                
                
                
                
                


                muted: this.publishAudio.bind(this, false),
                unmuted: this.publishAudio.bind(this, true)
            });
        },

        reset = function() {
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

            if (_container) {
                _container.destroy();
                _container = null;
            }

            if (this.session) this._.unpublishFromSession(this.session);

            
            for (var conn_id in _qosIntervals) {
                clearInterval(_qosIntervals[conn_id]);
                delete _qosIntervals[conn_id];
            }

            _domId = null;
            _stream = null;
            _loaded = false;

            _session = null;
            _properties = null;

            _state.set('NotPublishing');
        }.bind(this);


    this.publish = function(targetElement, properties) {
        OT.debug("OT.Publisher: publish");

        if ( _state.attemptingToPublish || _state.publishing ) reset();
        _state.set('GetUserMedia');

        _publishProperties = OT.$.defaults(properties || {}, {
            publishAudio : true,
            publishVideo : true,
            mirror: true
        });

        _publishProperties.constraints = OT.$.defaults(_publishProperties.constraints || {}, defaultConstraints);

        if (_publishProperties.style) {
            this.setStyle(_publishProperties.style, null, true);
        }

        if (_publishProperties.name) {
            _publishProperties.name = _publishProperties.name.toString();
        }

        _publishProperties.classNames = 'OT_root OT_publisher';

        
        
        OT.onLoad(function() {
            _container = new OT.WidgetView(targetElement, _publishProperties);
            _domId = _container.domId;

            OT.$.getUserMedia(
                _publishProperties.constraints,
                onStreamAvailable.bind(this),
                onStreamAvailableError.bind(this),
                onAccessDialogOpened.bind(this),
                onAccessDialogClosed.bind(this),
                onAccessDenied.bind(this)
            );
        }, this);

        return this;
    };

 













    this.publishAudio = function(value) {
        _publishProperties.publishAudio = value;

        if (_microphone) {
            _microphone.muted = !value;
        }

        if (_session && _stream) {
            _session._.modifyStream(_stream.streamId, "hasAudio", value);
        }
        return this;
    };


 













    this.publishVideo = function(value) {
        var oldValue = _publishProperties.publishVideo;
        _publishProperties.publishVideo = value;

        if (_session && _stream && _publishProperties.publishVideo !== oldValue) {
            _session._.modifyStream(_stream.streamId, "hasVideo", value);
        }

        
        
        
        if (_webRTCStream) {
            var videoTracks = _webRTCStream.getVideoTracks();
            for (var i=0, num=videoTracks.length; i<num; ++i) {
                videoTracks[i].enabled = value;
            }
        }

        if(_container) {
            _container.showPoster = !value;
        }

        return this;
    };

    this.recordQOS = function() {

        
        for (var conn_id in _peerConnections) {
            recordQOS(conn_id);
        }
    };

    





    this.destroy = function( reason, quiet) {
        reset();

        if (quiet !== true) {
            this.dispatchEvent(
              new OT.DestroyedEvent(
                OT.Event.names.PUBLISHER_DESTROYED,
                this,
                reason
              ),
              this.off.bind(this)
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

        clearInterval(_qosIntervals[fromConnectionId]);
        delete _qosIntervals[fromConnectionId];

        if (pc) {
            pc.destroy();
            delete _peerConnections[fromConnectionId];

            logAnalyticsEvent('disconnect', 'PeerConnection', 'subscriberConnection', fromConnectionId);
        }
    };


    this.processMessage = function(type, fromConnection, message) {
        OT.debug("OT.Publisher.processMessage: Received " + type + " from " + fromConnection.id);
        OT.debug(message);

        switch (type) {
            case OT.Raptor.Actions.UNSUBSCRIBE:
                this.cleanupSubscriber(fromConnection.id);
                break;

            default:
                var peerConnection = createPeerConnectionForRemote.call(this, fromConnection);
                peerConnection.processMessage(type, message);
        }
    };

    




















    this.getImgData = function() {
        if (!_loaded) {
            OT.error("OT.Publisher.getImgData: Cannot getImgData before the Publisher is publishing.");

            return null;
        }

        return _targetElement.imgData;
    };


    
    this._ = {
        publishToSession: function(session) {
            
            this.session = session;

            var createStream = function() {
                
                
                if (!this.session) return;

                _state.set('PublishingToSession');

                if (_streamCreatedTimeout) {
                  clearTimeout(_streamCreatedTimeout);
                }
                _streamCreatedTimeout = setTimeout(function () {
                  logAnalyticsEvent('publish', 'Failure', 'reason', 'StreamCreated: Timed out waiting for streamRegistered');
                  this.trigger('publishError', "StreamCreated: Timed out waiting for streamRegistered");
                }.bind(this), 30000);

                session._.createStream( this.guid,
                                        _publishProperties && _publishProperties.name ? _publishProperties.name : "",
                                        OT.VideoOrientation.ROTATED_NORMAL,
                                        _targetElement.videoWidth,                      
                                        _targetElement.videoHeight,                     
                                        _publishProperties.publishAudio,
                                        _publishProperties.publishVideo );
            };

            if (_loaded) createStream.call(this);
            else this.on("initSuccess", createStream, this);

            logAnalyticsEvent('publish', 'Attempt', 'streamType', 'WebRTC');

            return this;
        }.bind(this),

        unpublishFromSession: function(session) {
            if (!this.session || session.id !== this.session.id) {
                OT.warn("The publisher " + this.guid + " is trying to unpublish from a session " + session.id + " it is not attached to");
                return this;
            }

            if (session.connected && this.stream) {
                session._.destroyStream(this.stream.id);
            }

            
            
            this.disconnect();
            this.session = null;

            
            _state.set('MediaBound');

            logAnalyticsEvent('unpublish', 'Success', 'sessionId', session.id);

            return this;
        }.bind(this),

        
        streamRegisteredHandler: function(stream) {
            clearTimeout(_streamCreatedTimeout);
            _streamCreatedTimeout = null;

            logAnalyticsEvent('publish', 'Success', 'streamType', 'WebRTC');

            this.stream = stream;
            this.stream.on('destroyed', this.disconnect, this);

            var oldGuid = _guid;
            _guid = OT.Publisher.nextId();

            
            
            if (oldGuid) {
                this.trigger('idUpdated', oldGuid, _guid);
            }

            _state.set('Publishing');
            _publishStartTime = new Date();

            this.trigger('publishSuccess');
        }.bind(this)
    };

    this.detectDevices = function() {
        OT.warn("Fixme: Haven't implemented detectDevices");
    };

    this.detectMicActivity = function() {
        OT.warn("Fixme: Haven't implemented detectMicActivity");
    };

    this.getEchoCancellationMode = function() {
        OT.warn("Fixme: Haven't implemented getEchoCancellationMode");
        return "fullDuplex";
    };

    this.setMicrophoneGain = function(value) {
        OT.warn("Fixme: Haven't implemented setMicrophoneGain");
    };

    this.getMicrophoneGain = function() {
        OT.warn("Fixme: Haven't implemented getMicrophoneGain");
        return 0.5;
    };

    this.setCamera = function(value) {
        OT.warn("Fixme: Haven't implemented setCamera");
    };

    this.setMicrophone = function(value) {
        OT.warn("Fixme: Haven't implemented setMicrophone");
    };


    Object.defineProperties(this, {
        id: {
            get: function() { return _domId; },
            enumerable: true
        },

        guid: {
            get: function() { return _guid; },
            enumerable: true
        },

        stream: {
            get: function() { return _stream; },
            set: function(stream) { _stream = stream; },
            enumerable: true
        },

        streamId: {
            get: function() {
                if (!_stream) return null;

                return _stream.id;
            },
            enumerable: true
        },

        targetElement: {
            get: function() { return _targetElement.domElement; }
        },

        domId: {
            get: function() { return _domId; }
        },

        session: {
            get: function() { return _session; },
            set: function(session) { _session = session; },
            enumerable: true
        },

        isWebRTC: {
            get: function() { return true; }
        },

        loading: {
            get: function(){ return _container && _container.loading }
        }
    });

    Object.defineProperty(this._, 'webRtcStream', {
        get: function() { return _webRTCStream; }
    });

    this.on('styleValueChanged', updateChromeForStyleChange, this);
    _state = new OT.PublishingState(stateChangeFailed);


	







	







	







	







};


OT.Publisher.nextId = OT.$.uuid;

})(window);
(function(window) {












OT.Subscriber = function(targetElement, options) {
    var _widgetId = OT.$.uuid(),
        _domId = targetElement || _widgetId,
        _container,
        _streamContainer,
        _chrome,
        _stream,
        _fromConnectionId,
        _peerConnection,
        _session = options.session,
        _subscribeStartTime,
        _startConnectingTime,
        _qosInterval,
        _properties = OT.$.clone(options),
        _analytics = new OT.Analytics(),
        _audioVolume = 50,
        _gettingStats = 0,
        _state,
        _subscribeAudioFalseWorkaround, 
        _prevStats = {
            "timeStamp" : OT.$.now()
        };


    if (!_session) {
        OT.handleJsException("Subscriber must be passed a session option", 2000, {
            session: _session,
            target: this
        });

        return;
    }

    OT.$.eventing(this);

    OT.StylableComponent(this, {
        nameDisplayMode: "auto",
        buttonDisplayMode: "auto",
        backgroundImageURI: null
    });

    var logAnalyticsEvent = function(action, variation, payloadType, payload) {
            _analytics.logEvent({
                action: action,
                variation: variation,
                payload_type: payloadType,
                payload: payload,
                stream_id: _stream ? _stream.id : null,
                session_id: _session ? _session.sessionId : null,
                connection_id: _session && _session.connected ? _session.connection.connectionId : null,
                partner_id: _session && _session.connected ? _session.sessionInfo.partnerId : null,
                widget_id: _widgetId,
                widget_type: 'Subscriber'
            });
        },

        recordQOS = function() {
            if(_state.subscribing && _session && _session.connected) {
                var QoS_blob = {
                    widget_type: 'Subscriber',
                    stream_type : 'WebRTC',
                    session_id: _session ? _session.sessionId : null,
                    connectionId: _session ? _session.connection.connectionId : null,
                    media_server_name: _session ? _session.sessionInfo.messagingServer : null,
                    p2pFlag: _session ? _session.sessionInfo.p2pEnabled : false,
                    partner_id: _session ? _session.apiKey : null,
                    stream_id: _stream.id,
                    widget_id: _widgetId,
                    version: OT.properties.version,
                    duration: parseInt(OT.$.now() - _subscribeStartTime, 10),
                    remote_connection_id: _stream.connection.connectionId
                };


                
                _peerConnection.getStats(_prevStats, function(stats) {
                    if (stats) {
                        for (stat_index in stats) {
                            QoS_blob[stat_index] = stats[stat_index];
                        }
                    }
                    _analytics.logQOS(QoS_blob);
                });
            }
        },

        stateChangeFailed = function(changeFailed) {
            OT.error("Subscriber State Change Failed: ", changeFailed.message);
            OT.debug(changeFailed);
        },

        onLoaded = function() {
            if (_state.subscribing || !_streamContainer) return;

            OT.debug("OT.Subscriber.onLoaded");

            _state.set('Subscribing');
            _subscribeStartTime = OT.$.now();

            logAnalyticsEvent('createPeerConnection', 'Success', 'pcc|hasRelayCandidates', [
                parseInt(_subscribeStartTime - _startConnectingTime, 10),
                _peerConnection && _peerConnection.hasRelayCandidates
            ].join('|'));

            _qosInterval = setInterval(recordQOS, 30000);

            if(_subscribeAudioFalseWorkaround) {
                _subscribeAudioFalseWorkaround = null;
                this.subscribeToVideo(false);
            }

            _container.loading = false;

            _createChrome.call(this);

            this.trigger('subscribeSuccess', this);
            this.trigger('loaded', this);


            logAnalyticsEvent('subscribe', 'Success', 'streamId', _stream.id);
        },

        onDisconnected = function() {
            OT.debug("OT.Subscriber has been disconnected from the Publisher's PeerConnection");

            if (_state.attemptingToSubscribe) {
                
                _state.set('Failed');
                this.trigger('subscribeError', "ClientDisconnected");
            }
            else if (_state.subscribing) {
                _state.set('Failed');

                
                
            }

            this.disconnect();
        },


        onPeerConnectionFailure = function(code, reason) {
            if (_state.attemptingToSubscribe) {
                
                
                logAnalyticsEvent('createPeerConnection', 'Failure', 'reason|hasRelayCandidates', [
                    "Subscriber PeerConnection Error: " + reason,
                    _peerConnection && _peerConnection.hasRelayCandidates
                ].join('|'));

                _state.set('Failed');
                this.trigger('subscribeError', reason);
            }
            else if (_state.subscribing) {
                
                _state.set('Failed');
                this.trigger('error', reason);
            }

            this.disconnect();

            logAnalyticsEvent('subscribe', 'Failure', 'reason', reason + ":Subscriber PeerConnection Error");

            OT.handleJsException("Subscriber PeerConnection Error: " + reason, OT.ExceptionCodes.P2P_CONNECTION_FAILED, {
                session: _session,
                target: this
            });
            _showError.call(this, reason);
        },

        onRemoteStreamAdded = function(webOTStream) {
            OT.debug("OT.Subscriber.onRemoteStreamAdded");

            _state.set('BindingRemoteStream');

            
            this.subscribeToAudio(_properties.subscribeToAudio);

            var preserver = _subscribeAudioFalseWorkaround;
            this.subscribeToVideo(_properties.subscribeToVideo);
            _subscribeAudioFalseWorkaround = preserver;

            var streamElement = new OT.VideoElement();

            
            streamElement.setAudioVolume(_audioVolume);
            streamElement.on({
                    streamBound: onLoaded,
                    loadError: onPeerConnectionFailure,
                    error: onPeerConnectionFailure
                }, this);

            streamElement.bindToStream(webOTStream);
             _container.video = streamElement;

            _streamContainer = streamElement;

            _streamContainer.orientation = {
                width: _stream.videoDimensions.width,
                height: _stream.videoDimensions.height,
                videoOrientation: _stream.videoDimensions.orientation
            };

            logAnalyticsEvent('createPeerConnection', 'StreamAdded', '', '');
            this.trigger('streamAdded', this);
        },

        onRemoteStreamRemoved = function(webOTStream) {
            OT.debug("OT.Subscriber.onStreamRemoved");

            if (_streamContainer.stream == webOTStream) {
                _streamContainer.destroy();
                _streamContainer = null;
            }


            this.trigger('streamRemoved', this);
        },

        streamUpdated = function(event) {
            switch(event.changedProperty) {
                case 'orientation':
                    _streamContainer.orientation = {
                        width: _stream.videoDimensions.width,
                        height: _stream.videoDimensions.height,
                        videoOrientation: _stream.videoDimensions.orientation
                    };
                    break;

                case 'hasVideo':
                    if(_container) {
                        _container.showPoster = !(_stream.hasVideo && _properties.subscribeToVideo);
                    }

                    break;

                case 'hasAudio':
                    
            }
        },

        

        updateChromeForStyleChange = function(key, value, oldValue) {
            if (!_chrome) return;

            switch(key) {
                case 'nameDisplayMode':
                    _chrome.name.setDisplayMode(value);
                    break;

                case 'buttonDisplayMode':
                    
            }
        },

        _createChrome = function() {
            _chrome = new OT.Chrome({
                parent: _container.domElement
            }).set({
                name: new OT.Chrome.NamePanel({
                    name: _properties.name,
                    mode: this.getStyle('nameDisplayMode')
                }),

                
                
                
                
                

                opentokButton: new OT.Chrome.OpenTokButton()
            }).on({
                muted: function() {
                    
                },

                unmuted: function() {
                    
                }
            });
        },

        _showError = function(errorMsg) {
            
            
            if (_container) _container.addError(errorMsg);
        };


    this.recordQOS = function() {
        recordQOS();
    };

    this.subscribe = function(stream) {
        OT.debug("OT.Subscriber: subscribe to " + stream.id);

        if (_state.subscribing) {
            
            OT.error("OT.Subscriber.Subscribe: Cannot subscribe, already subscribing.");
            return false;
        }

        _state.set('Init');

        if (!stream) {
            
            OT.error("OT.Subscriber: No stream parameter.");
            return false;
        }

        if (_stream) {
            
            OT.error("OT.Subscriber: Already subscribed");
            return false;
        }

        _stream = stream;
        _stream.on({
            updated: streamUpdated,
            destroyed: this.disconnect
        }, this);

        _fromConnectionId = stream.connection.connectionId;
        _properties.name = _stream.name;
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
        _domId = _container.domId;

        if(!_properties.subscribeToVideo && OT.$.browser() == 'Chrome') {
            _subscribeAudioFalseWorkaround = true;
            _properties.subscribeToVideo = true;
        }

        _startConnectingTime = OT.$.now();

        if (_stream.connection.id !== _session.connection.id) {
            logAnalyticsEvent('createPeerConnection', 'Attempt', '', '');

            _state.set('ConnectingToPeer');

            _peerConnection = new OT.SubscriberPeerConnection(_stream.connection, _session, _stream, _properties);

            _peerConnection.on({
                disconnected: onDisconnected,
                error: onPeerConnectionFailure,
                remoteStreamAdded: onRemoteStreamAdded,
                remoteStreamRemoved: onRemoteStreamRemoved
            }, this);

            
            _peerConnection.init();
        }
        else {
            logAnalyticsEvent('createPeerConnection', 'Attempt', '', '');

            
            onRemoteStreamAdded.call(this, _session.getPublisherForStream(_stream)._.webRtcStream);
        }

        logAnalyticsEvent('subscribe', 'Attempt', 'streamId', _stream.id);

        return this;
    };

    this.destroy = function( reason, quiet) {
        clearInterval(_qosInterval);
        _qosInterval = null;

        this.disconnect();

        if (_chrome) {
            _chrome.destroy();
            _chrome = null;
        }

        if (_container) {
            _container.destroy();
            _container = null;
        }

        if (_stream && !_stream.destroyed) logAnalyticsEvent('unsubscribe', null, 'streamId', _stream.id);

        _domId = null;
        _stream = null;

        _session = null;
        _properties = null;

        if (quiet !== true) {
            this.dispatchEvent(
              new OT.DestroyedEvent(
                OT.Event.names.SUBSCRIBER_DESTROYED,
                this,
                reason
              ),
              this.off.bind(this)
            );
        }

        return this;
    };


    this.disconnect = function() {
        _state.set('NotSubscribing');

        if (_streamContainer) {
            _streamContainer.destroy();
            _streamContainer = null;
        }

        if (_peerConnection) {
            _peerConnection.destroy();
            _peerConnection = null;

            logAnalyticsEvent('disconnect', 'PeerConnection', 'streamId', _stream.id);
        }
    };

    this.processMessage = function(type, fromConnection, message) {
        OT.debug("OT.Subscriber.processMessage: Received " + type + " message from " + fromConnection.id);
        OT.debug(message);

        if (_fromConnectionId != fromConnection.id) {
            _fromConnectionId = fromConnection.id;
        }

        if (_peerConnection) {
          _peerConnection.processMessage(type, message);
        }
    };

    this.updateQuality = function(quality) {
        
        
        OT.warn("Due to high packet loss and low bandwidth, video has been disabled");
        this.subscribeToVideo(false);
        this.dispatchEvent(new OT.Event("videoDisabled"));
    };

    


















    this.getImgData = function() {
        if (!this.subscribing) {
            OT.error("OT.Subscriber.getImgData: Cannot getImgData before the Subscriber is subscribing.");
            return null;
        }

        return _streamContainer.imgData;
    };

    

















    this.setAudioVolume = function(value) {
        value = parseInt(value, 10);
        if (isNaN(value)) {
            OT.error("OT.Subscriber.setAudioVolume: value should be an integer between 0 and 100");
            return this;
        }
        _audioVolume = Math.max(0, Math.min(100, value));
        if (_audioVolume != value) {
            OT.warn("OT.Subscriber.setAudioVolume: value should be an integer between 0 and 100");
        }
        if (_streamContainer) {
            _streamContainer.setAudioVolume(_audioVolume);
        }

        return this;
    };

    









    this.getAudioVolume = function() {
        if (_streamContainer) return _streamContainer.getAudioVolume();
        else return _audioVolume;
    };

    






















    this.subscribeToAudio = function(p_value) {
        var value = OT.$.castToBoolean(p_value, true);

        if (_peerConnection) {
            _peerConnection.subscribeToAudio(value);

            if (_session && _stream && value !== _properties.subscribeToAudio) {
                _session._.modifySubscriber(this, "hasAudio", value);
            }
        }

        _properties.subscribeToAudio = value;

        return this;
    };


    






















    this.subscribeToVideo = function(p_value) {
        if(_subscribeAudioFalseWorkaround && p_value == true) {
            
            _subscribeAudioFalseWorkaround = false;
            return;
        }

        var value = OT.$.castToBoolean(p_value, true);

        if(_container) {
            _container.showPoster = !(value && _stream.hasVideo);
            if(value && _container.video) {
                _container.loading = value;
                _container.video.whenTimeIncrements(function(){
                    _container.loading = false;
                }, this);
            }
        }

        if (_peerConnection) {
            _peerConnection.subscribeToVideo(value);

            if (_session && _stream && value !== _properties.subscribeToVideo) {
                _session._.modifySubscriber(this, "hasVideo", value);
            }
        }

        _properties.subscribeToVideo = value;

        return this;
    };

    Object.defineProperties(this, {
        id: {
            get: function() { return _domId; },
            enumerable: true
        },

        widgetId: {
            get: function() { return _widgetId; }
        },

        stream: {
            get: function() { return _stream; },
            enumerable: true
        },

        streamId: {
            get: function() {
                if (!_stream) return null;

                return _stream.id;
            },
            enumerable: true
        },

        targetElement: {
            get: function() { return _streamContainer ? _streamContainer.domElement : null; }
        },

        subscribing: {
            get: function() { return _state.subscribing; },
            enumerable: true
        },

        isWebRTC: {
            get: function() { return true; }
        },

        loading: {
            get: function(){ return _container && _container.loading }
        },

        session: {
            get: function() { return _session; }
        }
    });

    this.on('styleValueChanged', updateChromeForStyleChange, this);

    _state = new OT.SubscribingState(stateChangeFailed);

	










};

})(window);
(function(window) {
    OT.SessionInfo = function(xmlDocument) {
        var sessionXML = null;

        this.sessionId = null;
        this.partnerId = null;
        this.sessionStatus = null;
        this.p2pEnabled = false;

        this.messagingServer = null;
        this.iceServers = null;

        OT.log("SessionInfo Response:")
        OT.log(xmlDocument);

        if (xmlDocument && xmlDocument.documentElement && xmlDocument.documentElement.firstElementChild !== null) {
            sessionXML = xmlDocument.documentElement.firstElementChild;
        }

        var element = sessionXML.firstElementChild;
        do {
            switch (element.localName) {
            case "session_id":
                this.sessionId = element.textContent;
                break;

            case "partner_id":
                this.partnerId = element.textContent;
                break;

            case "session_status":
                this.sessionStatus = element.textContent;
                break;

            case "messaging_server_url":
                this.messagingServer = element.textContent;
                break;

            case "ice_servers":
                
                
                
                

                this.iceServers = normaliseIceServers( parseIceServersXml(element.childNodes) );
                break;

            case "properties":
                var property = element.firstElementChild;
                if (property) {
                    do {
                        if (property.localName === "p2p" && property.firstElementChild !== null) {
                            this.p2pEnabled = (property.firstElementChild.textContent === "enabled");
                            break;
                        }
                    } while (property = property.nextElementSibling);
                }

                break;

            default:
                
                break;
            }

        } while (element = element.nextElementSibling);

        if (!this.iceServers || this.iceServers.length === 0) {
            
            OT.warn("SessionInfo contained not ICE Servers, using the default");
            this.iceServers = [ {"url": "stun:stun.l.google.com:19302"} ];
        }

        

        sessionXML = null;
    };





OT.SessionInfo.get = function(session, onSuccess, onFailure) {
    var sessionInfoURL = OT.properties.apiURL + '/session/' + session.id + "?extended=true",

        startTime = OT.$.now(),

        validateRawSessionInfo = function(sessionInfo) {
            session.logEvent('Instrumentation', null, 'gsi', OT.$.now() - startTime);

            var error = parseErrorFromXMLDocument(sessionInfo);

            if (error === false) {
                onGetResponseCallback(session, onSuccess, sessionInfo);
            }
            else {
                onGetErrorCallback(session, onFailure, error);
            }
        };

    session.logEvent('getSessionInfo', 'Attempt', 'api_url', OT.properties.apiURL);

    OT.$.getXML(sessionInfoURL, {
        headers: {"X-TB-TOKEN-AUTH": session.token, "X-TB-VERSION": 1},

        success: validateRawSessionInfo,

        error: function(event) {
            onGetErrorCallback(session, onFailure, parseErrorFromXMLDocument(event.target.responseXML));
        }
    });
};

var messageServerToClientErrorCodes = {};
messageServerToClientErrorCodes['404'] = OT.ExceptionCodes.INVALID_SESSION_ID;
messageServerToClientErrorCodes['403'] = OT.ExceptionCodes.AUTHENTICATION_ERROR;



parseErrorFromXMLDocument = function(xmlDocument) {
    if (xmlDocument && xmlDocument.documentElement && xmlDocument.documentElement.firstElementChild !== null) {
        var errorNodes = xmlDocument.evaluate('//error', xmlDocument.documentElement, null, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null ),
            numErrorNodes = errorNodes.snapshotLength;

        if (numErrorNodes === 0) return false;

        for (var i=0; i<numErrorNodes; ++i) {
            var errorNode = errorNodes.snapshotItem(i);

            return {
                code: errorNode.getAttribute('code'),
                message: errorNode.firstElementChild.getAttribute('message')
            };
        }
    }

    
    return {
        code: null,
        message: "Unknown error: getSessionInfo XML response was badly formed " +
          (xmlDocument && xmlDocument.documentElement && xmlDocument.documentElement.innerHTML)
    };
};

onGetResponseCallback = function(session, onSuccess, rawSessionInfo) {
  session.logEvent('getSessionInfo', 'Success', 'api_url', OT.properties.apiURL);

  onSuccess( new OT.SessionInfo(rawSessionInfo) );
};

onGetErrorCallback = function(session, onFailure, error) {
    TB.handleJsException("TB.SessionInfoError :: Unable to get session info " + error.message, messageServerToClientErrorCodes[error.code], {
        session: session
    });

    session.logEvent('Connect', 'Failure', 'errorMessage', "GetSessionInfo:" + error.code + ": Unable to get session info " + error.message);
    onFailure(error, session);
};

parseIceServersXml = function (nodes) {
    var iceServers = [],
        iceServer,
        attributes;

    for (var i=0, numNodes=nodes.length; i<numNodes; ++i) {
        if (nodes[i].localName === 'ice_server') {
            
            attributes = nodes[i].attributes;
            iceServer = {
                url: attributes.getNamedItem('url').nodeValue
            };

            if (attributes.getNamedItem('credential') && attributes.getNamedItem('credential').nodeValue.length) {
                iceServer.credential = attributes.getNamedItem('credential').nodeValue;
            }

            iceServers.push(iceServer);
        }
    }

    return iceServers;
};











normaliseIceServers = function (iceServers) {
    var userAgent = navigator.userAgent.match(/(Firefox)\/([0-9]+\.[0-9]+)/),
        firefoxVersion = userAgent ? parseFloat(userAgent[2], 10) : void 0,
        bits;

    return iceServers.map(function(iceServer) {
        
        if (iceServer.url.trim().substr(0, 5) !== 'turn:') {
            return iceServer;
        }

        if (userAgent !== null) {
            
            if (firefoxVersion < 25) {
                return {url: iceServer.url.replace("turn:", "stun:")};
            }

            
            if (firefoxVersion < 27 && iceServer.url.indexOf('?') !== -1) {
                iceServer.url = iceServer.url.trim().split('?')[0];
            }
        }

        bits = iceServer.url.trim().split(/[:@]/);

        return {
            username: bits[1],
            credential: iceServer.credential,
            url: bits[0] + ':' + bits[2] + (bits.length === 4 ? ':' + bits[3] : '')
        };
    });
};

})(window);
(function(window) {
	
























	OT.Capabilities = function(permissions) {
	    this.publish = permissions.indexOf('publish') !== -1 ? 1 : 0;
	    this.subscribe = permissions.indexOf('subscribe') !== -1 ? 1 : 0;
	    this.forceUnpublish = permissions.indexOf('forceunpublish') !== -1 ? 1 : 0;
	    this.forceDisconnect = permissions.indexOf('forcedisconnect') !== -1 ? 1 : 0;
	    this.supportsWebRTC = OT.$.supportsWebRTC() ? 1 : 0;
    };

})(window);
(function(window) {



var RemoteWork = function RemoteWork (parent, success, error, options) {
  var REQUEST_TIMEOUT = 30000,
      timeoutInterval,
      exceptionCodesIndicatingFailure = {};

  var destroy = function() {
        clearTimeout(timeoutInterval);
        parent.off('exception', onException);
      },

      onException = function(event) {
        if (!exceptionCodesIndicatingFailure.hasOwnProperty(event.code)) return;

        
        this.failed(exceptionCodesIndicatingFailure[event.code]);
      },

      onTimeout = function() {
        var reason = options && options.timeoutMessage ? options.timeoutMessage : "Timed out while waiting for the server to respond.";
        this.failed(reason);
      };


  this.failsOnExceptionCodes = function(codes) {
    exceptionCodesIndicatingFailure = codes;
  };

  this.succeeded = function() {
    destroy();
    if (completionHandler) OT.$.callAsync(completionHandler, null);
  };

  this.failed = function(reason) {
    destroy();
    if (completionHandler) OT.$.callAsync(completionHandler, new OT.Error(null, reason));
  };

  parent.on('exception', onException, this);
  timeoutInterval = setTimeout(onTimeout.bind(this), REQUEST_TIMEOUT);
};




















OT.Session = function(sessionId) {
  
  
  if (!OT.checkSystemRequirements()) {
      OT.upgradeSystemRequirements();
      return;
  }

  var _initialConnection = true,
      _apiKey,
      _token,
      _sessionId = sessionId,
      _socket,
      _widgetId = OT.$.uuid(),
      _analytics = new OT.Analytics(),
      _connectionId,
      _callbacks = {
        forceDisconnect: {},
        forceUnpublish: {}
      };


  OT.$.eventing(this);
  var setState = OT.$.statable(this, ['disconnected', 'connecting', 'connected', 'disconnecting'], 'disconnected');

  this.connections = new OT.Collection();
  this.streams = new OT.Collection();


	
	
	

	var
  
  
  sessionConnectFailed = function(reason, code) {
    setState('disconnected');

    OT.error(reason);

    this.trigger('sessionConnectFailed', reason);

    TB.handleJsException(reason, code || OT.ExceptionCodes.CONNECT_FAILED, {
      session: this
    });
  },

	sessionDisconnectedHandler = function(event) {
    var reason = event.reason;
    if(reason == "networkTimedout") {
      reason = "networkDisconnected";
      this.logEvent('Connect', 'TimeOutDisconnect', "reason", event.reason);
    } else {
      this.logEvent('Connect', 'Disconnected', "reason", event.reason);
    }

		var publicEvent = new OT.SessionDisconnectEvent('sessionDisconnected', reason);

    reset.call(this);
    disconnectComponents.call(this);

		var defaultAction = function() {
      if (!publicEvent.isDefaultPrevented()) destroyComponents.call(this, publicEvent.reason);
		}.bind(this);

		this.dispatchEvent(publicEvent, defaultAction);
	},

  connectionCreatedHandler = function(connection) {
    
    if (connection.id.match(/^symphony\./)) return;

    this.dispatchEvent(new OT.ConnectionEvent(
        OT.Event.names.CONNECTION_CREATED,
        [connection]
    ));
  },

	connectionDestroyedHandler = function(connection, reason) {
    
    if (connection.id.match(/^symphony\./)) return;

    
    
    
    if (connection.id === _socket.id) return;

    
    if (_callbacks.forceDisconnect[connection.id]) {
      var callback = _callbacks.forceDisconnect[connection.id];
      delete _callbacks.forceDisconnect[connection.id];


      if (reason !== 'forceDisconnected') {
        OT.warn("Expected a forceDisconnect for connection " + connection.id + ", but a " + reason + " was received instead.");
      }

      callback.succeeded();
    }

    this.dispatchEvent(
      new OT.ConnectionEvent(
        OT.Event.names.CONNECTION_DESTROYED,
        [connection],
        reason
      )
    );
	},

  streamCreatedHandler = function(stream) {
    this.dispatchEvent(new OT.StreamEvent(
      OT.Event.names.STREAM_CREATED,
      [stream]
    ));
  },

  streamPropertyModifiedHandler = function(event) {
    var stream = event.target,
        propertyName = event.changedProperty,
        newValue = event.newValue;

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
  },

	streamDestroyedHandler = function(stream, reason) {
    
    if (_callbacks.forceUnpublish[stream.id]) {
      var callback = _callbacks.forceUnpublish[stream.id];
      delete _callbacks.forceUnpublish[stream.id];

      if (reason !== 'forceUnpublished') {
        OT.warn("Expected a forceUnpublish for stream " + stream.id + ", but a " + reason + " destroyed was received instead.");
      }

      callback.succeeded();
    }

    var event = new OT.StreamEvent('streamDestroyed', [stream], reason);

    var defaultAction = function() {
      if (!event.isDefaultPrevented()) {
        
        
        var publisher = OT.publishers.where({streamId: stream.id})[0];
        if (publisher) {
          publisher._.unpublishFromSession(this);
          publisher.destroy();
        }

        
        OT.subscribers.where({streamId: stream.id}).forEach(function(subscriber) {
          if (subscriber.session.id === this.id) {
            this.unsubscribe(subscriber);
          }
        }, this);
      }
		}.bind(this);

		this.dispatchEvent(event, defaultAction);
	},


	
	reset = function() {
    _apiKey = null;
    _token = null;
    setState('disconnected');

    this.connections.destroy();
    this.streams.destroy();
	},

  disconnectComponents = function() {
    OT.publishers.where({session: this}).forEach(function(publisher) {
      publisher.disconnect();
    });

    OT.subscribers.where({session: this}).forEach(function(subscriber) {
      subscriber.disconnect();
    });
  },

  destroyComponents = function(reason) {
    OT.publishers.where({session: this}).forEach(function(publisher) {
      publisher.destroy(reason);
    });

    OT.subscribers.where({session: this}).forEach(function(subscriber) {
      subscriber.destroy(reason);
    });
  },

	connectMessenger = function() {
    TB.debug("OT.Session: connecting to Raptor");

    _socket = new OT.Raptor.Socket(_widgetId, this.sessionInfo.messagingServer);
    _socket.connect(_token, this.sessionInfo, function(error, sessionState) {
      if (error) {
        sessionConnectFailed.call(this, error.reason, error.code);
        return;
      }

      OT.debug("OT.Session: Received session state from Raptor", sessionState);

      _connectionId = this.connection.id;
      setState('connected');

      
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

      this.dispatchEvent(new OT.SessionConnectEvent(
        OT.Event.names.SESSION_CONNECTED,
        sessionState.connections,
        sessionState.streams,
        sessionState.archives
      ));
    }.bind(this));
	},

  getSessionInfo = function() {
    if (this.is('connecting')) {
      OT.SessionInfo.get(
        this,
        onSessionInfoResponse.bind(this),
        function(error) {
          sessionConnectFailed.call(this, error.message + (error.code ? ' (' + error.code + ')' : ''));
        }.bind(this)
      );
    }
  },

  onSessionInfoResponse = function(sessionInfo) {
    if (this.is('connecting')) {
      this.sessionInfo = sessionInfo;
      if (this.sessionInfo.partnerId && this.sessionInfo.partnerId != _apiKey) {
          _apiKey = this.sessionInfo.partnerId;

          var reason = 'Authentication Error: The apiKey passed into the session.connect ' +
            'method does not match the apiKey in the token or session you are trying to ' +
            'connect to.';

          this.logEvent('Connect', 'Failure', 'reason', 'GetSessionInfo:' +
            OT.ExceptionCodes.AUTHENTICATION_ERROR + ':' + reason);

          sessionConnectFailed.call(this, reason, OT.ExceptionCodes.AUTHENTICATION_ERROR);
      } else {
          connectMessenger.call(this);
      }
    }
  },

  
  permittedTo = function(action) {
      return _socket && _socket.permittedTo(action);
  };

  this.logEvent = function(action, variation, payload_type, payload) {
    var event = {
      action: action,
      variation: variation,
      payload_type: payload_type,
      payload: payload,
      session_id: _sessionId,
      partner_id: _apiKey,
      widget_id: _widgetId,
      widget_type: 'Controller'
    };
    if (this.connection && this.connection.id) event.connection_id = this.connection.id;
    else if (_connectionId) event.connection_id = _connectionId;
    _analytics.logEvent(event);
  };

 












































































  this.connect = function(apiKey, token, completionHandler) {
    if (this.is('connecting', 'connected')) {
      OT.warn("OT.Session: Cannot connect, the session is already " + this.state);
      return;
    }

    reset.call(this);
    setState('connecting');
    _token = token;

    
    if (_initialConnection) {
      _initialConnection = false;
    } else {
      _widgetId = OT.$.uuid();
    }

    _apiKey = apiKey.toString();

    
    if (OT.APIKEY.length === 0) {
        OT.APIKEY = _apiKey;
    }

    if (completionHandler && OT.$.isFunction(completionHandler)) {
      this.once(OT.Event.names.SESSION_CONNECTED, completionHandler.bind(null, null));
      this.once('sessionConnectFailed', completionHandler);
    }

    var analyticsPayload = [
      navigator.userAgent, OT.properties.version,
      window.externalHost ? 'yes' : 'no'
    ];
    this.logEvent( 'Connect', 'Attempt',
      'userAgent|sdkVersion|chromeFrame',
      analyticsPayload.map(function(e) { return e.replace('|', '\\|'); }).join('|')
    );

    getSessionInfo.call(this);
  };

 


















































  this.disconnect = function() {
    if (_socket && _socket.isNot('disconnected')) {
      setState('disconnecting');
      _socket.disconnect();
    }
    else {
      reset.call(this);
    }
  };

  this.destroy = function(reason, quiet) {
    this.streams.destroy();
    this.connections.destroy();
    this.disconnect();
  };

 























































































  this.publish = function(publisher, properties, completionHandler) {
    var errorMsg;

    if (this.isNot('connected')) {
      _analytics.logError(1010, 'tb.exception', "We need to be connected before you can publish", null, {
        action: 'publish',
        variation: 'Failure',
        payload_type: "reason",
        payload: "We need to be connected before you can publish",
        session_id: _sessionId,
        partner_id: _apiKey,
        widgetId: _widgetId,
        widget_type: 'Controller'
      });

      if (completionHandler && OT.$.isFunction(completionHandler)) {
        errorMsg = "We need to be connected before you can publish";
        OT.$.callAsync(completionHandler, new OT.Error(OT.ExceptionCodes.NOT_CONNECTED, errorMsg));
      }

      return null;
    }

    if (!permittedTo("publish")) {
      this.logEvent('publish', 'Failure', 'reason', 'This token does not allow publishing. The role must be at least `publisher` to enable this functionality');

      TB.handleJsException("This token does not allow publishing. The role must be at least `publisher` to enable this functionality", OT.ExceptionCodes.UNABLE_TO_PUBLISH, {
        session: this
      });
      return null;
    }

    
    if (!publisher || typeof(publisher)==='string' || publisher.nodeType == Node.ELEMENT_NODE){
      
     publisher = OT.initPublisher(this.apiKey, publisher, properties);
    }
    else if (publisher instanceof OT.Publisher){

      
      if( "session" in publisher && publisher.session && "sessionId" in publisher.session ){
        
        if( publisher.session.sessionId === this.sessionId){
          OT.warn("Cannot publish " + publisher.guid + " again to " + this.sessionId + ". Please call session.unpublish(publisher) first.");
        }
        else {
          OT.warn("Cannot publish " + publisher.guid + " publisher already attached to " + publisher.session.sessionId+ ". Please call session.unpublish(publisher) first.");
        }
      }
    }
    else {
      errorMsg = "Session.publish :: First parameter passed in is neither a string nor an instance of the Publisher";
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    if (completionHandler && OT.$.isFunction(completionHandler)) publisher.once('publishComplete', completionHandler);

    
    publisher._.publishToSession(this);

    
    return publisher;
  };

 













































































  this.unpublish = function(publisher) {
    if (!publisher) {
      OT.error('OT.Session.unpublish: publisher parameter missing.');
      return;
    }

    
    publisher._.unpublishFromSession(this);
  };


 






































































































  this.subscribe = function(stream, targetElement, properties, completionHandler) {
    var errorMsg;

    if (!this.connection || !this.connection.connectionId) {
      errorMsg = "Session.subscribe :: Connection required to subscribe";
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    if (!stream) {
      errorMsg = "Session.subscribe :: stream cannot be null";
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    if (!stream.hasOwnProperty("streamId")) {
      errorMsg = "Session.subscribe :: invalid stream object";
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    var subscriber = new OT.Subscriber(targetElement, OT.$.extend(properties || {}, {
        session: this
    }));

    if (completionHandler && OT.$.isFunction(completionHandler)) subscriber.once('subscribeComplete', completionHandler);

    OT.subscribers.add(subscriber);
    subscriber.subscribe(stream);

    return subscriber;
  };

 































































  this.unsubscribe = function(subscriber) {
    if (!subscriber) {
      var errorMsg = "OT.Session.unsubscribe: subscriber cannot be null";
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    if (!subscriber.stream) {
        OT.warn("OT.Session.unsubscribe:: tried to unsubscribe a subscriber that had no stream");
        return false;
    }

    OT.debug("OT.Session.unsubscribe: subscriber " + subscriber.id);

    subscriber.destroy();

    return true;
  };

 












  this.getSubscribersForStream = function(stream) {
    return OT.subscribers.where({streamId: stream.id});
  };

 














  this.getPublisherForStream = function(stream) {
    var streamId;

    if (typeof(stream) == "string") {
      streamId = stream;
    } else if (typeof(stream) == "object" && stream && stream.hasOwnProperty("id")) {
      streamId = stream.id;
    } else {
      errorMsg = "Session.getPublisherForStream :: Invalid stream type";
      OT.error(errorMsg);
      throw new Error(errorMsg);
    }

    return OT.publishers.where({streamId: streamId})[0];
  };


  
  this._ = {
    jsepSubscribe: function(stream, subscribeToVideo, subscribeToAudio) {
      return _socket.jsepSubscribe(stream.connection.id, stream.id, subscribeToVideo, subscribeToAudio);
    }.bind(this),

    jsepUnsubscribe: function(stream) {
      return _socket.jsepUnsubscribe(stream.connection.id, stream.id);
    }.bind(this),

    jsepCandidate: function(toConnectionId, stream, candidate) {
      return _socket.jsepCandidate(toConnectionId, stream.id, candidate);
    }.bind(this),

    jsepOffer: function(toConnectionId, stream, offerSDP) {
      return _socket.jsepOffer(toConnectionId, stream.id, offerSDP);
    }.bind(this),

    jsepAnswer: function(toConnectionId, stream, answerSDP) {
      return _socket.jsepAnswer(toConnectionId, stream.id, answerSDP);
    }.bind(this),

    
    
    dispatchSignal: function(fromConnection, type, data) {
      var event = new OT.SignalEvent(type, data, fromConnection);
      event.target = this;

      
      
      this.trigger(OT.Event.names.SIGNAL, event);

      
      if (type) this.dispatchEvent(event);
    }.bind(this),

    modifySubscriber: function(subscriber, key, value) {
      return _socket.modifySubscriber(subscriber, key, value);
    }.bind(this),

    createStream: function(publisherId, name, orientation, encodedWidth, encodedHeight, hasAudio, hasVideo) {
      _socket.createStream(publisherId, name, orientation, encodedWidth, encodedHeight, hasAudio, hasVideo);
    }.bind(this),

    modifyStream: function(streamId, key, value) {
      if (!streamId || !key || value === void 0) {
        OT.error('OT.Session.modifyStream: must provide streamId, key and value to modify a stream property.');
        return;
      }

      _socket.updateStream(streamId, key, value);
    }.bind(this),

    destroyStream: function(streamId) {
      _socket.destroyStream(streamId);
    }.bind(this)
  };


 












































































































  this.signal = function(options, completion) {
    _socket.signal(options, completion);
  };

 

























































  this.forceDisconnect = function(connectionOrConnectionId, completionHandler) {
    var notPermittedErrorMsg = "This token does not allow forceDisconnect. The role must be at least `moderator` to enable this functionality";

    if (permittedTo("forceDisconnect")) {
      var connectionId = typeof(connectionOrConnectionId) === 'string' ? connectionOrConnectionId : connectionOrConnectionId.id;

      if (completionHandler) {
        var work = new RemoteWork(this, completionHandler, {
          timeoutMessage: "Timed out while waiting for connection " + connectionId + " to be force Disconnected."
        });

        work.failsOnExceptionCodes({
          1520: notPermittedErrorMsg
        });

        _callbacks.forceDisconnect[connectionId] = work;
      }

      _socket.forceDisconnect(connectionId);
    } else {
      
      if (completionHandler) OT.$.callAsync(completionHandler, new OT.Error(null, notPermittedErrorMsg));

      TB.handleJsException(notPermittedErrorMsg, OT.ExceptionCodes.UNABLE_TO_FORCE_DISCONNECT, {
        session: this
      });
    }
  };

 


































  this.forceUnpublish = function(streamOrStreamId, completionHandler) {
    var notPermittedErrorMsg = "This token does not allow forceUnpublish. The role must be at least `moderator` to enable this functionality";

    if (permittedTo("forceUnpublish")) {
      var stream = typeof(streamOrStreamId) === 'string' ? this.streams.get(streamOrStreamId) : streamOrStreamId;

      if (completionHandler) {
        var work = new RemoteWork(this, completionHandler, {
          timeoutMessage: "Timed out while waiting for stream " + stream.id + " to be force unpublished."
        });

        work.failsOnExceptionCodes({
          1530: notPermittedErrorMsg
        });

        _callbacks.forceUnpublish[stream.id] = work;
      }

      _socket.forceUnpublish(stream.id);
    } else {
      
      if (completionHandler) OT.$.callAsync(completionHandler, new OT.Error(null, notPermittedErrorMsg));

      TB.handleJsException(notPermittedErrorMsg, OT.ExceptionCodes.UNABLE_TO_FORCE_UNPUBLISH, {
        session: this
      });
    }
  };

  this.getStateManager = function() {
      OT.warn("Fixme: Have not implemented session.getStateManager");
  };

  OT.$.defineGetters(this, {
    apiKey: function() { return _apiKey; },
    token: function() { return _token; },
    connected: function() { return this.is('connected'); },
    connection: function() { return _socket && _socket.id ? this.connections.get(_socket.id) : null; },
    capabilities: function() { return _socket ? _socket.capabilities : new OT.Capabilities([]); },
    sessionId: function() { return _sessionId; },
    id: function() { return _sessionId; }
  }, true);


	








	







	












	











	








	







	














	



























	































};

})(window);
(function(window) {
  var style = document.createElement('link');
  style.type = 'text/css';
  style.media = 'screen';
  style.rel = 'stylesheet';
  style.href = OT.properties.cssURL;
  var head = document.head || document.getElementsByTagName('head')[0];
  head.appendChild(style);
})(window);
(function(window){







if ( typeof define === "function" && define.amd ) {
  define( "TB", [], function () { return TB; } );
}

})(window);
