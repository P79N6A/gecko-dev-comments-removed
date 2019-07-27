




"use strict";

this.EXPORTED_SYMBOLS = ["PerformanceStats"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;













Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);



XPCOMUtils.defineLazyServiceGetter(this, "performanceStatsService",
  "@mozilla.org/toolkit/performance-stats-service;1",
  Ci.nsIPerformanceStatsService);



XPCOMUtils.defineLazyServiceGetter(this, "finalizer",
  "@mozilla.org/toolkit/finalizationwitness;1",
  Ci.nsIFinalizationWitnessService
);




const FINALIZATION_TOPIC = "performancemonitor-finalize";

const PROPERTIES_META_IMMUTABLE = ["name", "addonId", "isSystem"];
const PROPERTIES_META = [...PROPERTIES_META_IMMUTABLE, "windowId", "title"];









function Probe(name, impl) {
  this._name = name;
  this._counter = 0;
  this._impl = impl;
}
Probe.prototype = {
  





  acquire: function() {
    if (this._counter == 0) {
      this._impl.isActive = true;
    }
    this._counter++;
  },

  




  release: function() {
    this._counter--;
    if (this._counter == 0) {
      this._impl.isActive = false;
    }
  },

  








  extract: function(xpcom) {
    if (!this._impl.isActive) {
      throw new Error(`Probe is inactive: ${this._name}`);
    }
    return this._impl.extract(xpcom);
  },

  





  isEqual: function(a, b) {
    if (a == null && b == null) {
      return true;
    }
    if (a != null && b != null) {
      return this._impl.isEqual(a, b);
    }
    return false;
  },

  








  substract: function(a, b) {
    if (a == null) {
      throw new TypeError();
    }
    if (b == null) {
      return a;
    }
    return this._impl.substract(a, b);
  },

  


  get name() {
    return this._name;
  }
};



function lastNonZero(array) {
  for (let i = array.length - 1; i >= 0; --i) {
    if (array[i] != 0) {
      return i;
    }
  }
  return -1;
}




let Probes = {
  














  jank: new Probe("jank", {
    set isActive(x) {
      performanceStatsService.isMonitoringJank = x;
    },
    get isActive() {
      return performanceStatsService.isMonitoringJank;
    },
    extract: function(xpcom) {
      let durations = xpcom.getDurations();
      return {
        totalUserTime: xpcom.totalUserTime,
        totalSystemTime: xpcom.totalSystemTime,
        durations: durations,
        longestDuration: lastNonZero(durations)
      }
    },
    isEqual: function(a, b) {
      
      if (a.totalUserTime != b.totalUserTime) {
        return false;
      }
      if (a.totalSystemTime != b.totalSystemTime) {
        return false;
      }
      for (let i = 0; i < a.durations.length; ++i) {
        if (a.durations[i] != b.durations[i]) {
          return false;
        }
      }
      return true;
    },
    substract: function(a, b) {
      
      let result = {
        totalUserTime: a.totalUserTime - b.totalUserTime,
        totalSystemTime: a.totalSystemTime - b.totalSystemTime,
        durations: [],
        longestDuration: -1,
      };
      for (let i = 0; i < a.durations.length; ++i) {
        result.durations[i] = a.durations[i] - b.durations[i];
      }
      result.longestDuration = lastNonZero(result.durations);
      return result;
    }
  }),

  







  cpow: new Probe("cpow", {
    set isActive(x) {
      performanceStatsService.isMonitoringCPOW = x;
    },
    get isActive() {
      return performanceStatsService.isMonitoringCPOW;
    },
    extract: function(xpcom) {
      return {
        totalCPOWTime: xpcom.totalCPOWTime
      };
    },
    isEqual: function(a, b) {
      return a.totalCPOWTime == b.totalCPOWTime;
    },
    substract: function(a, b) {
      return {
        totalCPOWTime: a.totalCPOWTime - b.totalCPOWTime
      };
    }
  }),

  










  ticks: new Probe("ticks", {
    set isActive(x) {  },
    get isActive() { return true; },
    extract: function(xpcom) {
      return {
        ticks: xpcom.ticks
      };
    },
    isEqual: function(a, b) {
      return a.ticks == b.ticks;
    },
    substract: function(a, b) {
      return {
        ticks: a.ticks - b.ticks
      };
    }
  }),
};










function PerformanceMonitor(probes) {
  this._probes = probes;
  
  
  for (let probe of probes) {
    probe.acquire();
  }

  
  
  
  this._id = PerformanceMonitor.makeId();
  this._finalizer = finalizer.make(FINALIZATION_TOPIC, this._id)
  PerformanceMonitor._monitors.set(this._id, probes);
}
PerformanceMonitor.prototype = {
  



























  promiseSnapshot: function() {
    if (!this._finalizer) {
      throw new Error("dispose() has already been called, this PerformanceMonitor is not usable anymore");
    }
    
    return Promise.resolve().then(() => new Snapshot({
      xpcom: performanceStatsService.getSnapshot(),
      probes: this._probes
    }));
  },

  





  dispose: function() {
    if (!this._finalizer) {
      return;
    }
    this._finalizer.forget();
    PerformanceMonitor.dispose(this._id);

    
    this._probes = null;
    this._id = null;
    this._finalizer = null;
  }
};




PerformanceMonitor._monitors = new Map();





PerformanceMonitor.make = function(probeNames) {
  
  if (!Array.isArray(probeNames)) {
    throw new TypeError("Expected an array, got " + probes);
  }
  let probes = [];
  for (let probeName of probeNames) {
    if (!(probeName in Probes)) {
      throw new TypeError("Probe not implemented: " + k);
    }
    probes.push(Probes[probeName]);
  }

  return new PerformanceMonitor(probes);
};









PerformanceMonitor.dispose = function(id) {
  let probes = PerformanceMonitor._monitors.get(id);
  if (!probes) {
    throw new TypeError("`dispose()` has already been called on this monitor");
  }

  PerformanceMonitor._monitors.delete(id);
  for (let probe of probes) {
    probe.release();
  }
}



PerformanceMonitor._counter = 0;
PerformanceMonitor.makeId = function() {
  return "PerformanceMonitor-" + (this._counter++);
}



Services.obs.addObserver(function(subject, topic, value) {
  PerformanceMonitor.dispose(value);
}, FINALIZATION_TOPIC, false);


this.PerformanceStats = {
  


  getMonitor: function(probes) {
    return PerformanceMonitor.make(probes);
  }
};




































function PerformanceData({xpcom, probes}) {
  for (let k of PROPERTIES_META) {
    this[k] = xpcom[k];
  }
  for (let probe of probes) {
    this[probe.name] = probe.extract(xpcom);
  }
}
PerformanceData.prototype = {
  




  equals: function(to) {
    if (!(to instanceof PerformanceData)) {
      throw new TypeError();
    }
    for (let probeName of Object.keys(Probes)) {
      let probe = Probes[probeName];
      if (!probe.isEqual(this[probeName], to[probeName])) {
        return false;
      }
    }
  },

  







  subtract: function(to = null) {
    return new PerformanceDiff(this, to);
  }
};






function PerformanceDiff(current, old = null) {
  for (let k of PROPERTIES_META) {
    this[k] = current[k];
  }

  for (let probeName of Object.keys(Probes)) {
    let other = old ? old[probeName] : null;
    if (probeName in current) {
      this[probeName] = Probes[probeName].substract(current[probeName], other);
    }
  }
}




function Snapshot({xpcom, probes}) {
  this.componentsData = [];
  let enumeration = xpcom.getComponentsData().enumerate();
  while (enumeration.hasMoreElements()) {
    let stat = enumeration.getNext().QueryInterface(Ci.nsIPerformanceStats);
    this.componentsData.push(new PerformanceData({xpcom: stat, probes}));
  }
  this.processData = new PerformanceData({xpcom: xpcom.getProcessData(), probes});
}
