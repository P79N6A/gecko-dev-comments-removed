










































"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);


const DELAY_WARNING_MS = 10 * 1000;








function warn(msg, error = null) {
  dump("WARNING: " + msg + "\n");
  if (error) {
    dump("WARNING: " + error + "\n");
    if (typeof error == "object" && "stack" in error) {
      dump("WARNING: " + error.stack + "\n");
    }
  }
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
            " Condition: " + name +
            " Phase: " + topic;
          warn(msg);
        }, DELAY_WARNING_MS, Ci.nsITimer.TYPE_ONE_SHOT);

        condition = condition.then(function onSuccess() {
            timer.cancel();
          }, function onError(error) {
            timer.cancel();
            let msg = "A completion condition encountered an error" +
                " while we were spinning the event loop." +
                " Condition: " + name +
                " Phase: " + topic;
            warn(msg, error);
        });

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

    promise = promise.then(function() {
      satisfied = true;
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
