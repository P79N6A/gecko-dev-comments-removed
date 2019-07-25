



































const EXPORTED_SYMBOLS = ['Async'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");





function AsyncException(initFrame, message) {
  this.message = message;
  this._trace = initFrame;
}
AsyncException.prototype = {
  get message() { return this._message; },
  set message(value) { this._message = value; },

  get trace() { return this._trace; },
  set trace(value) { this._trace = value; },

  addFrame: function AsyncException_addFrame(frame) {
    this.trace += (this.trace? "\n" : "") + formatFrame(frame);
  },

  toString: function AsyncException_toString() {
    return this.message;
  }
};

function Generator(thisArg, method, onComplete, args) {
  this._log = Log4Moz.Service.getLogger("Async.Generator");
  this._log.level =
    Log4Moz.Level[Utils.prefs.getCharPref("log.logger.async")];
  this._thisArg = thisArg;
  this._method = method;
  this.onComplete = onComplete;
  this._args = args;
  this._initFrame = Components.stack.caller;
  
  
  while (this._initFrame.name.match(/^Async(Gen|)_/))
    this._initFrame = this._initFrame.caller;
}
Generator.prototype = {
  get name() { return this._method.name; },
  get generator() { return this._generator; },

  get cb() {
    let self = this, cb = function(data) { self.cont(data); };
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

  get trace() {
    return "unknown (async) :: " + this.name + "\n" + trace(this._initFrame);
  },

  _handleException: function AsyncGen__handleException(e) {
    if (e instanceof StopIteration) {
      

    } else if (this.onComplete.parentGenerator instanceof Generator) {
      this._log.trace("[" + this.name + "] Saving exception and stack trace");
      this._log.trace("Exception: " + Utils.exceptionStr(e));

        if (e instanceof AsyncException) {
          
          
          if (e.trace.indexOf(formatFrame(this._initFrame)) == -1)
            e.addFrame(this._initFrame);
        } else {
          e = new AsyncException(this.trace, e);
        }

      this._exception = e;

    } else {
      this._log.error("Exception: " + Utils.exceptionStr(e));
      this._log.debug("Stack trace:\n" + (e.trace? e.trace : this.trace));
    }

    
    
    
    
    if (!this._timer) {
      this._log.trace("[" + this.name + "] running done() from _handleException()");
      this.done();
    }
  },

  run: function AsyncGen_run() {
    this._continued = false;
    try {
      this._generator = this._method.apply(this._thisArg, this._args);
      this.generator.next(); 
      this.generator.send(this);
    } catch (e) {
      if (!(e instanceof StopIteration) || !this._timer)
        this._handleException(e);
    }
  },

  cont: function AsyncGen_cont(data) {
    this._continued = true;
    try { this.generator.send(data); }
    catch (e) {
      if (!(e instanceof StopIteration) || !this._timer)
        this._handleException(e);
    }
  },

  throw: function AsyncGen_throw(exception) {
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
    this._timer = Utils.makeTimerForCall(cb);
  },

  _done: function AsyncGen__done(retval) {
    if (!this._generator) {
      this._log.error("Async method '" + this.name + "' is missing a 'yield' call " +
                      "(or called done() after being finalized)");
      this._log.trace("Initial stack trace:\n" + this.trace);
    } else {
      this._generator.close();
    }
    this._generator = null;
    this._timer = null;

    if (this._exception) {
      this._log.trace("[" + this.name + "] Propagating exception to parent generator");
      this.onComplete.parentGenerator.throw(this._exception);
    } else {
      try {
        this._log.trace("[" + this.name + "] Running onComplete()");
        this.onComplete(retval);
      } catch (e) {
        this._log.error("Exception caught from onComplete handler of " +
                        this.name + " generator");
        this._log.error("Exception: " + Utils.exceptionStr(e));
        this._log.trace("Current stack trace:\n" + trace(Components.stack));
        this._log.trace("Initial stack trace:\n" + this.trace);
      }
    }
  }
};

function formatFrame(frame) {
  
  
  let tmp = frame.filename.replace(/^file:\/\/.*\/([^\/]+.js)$/, "module:$1");
  tmp += ":" + frame.lineNumber + " :: " + frame.name;
  return tmp;
}

function trace(frame, str) {
  if (!str)
    str = "";

  
  
  while (frame.name && frame.name.match(/^Async(Gen|)_/))
    frame = frame.caller;

  if (frame.caller)
    str = trace(frame.caller, str);
  str = formatFrame(frame) + (str? "\n" : "") + str;

  return str;
}


Async = {

  
  
  
  
  
  

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

  exceptionStr: function Async_exceptionStr(gen, ex) {
    return "Exception caught in " + gen.name + ": " + Utils.exceptionStr(ex);
  }
};
