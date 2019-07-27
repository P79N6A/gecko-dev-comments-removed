


"use strict"



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = ["DelayedInit"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "MessageLoop",
                                   "@mozilla.org/message-loop;1",
                                   "nsIMessageLoop");


































let DelayedInit = {
  schedule: function (fn, object, name, maxWait) {
    return Impl.scheduleInit(fn, object, name, maxWait);
  },
};



const MAX_IDLE_RUN_MS = 50;

let Impl = {
  pendingInits: [],

  onIdle: function () {
    let startTime = Cu.now();
    let time = startTime;
    let nextDue;

    
    
    for (let init of this.pendingInits) {
      if (init.complete) {
        continue;
      }

      if (time - startTime < MAX_IDLE_RUN_MS) {
        init.maybeInit();
        time = Cu.now();
      } else {
        
        nextDue = nextDue ? Math.min(nextDue, init.due) : init.due;
      }
    }

    
    this.pendingInits = this.pendingInits.filter((init) => !init.complete);

    if (nextDue !== undefined) {
      
      MessageLoop.postIdleTask(() => this.onIdle(),
                               Math.max(0, nextDue - time));
    }
  },

  addPendingInit: function (fn, wait) {
    let init = {
      fn: fn,
      due: Cu.now() + wait,
      complete: false,
      maybeInit: function () {
        if (this.complete) {
          return false;
        }
        this.complete = true;
        this.fn.call();
        this.fn = null;
        return true;
      },
    };

    if (!this.pendingInits.length) {
      
      MessageLoop.postIdleTask(() => this.onIdle(), wait);
    }
    this.pendingInits.push(init);
    return init;
  },

  scheduleInit: function (fn, object, name, wait) {
    let init = this.addPendingInit(fn, wait);

    if (!object || !name) {
      
      return;
    }

    
    let prop = Object.getOwnPropertyDescriptor(object, name) ||
               { configurable: true, enumerable: true, writable: true };

    if (!prop.configurable) {
      
      init.maybeInit();
      return;
    }

    
    
    Object.defineProperty(object, name, {
      get: function proxy_getter() {
        init.maybeInit();

        
        
        let newProp = Object.getOwnPropertyDescriptor(object, name);
        if (newProp.get !== proxy_getter) {
          
          prop = newProp;
        } else {
          
          Object.defineProperty(object, name, prop);
        }

        if (prop.get) {
          return prop.get.call(object);
        }
        return prop.value;
      },
      set: function (newVal) {
        init.maybeInit();

        
        
        if (prop.get || prop.set) {
          Object.defineProperty(object, name, prop);
          return prop.set.call(object);
        }

        prop.value = newVal;
        Object.defineProperty(object, name, prop);
        return newVal;
      },
      configurable: true,
      enumerable: true,
    });
  }
};
