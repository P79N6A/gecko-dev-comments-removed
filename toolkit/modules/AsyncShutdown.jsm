










































"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "gDebug",
  "@mozilla.org/xpcom/debug;1", "nsIDebug");
Object.defineProperty(this, "gCrashReporter", {
  get: function() {
    delete this.gCrashReporter;
    try {
      let reporter = Cc["@mozilla.org/xre/app-info;1"].
            getService(Ci.nsICrashReporter);
      return this.gCrashReporter = reporter;
    } catch (ex) {
      return this.gCrashReporter = null;
    }
  },
  configurable: true
});


const DELAY_WARNING_MS = 10 * 1000;




const PREF_DELAY_CRASH_MS = "toolkit.asyncshutdown.crash_timeout";
let DELAY_CRASH_MS = 60 * 1000; 
try {
  DELAY_CRASH_MS = Services.prefs.getIntPref(PREF_DELAY_CRASH_MS);
} catch (ex) {
  
}
Services.prefs.addObserver(PREF_DELAY_CRASH_MS, function() {
  DELAY_CRASH_MS = Services.prefs.getIntPref(PREF_DELAY_CRASH_MS);
}, false);









function log(msg, prefix = "", error = null) {
  dump(prefix + msg + "\n");
  if (error) {
    dump(prefix + error + "\n");
    if (typeof error == "object" && "stack" in error) {
      dump(prefix + error.stack + "\n");
    }
  }
}
function warn(msg, error = null) {
  return log(msg, "WARNING: ", error);
}
function err(msg, error = null) {
  return log(msg, "ERROR: ", error);
}










function looseTimer(delay) {
  let DELAY_BEAT = 1000;
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  let beats = Math.ceil(delay / DELAY_BEAT);
  let deferred = Promise.defer();
  timer.initWithCallback(function() {
    if (beats <= 0) {
      deferred.resolve();
    }
    --beats;
  }, DELAY_BEAT, Ci.nsITimer.TYPE_REPEATING_PRECISE_CAN_SKIP);
  
  
  deferred.promise.then(() => timer.cancel(), () => timer.cancel());
  return deferred;
}

this.EXPORTED_SYMBOLS = ["AsyncShutdown"];




let gPhases = new Map();

this.AsyncShutdown = {
  


  get _getPhase() {
    let accepted = false;
    try {
      accepted = Services.prefs.getBoolPref("toolkit.asyncshutdown.testing");
    } catch (ex) {
      
    }
    if (accepted) {
      return getPhase;
    }
    return undefined;
  }
};







function getPhase(topic) {
  let phase = gPhases.get(topic);
  if (phase) {
    return phase;
  }
  let spinner = new Spinner(topic);
  phase = Object.freeze({
    




































    addBlocker: function(name, condition) {
      if (typeof name != "string") {
        throw new TypeError("Expected a human-readable name as first argument");
      }
      spinner.addBlocker({name: name, condition: condition});
    }
  });
  gPhases.set(topic, phase);
  return phase;
}







function Spinner(topic) {
  this._topic = topic;
  this._conditions = new Set(); 
  Services.obs.addObserver(this, topic, false);
}

Spinner.prototype = {
  












  addBlocker: function(condition) {
    if (!this._conditions) {
      throw new Error("Phase " + this._topic +
                      " has already begun, it is too late to register" +
                      " completion conditions.");
    }
    this._conditions.add(condition);
  },

  observe: function() {
    let topic = this._topic;
    Services.obs.removeObserver(this, topic);

    let conditions = this._conditions;
    this._conditions = null; 

    if (conditions.size == 0) {
      
      return;
    }

    
    let allPromises = [];

    
    
    let allMonitors = [];

    for (let {condition, name} of conditions) {
      

      try {
        if (typeof condition == "function") {
          
          try {
            condition = condition(topic);
          } catch (ex) {
            condition = Promise.reject(ex);
          }
        }
        
        
        
        
        
        condition = Promise.resolve(condition);

        
        
        
        

        let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        timer.initWithCallback(function() {
          let msg = "A phase completion condition is" +
            " taking too long to complete." +
            " Condition: " + monitor.name +
            " Phase: " + topic;
          warn(msg);
        }, DELAY_WARNING_MS, Ci.nsITimer.TYPE_ONE_SHOT);

        let monitor = {
          isFrozen: true,
          name: name
        };
        condition = condition.then(function onSuccess() {
            timer.cancel(); 
                            
            monitor.isFrozen = false;
          }, function onError(error) {
            timer.cancel();
            let msg = "A completion condition encountered an error" +
                " while we were spinning the event loop." +
                " Condition: " + name +
                " Phase: " + topic;
            warn(msg, error);
            monitor.isFrozen = false;
        });
        allMonitors.push(monitor);
        allPromises.push(condition);

      } catch (error) {
          let msg = "A completion condition encountered an error" +
                " while we were initializing the phase." +
                " Condition: " + name +
                " Phase: " + topic;
          warn(msg, error);
      }

    }
    conditions = null;

    let promise = Promise.all(allPromises);
    allPromises = null;

    promise = promise.then(null, function onError(error) {
      
      
      let msg = "An uncaught error appeared while completing the phase." +
            " Phase: " + topic;
      warn(msg, error);
    });

    let satisfied = false; 

    
    
    
    
    
    
    let timeToCrash = looseTimer(DELAY_CRASH_MS);
    timeToCrash.promise.then(
      function onTimeout() {
        
        let frozen = [];
        for (let {name, isFrozen} of allMonitors) {
          if (isFrozen) {
            frozen.push(name);
          }
        }

        let msg = "At least one completion condition failed to complete" +
              " within a reasonable amount of time. Causing a crash to" +
              " ensure that we do not leave the user with an unresponsive" +
              " process draining resources." +
              " Conditions: " + frozen.join(", ") +
              " Phase: " + topic;
        err(msg);
        if (gCrashReporter && gCrashReporter.enabled) {
          let data = {
            phase: topic,
            conditions: frozen
          };
          gCrashReporter.annotateCrashReport("AsyncShutdownTimeout",
            JSON.stringify(data));
        } else {
          warn("No crash reporter available");
        }

        let error = new Error();
        gDebug.abort(error.fileName, error.lineNumber + 1);
      },
      function onSatisfied() {
        
        
      });

    promise = promise.then(function() {
      satisfied = true;
      timeToCrash.reject();
    });

    
    let thread = Services.tm.mainThread;
    while(!satisfied) {
      thread.processNextEvent(true);
    }
  }
};







this.AsyncShutdown.profileBeforeChange = getPhase("profile-before-change");
this.AsyncShutdown.webWorkersShutdown = getPhase("web-workers-shutdown");
Object.freeze(this.AsyncShutdown);
