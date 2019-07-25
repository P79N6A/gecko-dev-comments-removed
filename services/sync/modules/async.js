



































const EXPORTED_SYMBOLS = ['Async'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/constants.js");





function AsyncException(generator, message) {
  this.generator = generator;
  this.message = message;
  this._trace = this.generator.trace;
}
AsyncException.prototype = {
  get generator() { return this._generator; },
  set generator(value) {
    if (!(value instanceof Generator))
      throw "expected type 'Generator'";
    this._generator = value;
  },

  get message() { return this._message; },
  set message(value) { this._message = value; },

  get trace() { return this._trace; },
  set trace(value) { this._trace = value; },
  
  toString: function AsyncException_toString() {
    return this.message;
  }
};

function Generator(object, method, onComplete, args) {
  this._log = Log4Moz.Service.getLogger("Async.Generator");
  this._log.level =
    Log4Moz.Level[Utils.prefs.getCharPref("log.logger.async")];
  this._object = object;
  this._method = method;
  this.onComplete = onComplete;
  this._args = args;

  let frame = Components.stack.caller;
  if (frame.name == "Async_run")
    frame = frame.caller;
  if (frame.name == "Async_sugar")
    frame = frame.caller;

  this._initFrame = frame;
}
Generator.prototype = {
  get name() { return this._method.name; },
  get generator() { return this._generator; },

  
  
  
  get errorOnStop() { return this._errorOnStop; },
  set errorOnStop(value) { this._errorOnStop = value; },

  get cb() {
    let cb = Utils.bind2(this, function(data) { this.cont(data); });
    cb.parentGenerator = this;
    return cb;
  },
  get listener() { return new Utils.EventListener(this.cb); },

  get _object() { return this.__object; },
  set _object(value) {
    if (typeof value != "object")
      throw "expected type 'object', got type '" + typeof(value) + "'";
    this.__object = value;
  },

  get _method() { return this.__method; },
  set _method(value) {
    if (typeof value != "function")
      throw "expected type 'function', got type '" + typeof(value) + "'";
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
      throw "expected type 'function', got type '" + typeof(value) + "'";
    this._onComplete = value;
  },

  get trace() {
    return Utils.stackTrace(this._initFrame) +
      "JS frame :: unknown (async) :: " + this.name;
  },

  _handleException: function AsyncGen__handleException(e) {
    if (e instanceof StopIteration) {
      if (this.errorOnStop) {
        this._log.error("Generator stopped unexpectedly");
        this._log.trace("Stack trace:\n" + this.trace);
        this._exception = "Generator stopped unexpectedly"; 
      }

    } else if (this.onComplete.parentGenerator instanceof Generator) {
      

      if (e instanceof AsyncException)
        e.trace = this.trace + e.trace? "\n" + e.trace : "";
      else
        e = new AsyncException(this, e);

      this._exception = e;

    } else {
      this._log.error(Async.exceptionStr(this, e));
      this._log.debug("Stack trace:\n" + this.trace +
                      (e.trace? "\n" + e.trace : ""));
    }

    
    
    
    
    this.done();
  },

  run: function AsyncGen_run() {
    try {
      this._generator = this._method.apply(this._object, this._args);
      this.generator.next(); 
      this.generator.send(this);
    } catch (e) {
      this._handleException(e);
    }
  },

  cont: function AsyncGen_cont(data) {
    try { this.generator.send(data); }
    catch (e) { this._handleException(e); }
  },

  throw: function AsyncGen_throw(exception) {
    try { this.generator.throw(exception); }
    catch (e) { this._handleException(e); }
  },

  
  
  
  
  
  
  
  done: function AsyncGen_done(retval) {
    if (this._timer) 
      return;
    let self = this;
    let cb = function() { self._done(retval); };
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timer.initWithCallback(new Utils.EventListener(cb),
                                 0, this._timer.TYPE_ONE_SHOT);
  },

  _done: function AsyncGen__done(retval) {
    if (!this._generator) {
      this._log.error("Generator '" + this.name + "' called 'done' after it's finalized");
      this._log.trace("Initial stack trace:\n" + this.trace);
      return;
    }
    this._generator.close();
    this._generator = null;
    this._timer = null;

    if (this._exception) {
      this._log.trace("Propagating exception to parent generator");
      this.onComplete.parentGenerator.throw(this._exception);
    } else {
      try {
        this.onComplete(retval);
      } catch (e) {
        this._log.error("Exception caught from onComplete handler of " +
                        this.name + " generator");
        this._log.error("Exception: " + Utils.exceptionStr(e));
        this._log.trace("Current stack trace:\n" + Utils.stackTrace(Components.stack));
        this._log.trace("Initial stack trace:\n" + this.trace);
      }
    }
  }
};

Async = {

  
  
  
  
  
  

  run: function Async_run(object, method, onComplete, args) {
    let args = Array.prototype.slice.call(arguments, 3);
    let gen = new Generator(object, method, onComplete, args);
    gen.run();
    return gen;
  },

  
  
  
  
  
  
  
  
  
  
  
  

  sugar: function Async_sugar(object, onComplete, extra_args) {
    let args = Array.prototype.slice.call(arguments, 1);
    args.unshift(object, this);
    Async.run.apply(Async, args);
  },

  exceptionStr: function Async_exceptionStr(gen, ex) {
    return "Exception caught in " + gen.name + ": " + Utils.exceptionStr(ex);
  }
};
