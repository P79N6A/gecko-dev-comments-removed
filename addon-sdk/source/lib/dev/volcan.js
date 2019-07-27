


!function(e){if("object"==typeof exports)module.exports=e();else if("function"==typeof define&&define.amd)define(e);else{var f;"undefined"!=typeof window?f=window:"undefined"!=typeof global?f=global:"undefined"!=typeof self&&(f=self),f.volcan=e()}}(function(){var define,module,exports;return (function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);throw new Error("Cannot find module '"+o+"'")}var f=n[o]={exports:{}};t[o][0].call(f.exports,function(e){var n=t[o][1][e];return s(n?n:e)},f,f.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(_dereq_,module,exports){
"use strict";

var Client = _dereq_("../client").Client;

function connect(port) {
  var client = new Client();
  return client.connect(port);
}
exports.connect = connect;

},{"../client":4}],2:[function(_dereq_,module,exports){
"use strict";

exports.Promise = Promise;

},{}],3:[function(_dereq_,module,exports){
"use strict";

var describe = Object.getOwnPropertyDescriptor;
var Class = function(fields) {
  var names = Object.keys(fields);
  var constructor = names.indexOf("constructor") >= 0 ? fields.constructor :
                    function() {};
  var ancestor = fields.extends || Object;

  var descriptor = names.reduce(function(descriptor, key) {
    descriptor[key] = describe(fields, key);
    return descriptor;
  }, {});

  var prototype = Object.create(ancestor.prototype, descriptor);

  constructor.prototype = prototype;
  prototype.constructor = constructor;

  return constructor;
};
exports.Class = Class;

},{}],4:[function(_dereq_,module,exports){
"use strict";

var Class = _dereq_("./class").Class;
var TypeSystem = _dereq_("./type-system").TypeSystem;
var values = _dereq_("./util").values;
var Promise = _dereq_("es6-promise").Promise;
var MessageEvent = _dereq_("./event").MessageEvent;

var specification = _dereq_("./specification/core.json");

function recoverActorDescriptions(error) {
  console.warn("Failed to fetch protocol specification (see reason below). " +
               "Using a fallback protocal specification!",
               error);
  return _dereq_("./specification/protocol.json");
}



var Supervisor = Class({
  constructor: function(id) {
    this.id = id;
    this.workers = [];
  }
});

var Telemetry = Class({
  add: function(id, ms) {
    console.log("telemetry::", id, ms)
  }
});



var Client = Class({
  constructor: function() {
    this.root = null;
    this.telemetry = new Telemetry();

    this.setupConnection();
    this.setupLifeManagement();
    this.setupTypeSystem();
  },

  setupConnection: function() {
    this.requests = [];
  },
  setupLifeManagement: function() {
    this.cache = Object.create(null);
    this.graph = Object.create(null);
    this.get = this.get.bind(this);
    this.release = this.release.bind(this);
  },
  setupTypeSystem: function() {
    this.typeSystem = new TypeSystem(this);
    this.typeSystem.registerTypes(specification);
  },

  connect: function(port) {
    var client = this;
    return new Promise(function(resolve, reject) {
      client.port = port;
      port.onmessage = client.receive.bind(client);
      client.onReady = resolve;
      client.onFail = reject;

      port.start();
    });
  },
  send: function(packet) {
    this.port.postMessage(packet);
  },
  request: function(packet) {
    var client = this;
    return new Promise(function(resolve, reject) {
      client.requests.push(packet.to, { resolve: resolve, reject: reject });
      client.send(packet);
    });
  },

  receive: function(event) {
    var packet = event.data;
    if (!this.root) {
      if (packet.from !== "root")
        throw Error("Initial packet must be from root");
      if (!("applicationType" in packet))
        throw Error("Initial packet must contain applicationType field");

      this.root = this.typeSystem.read("root", null, "root");
      this.root
          .protocolDescription()
          .catch(recoverActorDescriptions)
          .then(this.typeSystem.registerTypes.bind(this.typeSystem))
          .then(this.onReady.bind(this, this.root), this.onFail);
    } else {
      var actor = this.get(packet.from) || this.root;
      var event = actor.events[packet.type];
      if (event) {
        var message = new MessageEvent(packet.type, {
          data: event.read(packet)
        });
        actor.dispatchEvent(message);
      } else {
        var index = this.requests.indexOf(actor.id);
        if (index >= 0) {
          var request = this.requests.splice(index, 2).pop();
          if (packet.error)
            request.reject(packet);
          else
            request.resolve(packet);
        } else {
          console.error(Error("Unexpected packet " + JSON.stringify(packet, 2, 2)),
                        packet,
                        this.requests.slice(0));
        }
      }
    }
  },

  get: function(id) {
    return this.cache[id];
  },
  supervisorOf: function(actor) {
    for (var id in this.graph) {
      if (this.graph[id].indexOf(actor.id) >= 0) {
        return id;
      }
    }
  },
  workersOf: function(actor) {
    return this.graph[actor.id];
  },
  supervise: function(actor, worker) {
    var workers = this.workersOf(actor)
    if (workers.indexOf(worker.id) < 0) {
      workers.push(worker.id);
    }
  },
  unsupervise: function(actor, worker) {
    var workers = this.workersOf(actor);
    var index = workers.indexOf(worker.id)
    if (index >= 0) {
      workers.splice(index, 1)
    }
  },

  register: function(actor) {
    var registered = this.get(actor.id);
    if (!registered) {
      this.cache[actor.id] = actor;
      this.graph[actor.id] = [];
    } else if (registered !== actor) {
      throw new Error("Different actor with same id is already registered");
    }
  },
  unregister: function(actor) {
    if (this.get(actor.id)) {
      delete this.cache[actor.id];
      delete this.graph[actor.id];
    }
  },

  release: function(actor) {
    var supervisor = this.supervisorOf(actor);
    if (supervisor)
      this.unsupervise(supervisor, actor);

    var workers = this.workersOf(actor)

    if (workers) {
      workers.map(this.get).forEach(this.release)
    }
    this.unregister(actor);
  }
});
exports.Client = Client;

},{"./class":3,"./event":5,"./specification/core.json":23,"./specification/protocol.json":24,"./type-system":25,"./util":26,"es6-promise":2}],5:[function(_dereq_,module,exports){
"use strict";

var Symbol = _dereq_("es6-symbol")
var EventEmitter = _dereq_("events").EventEmitter;
var Class = _dereq_("./class").Class;

var $bound = Symbol("EventTarget/handleEvent");
var $emitter = Symbol("EventTarget/emitter");

function makeHandler(handler) {
  return function(event) {
    handler.handleEvent(event);
  }
}

var EventTarget = Class({
  constructor: function() {
    Object.defineProperty(this, $emitter, {
      enumerable: false,
      configurable: true,
      writable: true,
      value: new EventEmitter()
    });
  },
  addEventListener: function(type, handler) {
    if (typeof(handler) === "function") {
      this[$emitter].on(type, handler);
    }
    else if (handler && typeof(handler) === "object") {
      if (!handler[$bound]) handler[$bound] = makeHandler(handler);
      this[$emitter].on(type, handler[$bound]);
    }
  },
  removeEventListener: function(type, handler) {
    if (typeof(handler) === "function")
      this[$emitter].removeListener(type, handler);
    else if (handler && handler[$bound])
      this[$emitter].removeListener(type, handler[$bound]);
  },
  dispatchEvent: function(event) {
    event.target = this;
    this[$emitter].emit(event.type, event);
  }
});
exports.EventTarget = EventTarget;

var MessageEvent = Class({
  constructor: function(type, options) {
    options = options || {};
    this.type = type;
    this.data = options.data === void(0) ? null : options.data;

    this.lastEventId = options.lastEventId || "";
    this.origin = options.origin || "";
    this.bubbles = options.bubbles || false;
    this.cancelable = options.cancelable || false;
  },
  source: null,
  ports: null,
  preventDefault: function() {
  },
  stopPropagation: function() {
  },
  stopImmediatePropagation: function() {
  }
});
exports.MessageEvent = MessageEvent;

},{"./class":3,"es6-symbol":7,"events":6}],6:[function(_dereq_,module,exports){





















function EventEmitter() {
  this._events = this._events || {};
  this._maxListeners = this._maxListeners || undefined;
}
module.exports = EventEmitter;


EventEmitter.EventEmitter = EventEmitter;

EventEmitter.prototype._events = undefined;
EventEmitter.prototype._maxListeners = undefined;



EventEmitter.defaultMaxListeners = 10;



EventEmitter.prototype.setMaxListeners = function(n) {
  if (!isNumber(n) || n < 0 || isNaN(n))
    throw TypeError('n must be a positive number');
  this._maxListeners = n;
  return this;
};

EventEmitter.prototype.emit = function(type) {
  var er, handler, len, args, i, listeners;

  if (!this._events)
    this._events = {};

  
  if (type === 'error') {
    if (!this._events.error ||
        (isObject(this._events.error) && !this._events.error.length)) {
      er = arguments[1];
      if (er instanceof Error) {
        throw er; 
      } else {
        throw TypeError('Uncaught, unspecified "error" event.');
      }
      return false;
    }
  }

  handler = this._events[type];

  if (isUndefined(handler))
    return false;

  if (isFunction(handler)) {
    switch (arguments.length) {
      
      case 1:
        handler.call(this);
        break;
      case 2:
        handler.call(this, arguments[1]);
        break;
      case 3:
        handler.call(this, arguments[1], arguments[2]);
        break;
      
      default:
        len = arguments.length;
        args = new Array(len - 1);
        for (i = 1; i < len; i++)
          args[i - 1] = arguments[i];
        handler.apply(this, args);
    }
  } else if (isObject(handler)) {
    len = arguments.length;
    args = new Array(len - 1);
    for (i = 1; i < len; i++)
      args[i - 1] = arguments[i];

    listeners = handler.slice();
    len = listeners.length;
    for (i = 0; i < len; i++)
      listeners[i].apply(this, args);
  }

  return true;
};

EventEmitter.prototype.addListener = function(type, listener) {
  var m;

  if (!isFunction(listener))
    throw TypeError('listener must be a function');

  if (!this._events)
    this._events = {};

  
  
  if (this._events.newListener)
    this.emit('newListener', type,
              isFunction(listener.listener) ?
              listener.listener : listener);

  if (!this._events[type])
    
    this._events[type] = listener;
  else if (isObject(this._events[type]))
    
    this._events[type].push(listener);
  else
    
    this._events[type] = [this._events[type], listener];

  
  if (isObject(this._events[type]) && !this._events[type].warned) {
    var m;
    if (!isUndefined(this._maxListeners)) {
      m = this._maxListeners;
    } else {
      m = EventEmitter.defaultMaxListeners;
    }

    if (m && m > 0 && this._events[type].length > m) {
      this._events[type].warned = true;
      console.error('(node) warning: possible EventEmitter memory ' +
                    'leak detected. %d listeners added. ' +
                    'Use emitter.setMaxListeners() to increase limit.',
                    this._events[type].length);
      if (typeof console.trace === 'function') {
        
        console.trace();
      }
    }
  }

  return this;
};

EventEmitter.prototype.on = EventEmitter.prototype.addListener;

EventEmitter.prototype.once = function(type, listener) {
  if (!isFunction(listener))
    throw TypeError('listener must be a function');

  var fired = false;

  function g() {
    this.removeListener(type, g);

    if (!fired) {
      fired = true;
      listener.apply(this, arguments);
    }
  }

  g.listener = listener;
  this.on(type, g);

  return this;
};


EventEmitter.prototype.removeListener = function(type, listener) {
  var list, position, length, i;

  if (!isFunction(listener))
    throw TypeError('listener must be a function');

  if (!this._events || !this._events[type])
    return this;

  list = this._events[type];
  length = list.length;
  position = -1;

  if (list === listener ||
      (isFunction(list.listener) && list.listener === listener)) {
    delete this._events[type];
    if (this._events.removeListener)
      this.emit('removeListener', type, listener);

  } else if (isObject(list)) {
    for (i = length; i-- > 0;) {
      if (list[i] === listener ||
          (list[i].listener && list[i].listener === listener)) {
        position = i;
        break;
      }
    }

    if (position < 0)
      return this;

    if (list.length === 1) {
      list.length = 0;
      delete this._events[type];
    } else {
      list.splice(position, 1);
    }

    if (this._events.removeListener)
      this.emit('removeListener', type, listener);
  }

  return this;
};

EventEmitter.prototype.removeAllListeners = function(type) {
  var key, listeners;

  if (!this._events)
    return this;

  
  if (!this._events.removeListener) {
    if (arguments.length === 0)
      this._events = {};
    else if (this._events[type])
      delete this._events[type];
    return this;
  }

  
  if (arguments.length === 0) {
    for (key in this._events) {
      if (key === 'removeListener') continue;
      this.removeAllListeners(key);
    }
    this.removeAllListeners('removeListener');
    this._events = {};
    return this;
  }

  listeners = this._events[type];

  if (isFunction(listeners)) {
    this.removeListener(type, listeners);
  } else {
    
    while (listeners.length)
      this.removeListener(type, listeners[listeners.length - 1]);
  }
  delete this._events[type];

  return this;
};

EventEmitter.prototype.listeners = function(type) {
  var ret;
  if (!this._events || !this._events[type])
    ret = [];
  else if (isFunction(this._events[type]))
    ret = [this._events[type]];
  else
    ret = this._events[type].slice();
  return ret;
};

EventEmitter.listenerCount = function(emitter, type) {
  var ret;
  if (!emitter._events || !emitter._events[type])
    ret = 0;
  else if (isFunction(emitter._events[type]))
    ret = 1;
  else
    ret = emitter._events[type].length;
  return ret;
};

function isFunction(arg) {
  return typeof arg === 'function';
}

function isNumber(arg) {
  return typeof arg === 'number';
}

function isObject(arg) {
  return typeof arg === 'object' && arg !== null;
}

function isUndefined(arg) {
  return arg === void 0;
}

},{}],7:[function(_dereq_,module,exports){
'use strict';

module.exports = _dereq_('./is-implemented')() ? Symbol : _dereq_('./polyfill');

},{"./is-implemented":8,"./polyfill":22}],8:[function(_dereq_,module,exports){
'use strict';

module.exports = function () {
	var symbol;
	if (typeof Symbol !== 'function') return false;
	symbol = Symbol('test symbol');
	try {
		if (String(symbol) !== 'Symbol (test symbol)') return false;
	} catch (e) { return false; }
	if (typeof Symbol.iterator === 'symbol') return true;

	
	if (typeof Symbol.isConcatSpreadable !== 'object') return false;
	if (typeof Symbol.isRegExp !== 'object') return false;
	if (typeof Symbol.iterator !== 'object') return false;
	if (typeof Symbol.toPrimitive !== 'object') return false;
	if (typeof Symbol.toStringTag !== 'object') return false;
	if (typeof Symbol.unscopables !== 'object') return false;

	return true;
};

},{}],9:[function(_dereq_,module,exports){
'use strict';

var assign        = _dereq_('es5-ext/object/assign')
  , normalizeOpts = _dereq_('es5-ext/object/normalize-options')
  , isCallable    = _dereq_('es5-ext/object/is-callable')
  , contains      = _dereq_('es5-ext/string/#/contains')

  , d;

d = module.exports = function (dscr, value) {
	var c, e, w, options, desc;
	if ((arguments.length < 2) || (typeof dscr !== 'string')) {
		options = value;
		value = dscr;
		dscr = null;
	} else {
		options = arguments[2];
	}
	if (dscr == null) {
		c = w = true;
		e = false;
	} else {
		c = contains.call(dscr, 'c');
		e = contains.call(dscr, 'e');
		w = contains.call(dscr, 'w');
	}

	desc = { value: value, configurable: c, enumerable: e, writable: w };
	return !options ? desc : assign(normalizeOpts(options), desc);
};

d.gs = function (dscr, get, set) {
	var c, e, options, desc;
	if (typeof dscr !== 'string') {
		options = set;
		set = get;
		get = dscr;
		dscr = null;
	} else {
		options = arguments[3];
	}
	if (get == null) {
		get = undefined;
	} else if (!isCallable(get)) {
		options = get;
		get = set = undefined;
	} else if (set == null) {
		set = undefined;
	} else if (!isCallable(set)) {
		options = set;
		set = undefined;
	}
	if (dscr == null) {
		c = true;
		e = false;
	} else {
		c = contains.call(dscr, 'c');
		e = contains.call(dscr, 'e');
	}

	desc = { get: get, set: set, configurable: c, enumerable: e };
	return !options ? desc : assign(normalizeOpts(options), desc);
};

},{"es5-ext/object/assign":10,"es5-ext/object/is-callable":13,"es5-ext/object/normalize-options":17,"es5-ext/string/#/contains":19}],10:[function(_dereq_,module,exports){
'use strict';

module.exports = _dereq_('./is-implemented')()
	? Object.assign
	: _dereq_('./shim');

},{"./is-implemented":11,"./shim":12}],11:[function(_dereq_,module,exports){
'use strict';

module.exports = function () {
	var assign = Object.assign, obj;
	if (typeof assign !== 'function') return false;
	obj = { foo: 'raz' };
	assign(obj, { bar: 'dwa' }, { trzy: 'trzy' });
	return (obj.foo + obj.bar + obj.trzy) === 'razdwatrzy';
};

},{}],12:[function(_dereq_,module,exports){
'use strict';

var keys  = _dereq_('../keys')
  , value = _dereq_('../valid-value')

  , max = Math.max;

module.exports = function (dest, src) {
	var error, i, l = max(arguments.length, 2), assign;
	dest = Object(value(dest));
	assign = function (key) {
		try { dest[key] = src[key]; } catch (e) {
			if (!error) error = e;
		}
	};
	for (i = 1; i < l; ++i) {
		src = arguments[i];
		keys(src).forEach(assign);
	}
	if (error !== undefined) throw error;
	return dest;
};

},{"../keys":14,"../valid-value":18}],13:[function(_dereq_,module,exports){


'use strict';

module.exports = function (obj) { return typeof obj === 'function'; };

},{}],14:[function(_dereq_,module,exports){
'use strict';

module.exports = _dereq_('./is-implemented')()
	? Object.keys
	: _dereq_('./shim');

},{"./is-implemented":15,"./shim":16}],15:[function(_dereq_,module,exports){
'use strict';

module.exports = function () {
	try {
		Object.keys('primitive');
		return true;
	} catch (e) { return false; }
};

},{}],16:[function(_dereq_,module,exports){
'use strict';

var keys = Object.keys;

module.exports = function (object) {
	return keys(object == null ? object : Object(object));
};

},{}],17:[function(_dereq_,module,exports){
'use strict';

var assign = _dereq_('./assign')

  , forEach = Array.prototype.forEach
  , create = Object.create, getPrototypeOf = Object.getPrototypeOf

  , process;

process = function (src, obj) {
	var proto = getPrototypeOf(src);
	return assign(proto ? process(proto, obj) : obj, src);
};

module.exports = function (options) {
	var result = create(null);
	forEach.call(arguments, function (options) {
		if (options == null) return;
		process(Object(options), result);
	});
	return result;
};

},{"./assign":10}],18:[function(_dereq_,module,exports){
'use strict';

module.exports = function (value) {
	if (value == null) throw new TypeError("Cannot use null or undefined");
	return value;
};

},{}],19:[function(_dereq_,module,exports){
'use strict';

module.exports = _dereq_('./is-implemented')()
	? String.prototype.contains
	: _dereq_('./shim');

},{"./is-implemented":20,"./shim":21}],20:[function(_dereq_,module,exports){
'use strict';

var str = 'razdwatrzy';

module.exports = function () {
	if (typeof str.contains !== 'function') return false;
	return ((str.contains('dwa') === true) && (str.contains('foo') === false));
};

},{}],21:[function(_dereq_,module,exports){
'use strict';

var indexOf = String.prototype.indexOf;

module.exports = function (searchString) {
	return indexOf.call(this, searchString, arguments[1]) > -1;
};

},{}],22:[function(_dereq_,module,exports){
'use strict';

var d = _dereq_('d')

  , create = Object.create, defineProperties = Object.defineProperties
  , generateName, Symbol;

generateName = (function () {
	var created = create(null);
	return function (desc) {
		var postfix = 0;
		while (created[desc + (postfix || '')]) ++postfix;
		desc += (postfix || '');
		created[desc] = true;
		return '@@' + desc;
	};
}());

module.exports = Symbol = function (description) {
	var symbol;
	if (this instanceof Symbol) {
		throw new TypeError('TypeError: Symbol is not a constructor');
	}
	symbol = create(Symbol.prototype);
	description = (description === undefined ? '' : String(description));
	return defineProperties(symbol, {
		__description__: d('', description),
		__name__: d('', generateName(description))
	});
};

Object.defineProperties(Symbol, {
	create: d('', Symbol('create')),
	hasInstance: d('', Symbol('hasInstance')),
	isConcatSpreadable: d('', Symbol('isConcatSpreadable')),
	isRegExp: d('', Symbol('isRegExp')),
	iterator: d('', Symbol('iterator')),
	toPrimitive: d('', Symbol('toPrimitive')),
	toStringTag: d('', Symbol('toStringTag')),
	unscopables: d('', Symbol('unscopables'))
});

defineProperties(Symbol.prototype, {
	properToString: d(function () {
		return 'Symbol (' + this.__description__ + ')';
	}),
	toString: d('', function () { return this.__name__; })
});
Object.defineProperty(Symbol.prototype, Symbol.toPrimitive, d('',
	function (hint) {
		throw new TypeError("Conversion of symbol objects is not allowed");
	}));
Object.defineProperty(Symbol.prototype, Symbol.toStringTag, d('c', 'Symbol'));

},{"d":9}],23:[function(_dereq_,module,exports){
module.exports={
  "types": {
    "root": {
      "category": "actor",
      "typeName": "root",
      "methods": [
        {
          "name": "echo",
          "request": {
            "string": { "_arg": 0, "type": "string" }
          },
          "response": {
            "string": { "_retval": "string" }
          }
        },
        {
          "name": "listTabs",
          "request": {},
          "response": { "_retval": "tablist" }
        },
        {
          "name": "protocolDescription",
          "request": {},
          "response": { "_retval": "json" }
        }
      ],
      "events": {
        "tabListChanged": {}
      }
    },
    "tablist": {
      "category": "dict",
      "typeName": "tablist",
      "specializations": {
        "selected": "number",
        "tabs": "array:tab",
        "url": "string",
        "consoleActor": "console",
        "inspectorActor": "inspector",
        "styleSheetsActor": "stylesheets",
        "styleEditorActor": "styleeditor",
        "memoryActor": "memory",
        "eventLoopLagActor": "eventLoopLag",
        "preferenceActor": "preference",
        "deviceActor": "device",

        "profilerActor": "profiler",
        "chromeDebugger": "chromeDebugger",
        "webappsActor": "webapps"
      }
    },
    "tab": {
      "category": "actor",
      "typeName": "tab",
      "fields": {
        "title": "string",
        "url": "string",
        "outerWindowID": "number",
        "inspectorActor": "inspector",
        "callWatcherActor": "call-watcher",
        "canvasActor": "canvas",
        "webglActor": "webgl",
        "webaudioActor": "webaudio",
        "storageActor": "storage",
        "gcliActor": "gcli",
        "memoryActor": "memory",
        "eventLoopLag": "eventLoopLag",
        "styleSheetsActor": "stylesheets",
        "styleEditorActor": "styleeditor",

        "consoleActor": "console",
        "traceActor": "trace"
      },
      "methods": [
         {
          "name": "attach",
          "request": {},
          "response": { "_retval": "json" }
         }
      ],
      "events": {
        "tabNavigated": {
           "typeName": "tabNavigated"
        }
      }
    },
    "console": {
      "category": "actor",
      "typeName": "console",
      "methods": [
        {
          "name": "evaluateJS",
          "request": {
            "text": {
              "_option": 0,
              "type": "string"
            },
            "url": {
              "_option": 1,
              "type": "string"
            },
            "bindObjectActor": {
              "_option": 2,
              "type": "nullable:string"
            },
            "frameActor": {
              "_option": 2,
              "type": "nullable:string"
            },
            "selectedNodeActor": {
              "_option": 2,
              "type": "nullable:string"
            }
          },
          "response": {
            "_retval": "evaluatejsresponse"
          }
        }
      ],
      "events": {}
    },
    "evaluatejsresponse": {
      "category": "dict",
      "typeName": "evaluatejsresponse",
      "specializations": {
        "result": "object",
        "exception": "object",
        "exceptionMessage": "string",
        "input": "string"
      }
    },
    "object": {
      "category": "actor",
      "typeName": "object",
      "methods": [
         {
           "name": "property",
           "request": {
              "name": {
                "_arg": 0,
                "type": "string"
              }
           },
           "response": {
              "descriptor": {
                "_retval": "json"
              }
           }
         }
      ]
    }
  }
}

},{}],24:[function(_dereq_,module,exports){
module.exports={
  "types": {
    "longstractor": {
      "category": "actor",
      "typeName": "longstractor",
      "methods": [
        {
          "name": "substring",
          "request": {
            "type": "substring",
            "start": {
              "_arg": 0,
              "type": "primitive"
            },
            "end": {
              "_arg": 1,
              "type": "primitive"
            }
          },
          "response": {
            "substring": {
              "_retval": "primitive"
            }
          }
        },
        {
          "name": "release",
          "release": true,
          "request": {
            "type": "release"
          },
          "response": {}
        }
      ],
      "events": {}
    },
    "stylesheet": {
      "category": "actor",
      "typeName": "stylesheet",
      "methods": [
        {
          "name": "toggleDisabled",
          "request": {
            "type": "toggleDisabled"
          },
          "response": {
            "disabled": {
              "_retval": "boolean"
            }
          }
        },
        {
          "name": "getText",
          "request": {
            "type": "getText"
          },
          "response": {
            "text": {
              "_retval": "longstring"
            }
          }
        },
        {
          "name": "getOriginalSources",
          "request": {
            "type": "getOriginalSources"
          },
          "response": {
            "originalSources": {
              "_retval": "nullable:array:originalsource"
            }
          }
        },
        {
          "name": "getOriginalLocation",
          "request": {
            "type": "getOriginalLocation",
            "line": {
              "_arg": 0,
              "type": "number"
            },
            "column": {
              "_arg": 1,
              "type": "number"
            }
          },
          "response": {
            "_retval": "originallocationresponse"
          }
        },
        {
          "name": "update",
          "request": {
            "type": "update",
            "text": {
              "_arg": 0,
              "type": "string"
            },
            "transition": {
              "_arg": 1,
              "type": "boolean"
            }
          },
          "response": {}
        }
      ],
      "events": {
        "property-change": {
          "type": "propertyChange",
          "property": {
            "_arg": 0,
            "type": "string"
          },
          "value": {
            "_arg": 1,
            "type": "json"
          }
        },
        "style-applied": {
          "type": "styleApplied"
        }
      }
    },
    "originalsource": {
      "category": "actor",
      "typeName": "originalsource",
      "methods": [
        {
          "name": "getText",
          "request": {
            "type": "getText"
          },
          "response": {
            "text": {
              "_retval": "longstring"
            }
          }
        }
      ],
      "events": {}
    },
    "stylesheets": {
      "category": "actor",
      "typeName": "stylesheets",
      "methods": [
        {
          "name": "getStyleSheets",
          "request": {
            "type": "getStyleSheets"
          },
          "response": {
            "styleSheets": {
              "_retval": "array:stylesheet"
            }
          }
        },
        {
          "name": "addStyleSheet",
          "request": {
            "type": "addStyleSheet",
            "text": {
              "_arg": 0,
              "type": "string"
            }
          },
          "response": {
            "styleSheet": {
              "_retval": "stylesheet"
            }
          }
        }
      ],
      "events": {}
    },
    "originallocationresponse": {
      "category": "dict",
      "typeName": "originallocationresponse",
      "specializations": {
        "source": "string",
        "line": "number",
        "column": "number"
      }
    },
    "domnode": {
      "category": "actor",
      "typeName": "domnode",
      "methods": [
        {
          "name": "getNodeValue",
          "request": {
            "type": "getNodeValue"
          },
          "response": {
            "value": {
              "_retval": "longstring"
            }
          }
        },
        {
          "name": "setNodeValue",
          "request": {
            "type": "setNodeValue",
            "value": {
              "_arg": 0,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "getImageData",
          "request": {
            "type": "getImageData",
            "maxDim": {
              "_arg": 0,
              "type": "nullable:number"
            }
          },
          "response": {
            "_retval": "imageData"
          }
        },
        {
          "name": "modifyAttributes",
          "request": {
            "type": "modifyAttributes",
            "modifications": {
              "_arg": 0,
              "type": "array:json"
            }
          },
          "response": {}
        }
      ],
      "events": {}
    },
    "appliedstyle": {
      "category": "dict",
      "typeName": "appliedstyle",
      "specializations": {
        "rule": "domstylerule#actorid",
        "inherited": "nullable:domnode#actorid"
      }
    },
    "matchedselector": {
      "category": "dict",
      "typeName": "matchedselector",
      "specializations": {
        "rule": "domstylerule#actorid",
        "selector": "string",
        "value": "string",
        "status": "number"
      }
    },
    "matchedselectorresponse": {
      "category": "dict",
      "typeName": "matchedselectorresponse",
      "specializations": {
        "rules": "array:domstylerule",
        "sheets": "array:stylesheet",
        "matched": "array:matchedselector"
      }
    },
    "appliedStylesReturn": {
      "category": "dict",
      "typeName": "appliedStylesReturn",
      "specializations": {
        "entries": "array:appliedstyle",
        "rules": "array:domstylerule",
        "sheets": "array:stylesheet"
      }
    },
    "pagestyle": {
      "category": "actor",
      "typeName": "pagestyle",
      "methods": [
        {
          "name": "getComputed",
          "request": {
            "type": "getComputed",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "markMatched": {
              "_option": 1,
              "type": "boolean"
            },
            "onlyMatched": {
              "_option": 1,
              "type": "boolean"
            },
            "filter": {
              "_option": 1,
              "type": "string"
            }
          },
          "response": {
            "computed": {
              "_retval": "json"
            }
          }
        },
        {
          "name": "getMatchedSelectors",
          "request": {
            "type": "getMatchedSelectors",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "property": {
              "_arg": 1,
              "type": "string"
            },
            "filter": {
              "_option": 2,
              "type": "string"
            }
          },
          "response": {
            "_retval": "matchedselectorresponse"
          }
        },
        {
          "name": "getApplied",
          "request": {
            "type": "getApplied",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "inherited": {
              "_option": 1,
              "type": "boolean"
            },
            "matchedSelectors": {
              "_option": 1,
              "type": "boolean"
            },
            "filter": {
              "_option": 1,
              "type": "string"
            }
          },
          "response": {
            "_retval": "appliedStylesReturn"
          }
        },
        {
          "name": "getLayout",
          "request": {
            "type": "getLayout",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "autoMargins": {
              "_option": 1,
              "type": "boolean"
            }
          },
          "response": {
            "_retval": "json"
          }
        }
      ],
      "events": {}
    },
    "domstylerule": {
      "category": "actor",
      "typeName": "domstylerule",
      "methods": [
        {
          "name": "modifyProperties",
          "request": {
            "type": "modifyProperties",
            "modifications": {
              "_arg": 0,
              "type": "array:json"
            }
          },
          "response": {
            "rule": {
              "_retval": "domstylerule"
            }
          }
        }
      ],
      "events": {}
    },
    "highlighter": {
      "category": "actor",
      "typeName": "highlighter",
      "methods": [
        {
          "name": "showBoxModel",
          "request": {
            "type": "showBoxModel",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "region": {
              "_option": 1,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "hideBoxModel",
          "request": {
            "type": "hideBoxModel"
          },
          "response": {}
        },
        {
          "name": "pick",
          "request": {
            "type": "pick"
          },
          "response": {}
        },
        {
          "name": "cancelPick",
          "request": {
            "type": "cancelPick"
          },
          "response": {}
        }
      ],
      "events": {}
    },
    "imageData": {
      "category": "dict",
      "typeName": "imageData",
      "specializations": {
        "data": "nullable:longstring",
        "size": "json"
      }
    },
    "disconnectedNode": {
      "category": "dict",
      "typeName": "disconnectedNode",
      "specializations": {
        "node": "domnode",
        "newParents": "array:domnode"
      }
    },
    "disconnectedNodeArray": {
      "category": "dict",
      "typeName": "disconnectedNodeArray",
      "specializations": {
        "nodes": "array:domnode",
        "newParents": "array:domnode"
      }
    },
    "dommutation": {
      "category": "dict",
      "typeName": "dommutation",
      "specializations": {}
    },
    "domnodelist": {
      "category": "actor",
      "typeName": "domnodelist",
      "methods": [
        {
          "name": "item",
          "request": {
            "type": "item",
            "item": {
              "_arg": 0,
              "type": "primitive"
            }
          },
          "response": {
            "_retval": "disconnectedNode"
          }
        },
        {
          "name": "items",
          "request": {
            "type": "items",
            "start": {
              "_arg": 0,
              "type": "nullable:number"
            },
            "end": {
              "_arg": 1,
              "type": "nullable:number"
            }
          },
          "response": {
            "_retval": "disconnectedNodeArray"
          }
        },
        {
          "name": "release",
          "release": true,
          "request": {
            "type": "release"
          },
          "response": {}
        }
      ],
      "events": {}
    },
    "domtraversalarray": {
      "category": "dict",
      "typeName": "domtraversalarray",
      "specializations": {
        "nodes": "array:domnode"
      }
    },
    "domwalker": {
      "category": "actor",
      "typeName": "domwalker",
      "methods": [
        {
          "name": "release",
          "release": true,
          "request": {
            "type": "release"
          },
          "response": {}
        },
        {
          "name": "pick",
          "request": {
            "type": "pick"
          },
          "response": {
            "_retval": "disconnectedNode"
          }
        },
        {
          "name": "cancelPick",
          "request": {
            "type": "cancelPick"
          },
          "response": {}
        },
        {
          "name": "highlight",
          "request": {
            "type": "highlight",
            "node": {
              "_arg": 0,
              "type": "nullable:domnode"
            }
          },
          "response": {}
        },
        {
          "name": "document",
          "request": {
            "type": "document",
            "node": {
              "_arg": 0,
              "type": "nullable:domnode"
            }
          },
          "response": {
            "node": {
              "_retval": "domnode"
            }
          }
        },
        {
          "name": "documentElement",
          "request": {
            "type": "documentElement",
            "node": {
              "_arg": 0,
              "type": "nullable:domnode"
            }
          },
          "response": {
            "node": {
              "_retval": "domnode"
            }
          }
        },
        {
          "name": "parents",
          "request": {
            "type": "parents",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "sameDocument": {
              "_option": 1,
              "type": "primitive"
            }
          },
          "response": {
            "nodes": {
              "_retval": "array:domnode"
            }
          }
        },
        {
          "name": "retainNode",
          "request": {
            "type": "retainNode",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {}
        },
        {
          "name": "unretainNode",
          "request": {
            "type": "unretainNode",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {}
        },
        {
          "name": "releaseNode",
          "request": {
            "type": "releaseNode",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "force": {
              "_option": 1,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "children",
          "request": {
            "type": "children",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "maxNodes": {
              "_option": 1,
              "type": "primitive"
            },
            "center": {
              "_option": 1,
              "type": "domnode"
            },
            "start": {
              "_option": 1,
              "type": "domnode"
            },
            "whatToShow": {
              "_option": 1,
              "type": "primitive"
            }
          },
          "response": {
            "_retval": "domtraversalarray"
          }
        },
        {
          "name": "siblings",
          "request": {
            "type": "siblings",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "maxNodes": {
              "_option": 1,
              "type": "primitive"
            },
            "center": {
              "_option": 1,
              "type": "domnode"
            },
            "start": {
              "_option": 1,
              "type": "domnode"
            },
            "whatToShow": {
              "_option": 1,
              "type": "primitive"
            }
          },
          "response": {
            "_retval": "domtraversalarray"
          }
        },
        {
          "name": "nextSibling",
          "request": {
            "type": "nextSibling",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "whatToShow": {
              "_option": 1,
              "type": "primitive"
            }
          },
          "response": {
            "node": {
              "_retval": "nullable:domnode"
            }
          }
        },
        {
          "name": "previousSibling",
          "request": {
            "type": "previousSibling",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "whatToShow": {
              "_option": 1,
              "type": "primitive"
            }
          },
          "response": {
            "node": {
              "_retval": "nullable:domnode"
            }
          }
        },
        {
          "name": "querySelector",
          "request": {
            "type": "querySelector",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "selector": {
              "_arg": 1,
              "type": "primitive"
            }
          },
          "response": {
            "_retval": "disconnectedNode"
          }
        },
        {
          "name": "querySelectorAll",
          "request": {
            "type": "querySelectorAll",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "selector": {
              "_arg": 1,
              "type": "primitive"
            }
          },
          "response": {
            "list": {
              "_retval": "domnodelist"
            }
          }
        },
        {
          "name": "getSuggestionsForQuery",
          "request": {
            "type": "getSuggestionsForQuery",
            "query": {
              "_arg": 0,
              "type": "primitive"
            },
            "completing": {
              "_arg": 1,
              "type": "primitive"
            },
            "selectorState": {
              "_arg": 2,
              "type": "primitive"
            }
          },
          "response": {
            "list": {
              "_retval": "array:array:string"
            }
          }
        },
        {
          "name": "addPseudoClassLock",
          "request": {
            "type": "addPseudoClassLock",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "pseudoClass": {
              "_arg": 1,
              "type": "primitive"
            },
            "parents": {
              "_option": 2,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "hideNode",
          "request": {
            "type": "hideNode",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {}
        },
        {
          "name": "unhideNode",
          "request": {
            "type": "unhideNode",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {}
        },
        {
          "name": "removePseudoClassLock",
          "request": {
            "type": "removePseudoClassLock",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "pseudoClass": {
              "_arg": 1,
              "type": "primitive"
            },
            "parents": {
              "_option": 2,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "clearPseudoClassLocks",
          "request": {
            "type": "clearPseudoClassLocks",
            "node": {
              "_arg": 0,
              "type": "nullable:domnode"
            }
          },
          "response": {}
        },
        {
          "name": "innerHTML",
          "request": {
            "type": "innerHTML",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {
            "value": {
              "_retval": "longstring"
            }
          }
        },
        {
          "name": "outerHTML",
          "request": {
            "type": "outerHTML",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {
            "value": {
              "_retval": "longstring"
            }
          }
        },
        {
          "name": "setOuterHTML",
          "request": {
            "type": "setOuterHTML",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "value": {
              "_arg": 1,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "removeNode",
          "request": {
            "type": "removeNode",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {
            "nextSibling": {
              "_retval": "nullable:domnode"
            }
          }
        },
        {
          "name": "insertBefore",
          "request": {
            "type": "insertBefore",
            "node": {
              "_arg": 0,
              "type": "domnode"
            },
            "parent": {
              "_arg": 1,
              "type": "domnode"
            },
            "sibling": {
              "_arg": 2,
              "type": "nullable:domnode"
            }
          },
          "response": {}
        },
        {
          "name": "getMutations",
          "request": {
            "type": "getMutations",
            "cleanup": {
              "_option": 0,
              "type": "primitive"
            }
          },
          "response": {
            "mutations": {
              "_retval": "array:dommutation"
            }
          }
        },
        {
          "name": "isInDOMTree",
          "request": {
            "type": "isInDOMTree",
            "node": {
              "_arg": 0,
              "type": "domnode"
            }
          },
          "response": {
            "attached": {
              "_retval": "boolean"
            }
          }
        },
        {
          "name": "getNodeActorFromObjectActor",
          "request": {
            "type": "getNodeActorFromObjectActor",
            "objectActorID": {
              "_arg": 0,
              "type": "string"
            }
          },
          "response": {
            "nodeFront": {
              "_retval": "nullable:disconnectedNode"
            }
          }
        }
      ],
      "events": {
        "new-mutations": {
          "type": "newMutations"
        },
        "picker-node-picked": {
          "type": "pickerNodePicked",
          "node": {
            "_arg": 0,
            "type": "disconnectedNode"
          }
        },
        "picker-node-hovered": {
          "type": "pickerNodeHovered",
          "node": {
            "_arg": 0,
            "type": "disconnectedNode"
          }
        },
        "highlighter-ready": {
          "type": "highlighter-ready"
        },
        "highlighter-hide": {
          "type": "highlighter-hide"
        }
      }
    },
    "inspector": {
      "category": "actor",
      "typeName": "inspector",
      "methods": [
        {
          "name": "getWalker",
          "request": {
            "type": "getWalker"
          },
          "response": {
            "walker": {
              "_retval": "domwalker"
            }
          }
        },
        {
          "name": "getPageStyle",
          "request": {
            "type": "getPageStyle"
          },
          "response": {
            "pageStyle": {
              "_retval": "pagestyle"
            }
          }
        },
        {
          "name": "getHighlighter",
          "request": {
            "type": "getHighlighter",
            "autohide": {
              "_arg": 0,
              "type": "boolean"
            }
          },
          "response": {
            "highligter": {
              "_retval": "highlighter"
            }
          }
        },
        {
          "name": "getImageDataFromURL",
          "request": {
            "type": "getImageDataFromURL",
            "url": {
              "_arg": 0,
              "type": "primitive"
            },
            "maxDim": {
              "_arg": 1,
              "type": "nullable:number"
            }
          },
          "response": {
            "_retval": "imageData"
          }
        }
      ],
      "events": {}
    },
    "call-stack-item": {
      "category": "dict",
      "typeName": "call-stack-item",
      "specializations": {
        "name": "string",
        "file": "string",
        "line": "number"
      }
    },
    "call-details": {
      "category": "dict",
      "typeName": "call-details",
      "specializations": {
        "type": "number",
        "name": "string",
        "stack": "array:call-stack-item"
      }
    },
    "function-call": {
      "category": "actor",
      "typeName": "function-call",
      "methods": [
        {
          "name": "getDetails",
          "request": {
            "type": "getDetails"
          },
          "response": {
            "info": {
              "_retval": "call-details"
            }
          }
        }
      ],
      "events": {}
    },
    "call-watcher": {
      "category": "actor",
      "typeName": "call-watcher",
      "methods": [
        {
          "name": "setup",
          "oneway": true,
          "request": {
            "type": "setup",
            "tracedGlobals": {
              "_option": 0,
              "type": "nullable:array:string"
            },
            "tracedFunctions": {
              "_option": 0,
              "type": "nullable:array:string"
            },
            "startRecording": {
              "_option": 0,
              "type": "boolean"
            },
            "performReload": {
              "_option": 0,
              "type": "boolean"
            }
          },
          "response": {}
        },
        {
          "name": "finalize",
          "oneway": true,
          "request": {
            "type": "finalize"
          },
          "response": {}
        },
        {
          "name": "isRecording",
          "request": {
            "type": "isRecording"
          },
          "response": {
            "_retval": "boolean"
          }
        },
        {
          "name": "resumeRecording",
          "request": {
            "type": "resumeRecording"
          },
          "response": {}
        },
        {
          "name": "pauseRecording",
          "request": {
            "type": "pauseRecording"
          },
          "response": {
            "calls": {
              "_retval": "array:function-call"
            }
          }
        },
        {
          "name": "eraseRecording",
          "request": {
            "type": "eraseRecording"
          },
          "response": {}
        }
      ],
      "events": {}
    },
    "snapshot-image": {
      "category": "dict",
      "typeName": "snapshot-image",
      "specializations": {
        "index": "number",
        "width": "number",
        "height": "number",
        "flipped": "boolean",
        "pixels": "uint32-array"
      }
    },
    "snapshot-overview": {
      "category": "dict",
      "typeName": "snapshot-overview",
      "specializations": {
        "calls": "array:function-call",
        "thumbnails": "array:snapshot-image",
        "screenshot": "snapshot-image"
      }
    },
    "frame-snapshot": {
      "category": "actor",
      "typeName": "frame-snapshot",
      "methods": [
        {
          "name": "getOverview",
          "request": {
            "type": "getOverview"
          },
          "response": {
            "overview": {
              "_retval": "snapshot-overview"
            }
          }
        },
        {
          "name": "generateScreenshotFor",
          "request": {
            "type": "generateScreenshotFor",
            "call": {
              "_arg": 0,
              "type": "function-call"
            }
          },
          "response": {
            "screenshot": {
              "_retval": "snapshot-image"
            }
          }
        }
      ],
      "events": {}
    },
    "canvas": {
      "category": "actor",
      "typeName": "canvas",
      "methods": [
        {
          "name": "setup",
          "oneway": true,
          "request": {
            "type": "setup",
            "reload": {
              "_option": 0,
              "type": "boolean"
            }
          },
          "response": {}
        },
        {
          "name": "finalize",
          "oneway": true,
          "request": {
            "type": "finalize"
          },
          "response": {}
        },
        {
          "name": "isInitialized",
          "request": {
            "type": "isInitialized"
          },
          "response": {
            "initialized": {
              "_retval": "boolean"
            }
          }
        },
        {
          "name": "recordAnimationFrame",
          "request": {
            "type": "recordAnimationFrame"
          },
          "response": {
            "snapshot": {
              "_retval": "frame-snapshot"
            }
          }
        }
      ],
      "events": {}
    },
    "gl-shader": {
      "category": "actor",
      "typeName": "gl-shader",
      "methods": [
        {
          "name": "getText",
          "request": {
            "type": "getText"
          },
          "response": {
            "text": {
              "_retval": "string"
            }
          }
        },
        {
          "name": "compile",
          "request": {
            "type": "compile",
            "text": {
              "_arg": 0,
              "type": "string"
            }
          },
          "response": {
            "error": {
              "_retval": "nullable:json"
            }
          }
        }
      ],
      "events": {}
    },
    "gl-program": {
      "category": "actor",
      "typeName": "gl-program",
      "methods": [
        {
          "name": "getVertexShader",
          "request": {
            "type": "getVertexShader"
          },
          "response": {
            "shader": {
              "_retval": "gl-shader"
            }
          }
        },
        {
          "name": "getFragmentShader",
          "request": {
            "type": "getFragmentShader"
          },
          "response": {
            "shader": {
              "_retval": "gl-shader"
            }
          }
        },
        {
          "name": "highlight",
          "oneway": true,
          "request": {
            "type": "highlight",
            "tint": {
              "_arg": 0,
              "type": "array:number"
            }
          },
          "response": {}
        },
        {
          "name": "unhighlight",
          "oneway": true,
          "request": {
            "type": "unhighlight"
          },
          "response": {}
        },
        {
          "name": "blackbox",
          "oneway": true,
          "request": {
            "type": "blackbox"
          },
          "response": {}
        },
        {
          "name": "unblackbox",
          "oneway": true,
          "request": {
            "type": "unblackbox"
          },
          "response": {}
        }
      ],
      "events": {}
    },
    "webgl": {
      "category": "actor",
      "typeName": "webgl",
      "methods": [
        {
          "name": "setup",
          "oneway": true,
          "request": {
            "type": "setup",
            "reload": {
              "_option": 0,
              "type": "boolean"
            }
          },
          "response": {}
        },
        {
          "name": "finalize",
          "oneway": true,
          "request": {
            "type": "finalize"
          },
          "response": {}
        },
        {
          "name": "getPrograms",
          "request": {
            "type": "getPrograms"
          },
          "response": {
            "programs": {
              "_retval": "array:gl-program"
            }
          }
        }
      ],
      "events": {
        "program-linked": {
          "type": "programLinked",
          "program": {
            "_arg": 0,
            "type": "gl-program"
          }
        }
      }
    },
    "audionode": {
      "category": "actor",
      "typeName": "audionode",
      "methods": [
        {
          "name": "getType",
          "request": {
            "type": "getType"
          },
          "response": {
            "type": {
              "_retval": "string"
            }
          }
        },
        {
          "name": "isSource",
          "request": {
            "type": "isSource"
          },
          "response": {
            "source": {
              "_retval": "boolean"
            }
          }
        },
        {
          "name": "setParam",
          "request": {
            "type": "setParam",
            "param": {
              "_arg": 0,
              "type": "string"
            },
            "value": {
              "_arg": 1,
              "type": "nullable:primitive"
            }
          },
          "response": {
            "error": {
              "_retval": "nullable:json"
            }
          }
        },
        {
          "name": "getParam",
          "request": {
            "type": "getParam",
            "param": {
              "_arg": 0,
              "type": "string"
            }
          },
          "response": {
            "text": {
              "_retval": "nullable:primitive"
            }
          }
        },
        {
          "name": "getParamFlags",
          "request": {
            "type": "getParamFlags",
            "param": {
              "_arg": 0,
              "type": "string"
            }
          },
          "response": {
            "flags": {
              "_retval": "nullable:primitive"
            }
          }
        },
        {
          "name": "getParams",
          "request": {
            "type": "getParams"
          },
          "response": {
            "params": {
              "_retval": "json"
            }
          }
        }
      ],
      "events": {}
    },
    "webaudio": {
      "category": "actor",
      "typeName": "webaudio",
      "methods": [
        {
          "name": "setup",
          "oneway": true,
          "request": {
            "type": "setup",
            "reload": {
              "_option": 0,
              "type": "boolean"
            }
          },
          "response": {}
        },
        {
          "name": "finalize",
          "oneway": true,
          "request": {
            "type": "finalize"
          },
          "response": {}
        }
      ],
      "events": {
        "start-context": {
          "type": "startContext"
        },
        "connect-node": {
          "type": "connectNode",
          "source": {
            "_option": 0,
            "type": "audionode"
          },
          "dest": {
            "_option": 0,
            "type": "audionode"
          }
        },
        "disconnect-node": {
          "type": "disconnectNode",
          "source": {
            "_arg": 0,
            "type": "audionode"
          }
        },
        "connect-param": {
          "type": "connectParam",
          "source": {
            "_arg": 0,
            "type": "audionode"
          },
          "param": {
            "_arg": 1,
            "type": "string"
          }
        },
        "change-param": {
          "type": "changeParam",
          "source": {
            "_option": 0,
            "type": "audionode"
          },
          "param": {
            "_option": 0,
            "type": "string"
          },
          "value": {
            "_option": 0,
            "type": "string"
          }
        },
        "create-node": {
          "type": "createNode",
          "source": {
            "_arg": 0,
            "type": "audionode"
          }
        }
      }
    },
    "old-stylesheet": {
      "category": "actor",
      "typeName": "old-stylesheet",
      "methods": [
        {
          "name": "toggleDisabled",
          "request": {
            "type": "toggleDisabled"
          },
          "response": {
            "disabled": {
              "_retval": "boolean"
            }
          }
        },
        {
          "name": "fetchSource",
          "request": {
            "type": "fetchSource"
          },
          "response": {}
        },
        {
          "name": "update",
          "request": {
            "type": "update",
            "text": {
              "_arg": 0,
              "type": "string"
            },
            "transition": {
              "_arg": 1,
              "type": "boolean"
            }
          },
          "response": {}
        }
      ],
      "events": {
        "property-change": {
          "type": "propertyChange",
          "property": {
            "_arg": 0,
            "type": "string"
          },
          "value": {
            "_arg": 1,
            "type": "json"
          }
        },
        "source-load": {
          "type": "sourceLoad",
          "source": {
            "_arg": 0,
            "type": "string"
          }
        },
        "style-applied": {
          "type": "styleApplied"
        }
      }
    },
    "styleeditor": {
      "category": "actor",
      "typeName": "styleeditor",
      "methods": [
        {
          "name": "newDocument",
          "request": {
            "type": "newDocument"
          },
          "response": {}
        },
        {
          "name": "newStyleSheet",
          "request": {
            "type": "newStyleSheet",
            "text": {
              "_arg": 0,
              "type": "string"
            }
          },
          "response": {
            "styleSheet": {
              "_retval": "old-stylesheet"
            }
          }
        }
      ],
      "events": {
        "document-load": {
          "type": "documentLoad",
          "styleSheets": {
            "_arg": 0,
            "type": "array:old-stylesheet"
          }
        }
      }
    },
    "cookieobject": {
      "category": "dict",
      "typeName": "cookieobject",
      "specializations": {
        "name": "string",
        "value": "longstring",
        "path": "nullable:string",
        "host": "string",
        "isDomain": "boolean",
        "isSecure": "boolean",
        "isHttpOnly": "boolean",
        "creationTime": "number",
        "lastAccessed": "number",
        "expires": "number"
      }
    },
    "cookiestoreobject": {
      "category": "dict",
      "typeName": "cookiestoreobject",
      "specializations": {
        "total": "number",
        "offset": "number",
        "data": "array:nullable:cookieobject"
      }
    },
    "storageobject": {
      "category": "dict",
      "typeName": "storageobject",
      "specializations": {
        "name": "string",
        "value": "longstring"
      }
    },
    "storagestoreobject": {
      "category": "dict",
      "typeName": "storagestoreobject",
      "specializations": {
        "total": "number",
        "offset": "number",
        "data": "array:nullable:storageobject"
      }
    },
    "idbobject": {
      "category": "dict",
      "typeName": "idbobject",
      "specializations": {
        "name": "nullable:string",
        "db": "nullable:string",
        "objectStore": "nullable:string",
        "origin": "nullable:string",
        "version": "nullable:number",
        "objectStores": "nullable:number",
        "keyPath": "nullable:string",
        "autoIncrement": "nullable:boolean",
        "indexes": "nullable:string",
        "value": "nullable:longstring"
      }
    },
    "idbstoreobject": {
      "category": "dict",
      "typeName": "idbstoreobject",
      "specializations": {
        "total": "number",
        "offset": "number",
        "data": "array:nullable:idbobject"
      }
    },
    "storeUpdateObject": {
      "category": "dict",
      "typeName": "storeUpdateObject",
      "specializations": {
        "changed": "nullable:json",
        "deleted": "nullable:json",
        "added": "nullable:json"
      }
    },
    "cookies": {
      "category": "actor",
      "typeName": "cookies",
      "methods": [
        {
          "name": "getStoreObjects",
          "request": {
            "type": "getStoreObjects",
            "host": {
              "_arg": 0,
              "type": "primitive"
            },
            "names": {
              "_arg": 1,
              "type": "nullable:array:string"
            },
            "options": {
              "_arg": 2,
              "type": "nullable:json"
            }
          },
          "response": {
            "_retval": "cookiestoreobject"
          }
        }
      ],
      "events": {}
    },
    "localStorage": {
      "category": "actor",
      "typeName": "localStorage",
      "methods": [
        {
          "name": "getStoreObjects",
          "request": {
            "type": "getStoreObjects",
            "host": {
              "_arg": 0,
              "type": "primitive"
            },
            "names": {
              "_arg": 1,
              "type": "nullable:array:string"
            },
            "options": {
              "_arg": 2,
              "type": "nullable:json"
            }
          },
          "response": {
            "_retval": "storagestoreobject"
          }
        }
      ],
      "events": {}
    },
    "sessionStorage": {
      "category": "actor",
      "typeName": "sessionStorage",
      "methods": [
        {
          "name": "getStoreObjects",
          "request": {
            "type": "getStoreObjects",
            "host": {
              "_arg": 0,
              "type": "primitive"
            },
            "names": {
              "_arg": 1,
              "type": "nullable:array:string"
            },
            "options": {
              "_arg": 2,
              "type": "nullable:json"
            }
          },
          "response": {
            "_retval": "storagestoreobject"
          }
        }
      ],
      "events": {}
    },
    "indexedDB": {
      "category": "actor",
      "typeName": "indexedDB",
      "methods": [
        {
          "name": "getStoreObjects",
          "request": {
            "type": "getStoreObjects",
            "host": {
              "_arg": 0,
              "type": "primitive"
            },
            "names": {
              "_arg": 1,
              "type": "nullable:array:string"
            },
            "options": {
              "_arg": 2,
              "type": "nullable:json"
            }
          },
          "response": {
            "_retval": "idbstoreobject"
          }
        }
      ],
      "events": {}
    },
    "storelist": {
      "category": "dict",
      "typeName": "storelist",
      "specializations": {
        "cookies": "cookies",
        "localStorage": "localStorage",
        "sessionStorage": "sessionStorage",
        "indexedDB": "indexedDB"
      }
    },
    "storage": {
      "category": "actor",
      "typeName": "storage",
      "methods": [
        {
          "name": "listStores",
          "request": {
            "type": "listStores"
          },
          "response": {
            "_retval": "storelist"
          }
        }
      ],
      "events": {
        "stores-update": {
          "type": "storesUpdate",
          "data": {
            "_arg": 0,
            "type": "storeUpdateObject"
          }
        },
        "stores-cleared": {
          "type": "storesCleared",
          "data": {
            "_arg": 0,
            "type": "json"
          }
        },
        "stores-reloaded": {
          "type": "storesRelaoded",
          "data": {
            "_arg": 0,
            "type": "json"
          }
        }
      }
    },
    "gcli": {
      "category": "actor",
      "typeName": "gcli",
      "methods": [
        {
          "name": "specs",
          "request": {
            "type": "specs"
          },
          "response": {
            "_retval": "json"
          }
        },
        {
          "name": "execute",
          "request": {
            "type": "execute",
            "typed": {
              "_arg": 0,
              "type": "string"
            }
          },
          "response": {
            "_retval": "json"
          }
        },
        {
          "name": "state",
          "request": {
            "type": "state",
            "typed": {
              "_arg": 0,
              "type": "string"
            },
            "start": {
              "_arg": 1,
              "type": "number"
            },
            "rank": {
              "_arg": 2,
              "type": "number"
            }
          },
          "response": {
            "_retval": "json"
          }
        },
        {
          "name": "typeparse",
          "request": {
            "type": "typeparse",
            "typed": {
              "_arg": 0,
              "type": "string"
            },
            "param": {
              "_arg": 1,
              "type": "string"
            }
          },
          "response": {
            "_retval": "json"
          }
        },
        {
          "name": "typeincrement",
          "request": {
            "type": "typeincrement",
            "typed": {
              "_arg": 0,
              "type": "string"
            },
            "param": {
              "_arg": 1,
              "type": "string"
            }
          },
          "response": {
            "_retval": "string"
          }
        },
        {
          "name": "typedecrement",
          "request": {
            "type": "typedecrement",
            "typed": {
              "_arg": 0,
              "type": "string"
            },
            "param": {
              "_arg": 1,
              "type": "string"
            }
          },
          "response": {
            "_retval": "string"
          }
        },
        {
          "name": "selectioninfo",
          "request": {
            "type": "selectioninfo",
            "typed": {
              "_arg": 0,
              "type": "string"
            },
            "param": {
              "_arg": 1,
              "type": "string"
            },
            "action": {
              "_arg": 1,
              "type": "string"
            }
          },
          "response": {
            "_retval": "json"
          }
        }
      ],
      "events": {}
    },
    "memory": {
      "category": "actor",
      "typeName": "memory",
      "methods": [
        {
          "name": "measure",
          "request": {
            "type": "measure"
          },
          "response": {
            "_retval": "json"
          }
        }
      ],
      "events": {}
    },
    "eventLoopLag": {
      "category": "actor",
      "typeName": "eventLoopLag",
      "methods": [
        {
          "name": "start",
          "request": {
            "type": "start"
          },
          "response": {
            "success": {
              "_retval": "number"
            }
          }
        },
        {
          "name": "stop",
          "request": {
            "type": "stop"
          },
          "response": {}
        }
      ],
      "events": {
        "event-loop-lag": {
          "type": "event-loop-lag",
          "time": {
            "_arg": 0,
            "type": "number"
          }
        }
      }
    },
    "preference": {
      "category": "actor",
      "typeName": "preference",
      "methods": [
        {
          "name": "getBoolPref",
          "request": {
            "type": "getBoolPref",
            "value": {
              "_arg": 0,
              "type": "primitive"
            }
          },
          "response": {
            "value": {
              "_retval": "boolean"
            }
          }
        },
        {
          "name": "getCharPref",
          "request": {
            "type": "getCharPref",
            "value": {
              "_arg": 0,
              "type": "primitive"
            }
          },
          "response": {
            "value": {
              "_retval": "string"
            }
          }
        },
        {
          "name": "getIntPref",
          "request": {
            "type": "getIntPref",
            "value": {
              "_arg": 0,
              "type": "primitive"
            }
          },
          "response": {
            "value": {
              "_retval": "number"
            }
          }
        },
        {
          "name": "getAllPrefs",
          "request": {
            "type": "getAllPrefs"
          },
          "response": {
            "value": {
              "_retval": "json"
            }
          }
        },
        {
          "name": "setBoolPref",
          "request": {
            "type": "setBoolPref",
            "name": {
              "_arg": 0,
              "type": "primitive"
            },
            "value": {
              "_arg": 1,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "setCharPref",
          "request": {
            "type": "setCharPref",
            "name": {
              "_arg": 0,
              "type": "primitive"
            },
            "value": {
              "_arg": 1,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "setIntPref",
          "request": {
            "type": "setIntPref",
            "name": {
              "_arg": 0,
              "type": "primitive"
            },
            "value": {
              "_arg": 1,
              "type": "primitive"
            }
          },
          "response": {}
        },
        {
          "name": "clearUserPref",
          "request": {
            "type": "clearUserPref",
            "name": {
              "_arg": 0,
              "type": "primitive"
            }
          },
          "response": {}
        }
      ],
      "events": {}
    },
    "device": {
      "category": "actor",
      "typeName": "device",
      "methods": [
        {
          "name": "getDescription",
          "request": {
            "type": "getDescription"
          },
          "response": {
            "value": {
              "_retval": "json"
            }
          }
        },
        {
          "name": "getWallpaper",
          "request": {
            "type": "getWallpaper"
          },
          "response": {
            "value": {
              "_retval": "longstring"
            }
          }
        },
        {
          "name": "screenshotToDataURL",
          "request": {
            "type": "screenshotToDataURL"
          },
          "response": {
            "value": {
              "_retval": "longstring"
            }
          }
        },
        {
          "name": "getRawPermissionsTable",
          "request": {
            "type": "getRawPermissionsTable"
          },
          "response": {
            "value": {
              "_retval": "json"
            }
          }
        }
      ],
      "events": {}
    }
  },
  "from": "root"
}

},{}],25:[function(_dereq_,module,exports){
"use strict";

var Class = _dereq_("./class").Class;
var util = _dereq_("./util");
var keys = util.keys;
var values = util.values;
var pairs = util.pairs;
var query = util.query;
var findPath = util.findPath;
var EventTarget = _dereq_("./event").EventTarget;

var TypeSystem = Class({
  constructor: function(client) {
    var types = Object.create(null);
    var specification = Object.create(null);

    this.specification = specification;
    this.types = types;

    var typeFor = function typeFor(typeName) {
      typeName = typeName || "primitive";
      if (!types[typeName]) {
        defineType(typeName);
      }

      return types[typeName];
    };
    this.typeFor = typeFor;

    var defineType = function(descriptor) {
      var type = void(0);
      if (typeof(descriptor) === "string") {
        if (descriptor.indexOf(":") > 0)
          type = makeCompoundType(descriptor);
        else if (descriptor.indexOf("#") > 0)
          type = new ActorDetail(descriptor);
          else if (specification[descriptor])
            type = makeCategoryType(specification[descriptor]);
      } else {
        type = makeCategoryType(descriptor);
      }

      if (type)
        types[type.name] = type;
      else
        throw TypeError("Invalid type: " + descriptor);
    };
    this.defineType = defineType;


    var makeCompoundType = function(name) {
      var index = name.indexOf(":");
      var baseType = name.slice(0, index);
      var subType = name.slice(index + 1);

      return baseType === "array" ? new ArrayOf(subType) :
      baseType === "nullable" ? new Maybe(subType) :
      null;
    };

    var makeCategoryType = function(descriptor) {
      var category = descriptor.category;
      return category === "dict" ? new Dictionary(descriptor) :
      category === "actor" ? new Actor(descriptor) :
      null;
    };

    var read = function(input, context, typeName) {
      return typeFor(typeName).read(input, context);
    }
    this.read = read;

    var write = function(input, context, typeName) {
      return typeFor(typeName).write(input);
    };
    this.write = write;


    var Type = Class({
      constructor: function() {
      },
      get name() {
        return this.category ? this.category + ":" + this.type :
        this.type;
      },
      read: function(input, context) {
        throw new TypeError("`Type` subclass must implement `read`");
      },
      write: function(input, context) {
        throw new TypeError("`Type` subclass must implement `write`");
      }
    });

    var Primitve = Class({
      extends: Type,
      constuctor: function(type) {
        this.type = type;
      },
      read: function(input, context) {
        return input;
      },
      write: function(input, context) {
        return input;
      }
    });

    var Maybe = Class({
      extends: Type,
      category: "nullable",
      constructor: function(type) {
        this.type = type;
      },
      read: function(input, context) {
        return input === null ? null :
        input === void(0) ? void(0) :
        read(input, context, this.type);
      },
      write: function(input, context) {
        return input === null ? null :
        input === void(0) ? void(0) :
        write(input, context, this.type);
      }
    });

    var ArrayOf = Class({
      extends: Type,
      category: "array",
      constructor: function(type) {
        this.type = type;
      },
      read: function(input, context) {
        var type = this.type;
        return input.map(function($) { return read($, context, type) });
      },
      write: function(input, context) {
        var type = this.type;
        return input.map(function($) { return write($, context, type) });
      }
    });

    var makeField = function makeField(name, type) {
      return {
        enumerable: true,
        configurable: true,
        get: function() {
          Object.defineProperty(this, name, {
            configurable: false,
            value: read(this.state[name], this.context, type)
          });
          return this[name];
        }
      }
    };

    var makeFields = function(descriptor) {
      return pairs(descriptor).reduce(function(fields, pair) {
        var name = pair[0], type = pair[1];
        fields[name] = makeField(name, type);
        return fields;
      }, {});
    }

    var DictionaryType = Class({});

    var Dictionary = Class({
      extends: Type,
      category: "dict",
      get name() { return this.type; },
      constructor: function(descriptor) {
        this.type = descriptor.typeName;
        this.types = descriptor.specializations;

        var proto = Object.defineProperties({
          extends: DictionaryType,
          constructor: function(state, context) {
            Object.defineProperties(this, {
              state: {
                enumerable: false,
                writable: true,
                configurable: true,
                value: state
              },
              context: {
                enumerable: false,
                writable: false,
                configurable: true,
                value: context
              }
            });
          }
        }, makeFields(this.types));

        this.class = new Class(proto);
      },
      read: function(input, context) {
        return new this.class(input, context);
      },
      write: function(input, context) {
        var output = {};
        for (var key in input) {
          output[key] = write(value, context, types[key]);
        }
        return output;
      }
    });

    var makeMethods = function(descriptors) {
      return descriptors.reduce(function(methods, descriptor) {
        methods[descriptor.name] = {
          enumerable: true,
          configurable: true,
          writable: false,
          value: makeMethod(descriptor)
        };
        return methods;
      }, {});
    };

    var makeEvents = function(descriptors) {
      return pairs(descriptors).reduce(function(events, pair) {
        var name = pair[0], descriptor = pair[1];
        var event = new Event(name, descriptor);
        events[event.eventType] = event;
        return events;
      }, Object.create(null));
    };

    var Actor = Class({
      extends: Type,
      category: "actor",
      get name() { return this.type; },
      constructor: function(descriptor) {
        this.type = descriptor.typeName;

        var events = makeEvents(descriptor.events || {});
        var fields = makeFields(descriptor.fields || {});
        var methods = makeMethods(descriptor.methods || []);


        var proto = {
          extends: Front,
          constructor: function() {
            Front.apply(this, arguments);
          },
          events: events
        };
        Object.defineProperties(proto, fields);
        Object.defineProperties(proto, methods);

        this.class = Class(proto);
      },
      read: function(input, context, detail) {
        var state = typeof(input) === "string" ? { actor: input } : input;

        var actor = client.get(state.actor) || new this.class(state, context);
        actor.form(state, detail, context);

        return actor;
      },
      write: function(input, context, detail) {
        return input.id;
      }
    });
    exports.Actor = Actor;


    var ActorDetail = Class({
      extends: Actor,
      constructor: function(name) {
        var parts = name.split("#")
        this.actorType = parts[0]
        this.detail = parts[1];
      },
      read: function(input, context) {
        return typeFor(this.actorType).read(input, context, this.detail);
      },
      write: function(input, context) {
        return typeFor(this.actorType).write(input, context, this.detail);
      }
    });
    exports.ActorDetail = ActorDetail;

    var Method = Class({
      extends: Type,
      constructor: function(descriptor) {
        this.type = descriptor.name;
        this.path = findPath(descriptor.response, "_retval");
        this.responseType = this.path && query(descriptor.response, this.path)._retval;
        this.requestType = descriptor.request.type;

        var params = [];
        for (var key in descriptor.request) {
          if (key !== "type") {
            var param = descriptor.request[key];
            var index = "_arg" in param ? param._arg : param._option;
            var isParam = param._option === index;
            var isArgument = param._arg === index;
            params[index] = {
              type: param.type,
              key: key,
              index: index,
              isParam: isParam,
              isArgument: isArgument
            };
          }
        }
        this.params = params;
      },
      read: function(input, context) {
        return read(query(input, this.path), context, this.responseType);
      },
      write: function(input, context) {
        return this.params.reduce(function(result, param) {
          result[param.key] = write(input[param.index], context, param.type);
          return result;
        }, {type: this.type});
      }
    });
    exports.Method = Method;

    var profiler = function(method, id) {
      return function() {
        var start = new Date();
        return method.apply(this, arguments).then(function(result) {
          var end = new Date();
          client.telemetry.add(id, +end - start);
          return result;
        });
      };
    };

    var destructor = function(method) {
      return function() {
        return method.apply(this, arguments).then(function(result) {
          client.release(this);
          return result;
        });
      };
    };

    function makeMethod(descriptor) {
      var type = new Method(descriptor);
      var method = descriptor.oneway ? makeUnidirecationalMethod(descriptor, type) :
                   makeBidirectionalMethod(descriptor, type);

      if (descriptor.telemetry)
        method = profiler(method);
      if (descriptor.release)
        method = destructor(method);

      return method;
    }

    var makeUnidirecationalMethod = function(descriptor, type) {
      return function() {
        var packet = type.write(arguments, this);
        packet.to = this.id;
        client.send(packet);
        return Promise.resolve(void(0));
      };
    };

    var makeBidirectionalMethod = function(descriptor, type) {
      return function() {
        var context = this.context;
        var packet = type.write(arguments, context);
        var context = this.context;
        packet.to = this.id;
        return client.request(packet).then(function(packet) {
          return type.read(packet, context);
        });
      };
    };

    var Event = Class({
      constructor: function(name, descriptor) {
        this.name = descriptor.type || name;
        this.eventType = descriptor.type || name;
        this.types = Object.create(null);

        var types = this.types;
        for (var key in descriptor) {
          if (key === "type") {
            types[key] = "string";
          } else {
            types[key] = descriptor[key].type;
          }
        }
      },
      read: function(input, context) {
        var output = {};
        var types = this.types;
        for (var key in input) {
          output[key] = read(input[key], context, types[key]);
        }
        return output;
      },
      write: function(input, context) {
        var output = {};
        var types = this.types;
        for (var key in this.types) {
          output[key] = write(input[key], context, types[key]);
        }
        return output;
      }
    });

    var Front = Class({
      extends: EventTarget,
      EventTarget: EventTarget,
      constructor: function(state) {
        this.EventTarget();
        Object.defineProperties(this,  {
          state: {
            enumerable: false,
            writable: true,
            configurable: true,
            value: state
          }
        });

        client.register(this);
      },
      get id() {
        return this.state.actor;
      },
      get context() {
        return this;
      },
      form: function(state, detail, context) {
        if (this.state !== state) {
          if (detail) {
            this.state[detail] = state[detail];
          } else {
            pairs(state).forEach(function(pair) {
              var key = pair[0], value = pair[1];
              this.state[key] = value;
            }, this);
          }
        }

        if (context) {
          client.supervise(context, this);
        }
      },
      requestTypes: function() {
        return client.request({
          to: this.id,
          type: "requestTypes"
        }).then(function(packet) {
          return packet.requestTypes;
        });
      }
    });
    types.primitive = new Primitve("primitive");
    types.string = new Primitve("string");
    types.number = new Primitve("number");
    types.boolean = new Primitve("boolean");
    types.json = new Primitve("json");
    types.array = new Primitve("array");
  },
  registerTypes: function(descriptor) {
    var specification = this.specification;
    values(descriptor.types).forEach(function(descriptor) {
      specification[descriptor.typeName] = descriptor;
    });
  }
});
exports.TypeSystem = TypeSystem;

},{"./class":3,"./event":5,"./util":26}],26:[function(_dereq_,module,exports){
"use strict";

var keys = Object.keys;
exports.keys = keys;


var values = function(object) {
  return keys(object).map(function(key) {
    return object[key]
  });
};
exports.values = values;


var pairs = function(object) {
  return keys(object).map(function(key) {
    return [key, object[key]]
  });
};
exports.pairs = pairs;



var query = function(object, path) {
  return path.reduce(function(object, entry) {
    return object && object[entry]
  }, object);
};
exports.query = query;

var isObject = function(x) {
  return x && typeof(x) === "object"
}

var findPath = function(object, key) {
  var path = void(0);
  if (object && typeof(object) === "object") {
    var names = keys(object);
    if (names.indexOf(key) >= 0) {
      path = [];
    } else {
      var index = 0;
      var count = names.length;
      while (index < count && !path){
        var head = names[index];
        var tail = findPath(object[head], key);
        path = tail ? [head].concat(tail) : tail;
        index = index + 1
      }
    }
  }
  return path;
};
exports.findPath = findPath;

},{}]},{},[1])

(1)
});
