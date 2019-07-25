



































const EXPORTED_SYMBOLS = ['Async'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");







function NamableTracker() {
  this.__length = 0;
  this.__dict = {};
}

NamableTracker.prototype = {
  get length() { return this.__length; },
  add: function GD_add(item) {
    this.__dict[item.name] = item;
    this.__length++;
  },
  remove: function GD_remove(item) {
    delete this.__dict[item.name];
    this.__length--;
  },
  __iterator__: function GD_iterator() {
    for (name in this.__dict)
      yield name;
  }
}

let gCurrentId = 0;
let gCurrentCbId = 0;
let gOutstandingGenerators = new NamableTracker();







function AsyncException(asyncStack, exceptionToWrap) {
  this._exceptionToWrap = exceptionToWrap;
  this._asyncStack = asyncStack;

  
  
  
  this.__proto__ = {
    get asyncStack() {
      return this._asyncStack;
    },

    addAsyncFrame: function AsyncException_addAsyncFrame(frame) {
      this._asyncStack += ((this._asyncStack? "\n" : "") +
                           formatAsyncFrame(frame));
    }
  };
  this.__proto__.__proto__ = this._exceptionToWrap;
}

function Generator(thisArg, method, onComplete, args) {
  this._outstandingCbs = 0;
  this._log = Log4Moz.Service.getLogger("Async.Generator");
  this._log.level =
    Log4Moz.Level[Utils.prefs.getCharPref("log.logger.async")];
  this._thisArg = thisArg;
  this._method = method;
  this._id = gCurrentId++;
  this.onComplete = onComplete;
  this._args = args;

  gOutstandingGenerators.add(this);

  this._initFrame = Components.stack.caller;
  
  
  while (this._initFrame.name.match(/^Async(Gen|)_/))
    this._initFrame = this._initFrame.caller;
}
Generator.prototype = {
  get name() { return this._method.name + "-" + this._id; },
  get generator() { return this._generator; },

  get cb() {
    let caller = Components.stack.caller;
    let cbId = gCurrentCbId++;
    this._outstandingCbs++;
    this._log.trace(this.name + ": cb-" + cbId + " generated at:\n" +
                    formatAsyncFrame(caller));
    let self = this;
    let cb = function(data) {
      self._log.trace(self.name + ": cb-" + cbId + " called.");
      self._cont(data);
    };
    cb.parentGenerator = this;
    return cb;
  },
  get listener() { return new Utils.EventListener(this.cb); },

  get _thisArg() { return this.__thisArg; },
  set _thisArg(value) {
    if (typeof value != "object")
      throw "Generator: expected type 'object', got type '" + typeof(value) + "'";
    this.__thisArg = value;
  },

  get _method() { return this.__method; },
  set _method(value) {
    if (typeof value != "function")
      throw "Generator: expected type 'function', got type '" + typeof(value) + "'";
    this.__method = value;
  },

  get onComplete() {
    if (this._onComplete)
      return this._onComplete;
    return function() {
      
    };
  },
  set onComplete(value) {
    if (value && typeof value != "function")
      throw "Generator: expected type 'function', got type '" + typeof(value) + "'";
    this._onComplete = value;
  },

  get asyncStack() {
    return ("unknown (async) :: " + this.name + "\n" +
            traceAsyncFrame(this._initFrame));
  },

  _handleException: function AsyncGen__handleException(e) {
    if (e instanceof StopIteration) {
      this._log.trace(this.name + ": End of coroutine reached.");
      

    } else if (this.onComplete.parentGenerator instanceof Generator) {
      this._log.trace("[" + this.name + "] Saving exception and stack trace");
      this._log.trace("Exception: " + Utils.exceptionStr(e));

        if (e instanceof AsyncException) {
          
          
          if (e.asyncStack.indexOf(formatAsyncFrame(this._initFrame)) == -1)
            e.addAsyncFrame(this._initFrame);
        } else {
          e = new AsyncException(this.asyncStack, e);
        }

      this._exception = e;

    } else {
      this._log.error("Exception: " + Utils.exceptionStr(e));
      this._log.debug("Stack trace:\n" + Utils.stackTrace(e));
    }

    
    
    
    
    if (!this._timer) {
      this._log.trace("[" + this.name + "] running done() from _handleException()");
      this.done();
    }
  },

  _detectDeadlock: function AsyncGen__detectDeadlock() {
    if (this._outstandingCbs == 0)
      this._log.warn("Async method '" + this.name +
                     "' may have yielded without an outstanding callback.");
  },

  run: function AsyncGen_run() {
    this._continued = false;
    try {
      this._generator = this._method.apply(this._thisArg, this._args);
      this.generator.next(); 
      this.generator.send(this);
      this._detectDeadlock();
    } catch (e) {
      if (!(e instanceof StopIteration) || !this._timer)
        this._handleException(e);
    }
  },

  _cont: function AsyncGen__cont(data) {
    this._outstandingCbs--;
    this._log.trace(this.name + ": resuming coroutine.");
    this._continued = true;
    try {
      this.generator.send(data);
      this._detectDeadlock();
    } catch (e) {
      if (!(e instanceof StopIteration) || !this._timer)
        this._handleException(e);
    }
  },

  _throw: function AsyncGen__throw(exception) {
    this._outstandingCbs--;
    try { this.generator.throw(exception); }
    catch (e) {
      if (!(e instanceof StopIteration) || !this._timer)
        this._handleException(e);
    }
  },

  
  
  
  
  
  
  
  done: function AsyncGen_done(retval) {
    if (this._timer) 
      return;
    let self = this;
    let cb = function() { self._done(retval); };
    this._log.trace(this.name + ": done() called.");
    if (!this._exception && this._outstandingCbs > 0)
      this._log.warn("Async method '" + this.name +
                     "' may have outstanding callbacks.");
    this._timer = Utils.makeTimerForCall(cb);
  },

  _done: function AsyncGen__done(retval) {
    if (!this._generator) {
      this._log.error("Async method '" + this.name + "' is missing a 'yield' call " +
                      "(or called done() after being finalized)");
      this._log.trace("Initial async stack trace:\n" + this.asyncStack);
    } else {
      this._generator.close();
    }
    this._generator = null;
    this._timer = null;

    if (this._exception) {
      this._log.trace("[" + this.name + "] Propagating exception to parent generator");
      this.onComplete.parentGenerator._throw(this._exception);
    } else {
      try {
        this._log.trace("[" + this.name + "] Running onComplete()");
        this.onComplete(retval);
      } catch (e) {
        this._log.error("Exception caught from onComplete handler of " +
                        this.name + " generator");
        this._log.error("Exception: " + Utils.exceptionStr(e));
        this._log.trace("Current stack trace:\n" + Utils.stackTrace(e));
        this._log.trace("Initial async stack trace:\n" + this.asyncStack);
      }
    }
    gOutstandingGenerators.remove(this);
  }
};

function formatAsyncFrame(frame) {
  
  
  let tmp = "<file:unknown>";
  if (frame.filename)
    tmp = frame.filename.replace(/^file:\/\/.*\/([^\/]+.js)$/, "module:$1");
  tmp += ":" + frame.lineNumber + " :: " + frame.name;
  return tmp;
}

function traceAsyncFrame(frame, str) {
  if (!str)
    str = "";

  
  
  while (frame.name && frame.name.match(/^Async(Gen|)_/))
    frame = frame.caller;

  if (frame.caller)
    str = traceAsyncFrame(frame.caller, str);
  str = formatAsyncFrame(frame) + (str? "\n" : "") + str;

  return str;
}


Async = {
  get outstandingGenerators() { return gOutstandingGenerators; },

  
  
  
  
  
  

  run: function Async_run(thisArg, method, onComplete ) {
    let args = Array.prototype.slice.call(arguments, 3);
    let gen = new Generator(thisArg, method, onComplete, args);
    gen.run();
    return gen;
  },

  
  
  
  
  
  
  
  
  
  
  
  

  sugar: function Async_sugar(thisArg, onComplete  ) {
    let args = Array.prototype.slice.call(arguments, 1);
    args.unshift(thisArg, this);
    Async.run.apply(Async, args);
  },
};
