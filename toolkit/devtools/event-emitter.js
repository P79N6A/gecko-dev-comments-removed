







this.EventEmitter = function EventEmitter() {};

if (typeof(require) === "function") {
   module.exports = EventEmitter;
   var {Cu, components} = require("chrome");
} else {
  var EXPORTED_SYMBOLS = ["EventEmitter"];
  var Cu = this["Components"].utils;
  var components = Components;
}

const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
const { Services } = Cu.import("resource://gre/modules/Services.jsm");








EventEmitter.decorate = function EventEmitter_decorate (aObjectToDecorate) {
  let emitter = new EventEmitter();
  aObjectToDecorate.on = emitter.on.bind(emitter);
  aObjectToDecorate.off = emitter.off.bind(emitter);
  aObjectToDecorate.once = emitter.once.bind(emitter);
  aObjectToDecorate.emit = emitter.emit.bind(emitter);
};

EventEmitter.prototype = {
  







  on: function EventEmitter_on(aEvent, aListener) {
    if (!this._eventEmitterListeners)
      this._eventEmitterListeners = new Map();
    if (!this._eventEmitterListeners.has(aEvent)) {
      this._eventEmitterListeners.set(aEvent, []);
    }
    this._eventEmitterListeners.get(aEvent).push(aListener);
  },

  













  once: function EventEmitter_once(aEvent, aListener) {
    let deferred = promise.defer();

    let handler = function(aEvent, aFirstArg) {
      this.off(aEvent, handler);
      if (aListener) {
        aListener.apply(null, arguments);
      }
      deferred.resolve(aFirstArg);
    }.bind(this);

    handler._originalListener = aListener;
    this.on(aEvent, handler);

    return deferred.promise;
  },

  








  off: function EventEmitter_off(aEvent, aListener) {
    if (!this._eventEmitterListeners)
      return;
    let listeners = this._eventEmitterListeners.get(aEvent);
    if (listeners) {
      this._eventEmitterListeners.set(aEvent, listeners.filter(l => {
        return l !== aListener && l._originalListener !== aListener;
      }));
    }
  },

  



  emit: function EventEmitter_emit(aEvent) {
    this.logEvent(aEvent, arguments);

    if (!this._eventEmitterListeners || !this._eventEmitterListeners.has(aEvent)) {
      return;
    }

    let originalListeners = this._eventEmitterListeners.get(aEvent);
    for (let listener of this._eventEmitterListeners.get(aEvent)) {
      
      
      if (!this._eventEmitterListeners) {
        break;
      }

      
      
      if (originalListeners === this._eventEmitterListeners.get(aEvent) ||
          this._eventEmitterListeners.get(aEvent).some(function(l) l === listener)) {
        try {
          listener.apply(null, arguments);
        }
        catch (ex) {
          
          let msg = ex + ": " + ex.stack;
          Cu.reportError(msg);
          dump(msg + "\n");
        }
      }
    }
  },

  logEvent: function(aEvent, args) {
    let logging = Services.prefs.getBoolPref("devtools.dump.emit");

    if (logging) {
      let caller = components.stack.caller.caller;
      let func = caller.name;
      let path = caller.filename.split(/ -> /)[1] + ":" + caller.lineNumber;

      let argOut = "(";
      if (args.length === 1) {
        argOut += aEvent;
      }

      let out = "EMITTING: ";

      
      try {
        for (let i = 1; i < args.length; i++) {
          if (i === 1) {
            argOut = "(" + aEvent + ", ";
          } else {
            argOut += ", ";
          }

          let arg = args[i];
          argOut += arg;

          if (arg && arg.nodeName) {
            argOut += " (" + arg.nodeName;
            if (arg.id) {
              argOut += "#" + arg.id;
            }
            if (arg.className) {
              argOut += "." + arg.className;
            }
            argOut += ")";
          }
        }
      } catch(e) {
        
        
      }

      argOut += ")";
      out += "emit" + argOut + " from " + func + "() -> " + path + "\n";

      dump(out);
    }
  },
};
