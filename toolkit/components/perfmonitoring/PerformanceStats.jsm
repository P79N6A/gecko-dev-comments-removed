




"use strict";

this.EXPORTED_SYMBOLS = ["PerformanceStats"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;













Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "PromiseUtils",
  "resource://gre/modules/PromiseUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "setTimeout",
  "resource://gre/modules/Timer.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "clearTimeout",
  "resource://gre/modules/Timer.jsm");



XPCOMUtils.defineLazyServiceGetter(this, "performanceStatsService",
  "@mozilla.org/toolkit/performance-stats-service;1",
  Ci.nsIPerformanceStatsService);



XPCOMUtils.defineLazyServiceGetter(this, "finalizer",
  "@mozilla.org/toolkit/finalizationwitness;1",
  Ci.nsIFinalizationWitnessService
);



const FINALIZATION_TOPIC = "performancemonitor-finalize";

const PROPERTIES_META_IMMUTABLE = ["addonId", "isSystem", "isChildProcess", "groupId"];
const PROPERTIES_META = [...PROPERTIES_META_IMMUTABLE, "windowId", "title", "name"];


const MAX_WAIT_FOR_CHILD_PROCESS_MS = 5000;

let isContent = Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT;








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

  








  subtract: function(a, b) {
    if (a == null) {
      throw new TypeError();
    }
    if (b == null) {
      return a;
    }
    return this._impl.subtract(a, b);
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
    subtract: function(a, b) {
      
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
    subtract: function(a, b) {
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
    subtract: function(a, b) {
      return {
        ticks: a.ticks - b.ticks
      };
    }
  }),

  "jank-content": new Probe("jank-content", {
    _isActive: false,
    set isActive(x) {
      this._isActive = x;
      if (x) {
        Process.broadcast("acquire", ["jank"]);
      } else {
        Process.broadcast("release", ["jank"]);
      }
    },
    get isActive() {
      return this._isActive;
    },
    extract: function(xpcom) {
      return {};
    },
    isEqual: function(a, b) {
      return true;
    },
    subtract: function(a, b) {
      return null;
    }
  }),

  "cpow-content": new Probe("cpow-content", {
    _isActive: false,
    set isActive(x) {
      this._isActive = x;
      if (x) {
        Process.broadcast("acquire", ["cpow"]);
      } else {
        Process.broadcast("release", ["cpow"]);
      }
    },
    get isActive() {
      return this._isActive;
    },
    extract: function(xpcom) {
      return {};
    },
    isEqual: function(a, b) {
      return true;
    },
    subtract: function(a, b) {
      return null;
    }
  }),

  "ticks-content": new Probe("ticks-content", {
    set isActive(x) {
      
    },
    get isActive() {
      return true;
    },
    extract: function(xpcom) {
      return {};
    },
    isEqual: function(a, b) {
      return true;
    },
    subtract: function(a, b) {
      return null;
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
  


  get probeNames() {
    return [for (probe of this._probes) probe.name];
  },

  
































  promiseSnapshot: function(options = null) {
    if (!this._finalizer) {
      throw new Error("dispose() has already been called, this PerformanceMonitor is not usable anymore");
    }
    let probes;
    if (options && options.probeNames || undefined) {
      if (!Array.isArray(options.probeNames)) {
        throw new TypeError();
      }
      
      for (let probeName of options.probeNames) {
        let probe = this._probes.find(probe => probe.name == probeName);
        if (!probe) {
          throw new TypeError(`I need probe ${probeName} but I only have ${this.probeNames}`);
        }
        if (!probes) {
          probes = [];
        }
        probes.push(probe);
      }
    } else {
      probes = this._probes;
    }
    return Task.spawn(function*() {
      let collected = yield Process.broadcastAndCollect("collect", {probeNames: [for (probe of probes) probe.name]});
      return new Snapshot({
        xpcom: performanceStatsService.getSnapshot(),
        childProcesses: collected,
        probes
      });
    });
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
      throw new TypeError("Probe not implemented: " + probeName);
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




































function PerformanceData({xpcom, json, probes}) {
  if (xpcom && json) {
    throw new TypeError("Cannot import both xpcom and json data");
  }
  let source = xpcom || json;
  for (let k of PROPERTIES_META) {
    this[k] = source[k];
  }
  if (xpcom) {
    for (let probe of probes) {
      this[probe.name] = probe.extract(xpcom);
    }
    this.isChildProcess = false;
  } else {
    for (let probe of probes) {
      this[probe.name] = json[probe.name];
    }
    this.isChildProcess = true;
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
      this[probeName] = Probes[probeName].subtract(current[probeName], other);
    }
  }
}








function Snapshot({xpcom, childProcesses, probes}) {
  this.componentsData = [];

  
  if (xpcom) {
    let enumeration = xpcom.getComponentsData().enumerate();
    while (enumeration.hasMoreElements()) {
      let xpcom = enumeration.getNext().QueryInterface(Ci.nsIPerformanceStats);
      this.componentsData.push(new PerformanceData({xpcom, probes}));
    }
  }

  
  if (childProcesses) {
    for (let {componentsData} of childProcesses) {
      
      for (let json of componentsData) {
        this.componentsData.push(new PerformanceData({json, probes}));
      }
    }
  }

  this.processData = new PerformanceData({xpcom: xpcom.getProcessData(), probes});
}




let Process = {
  
  _initialized: false,

  
  _loader: null,

  
  _idcounter: 0,

  




  get loader() {
    if (this._initialized) {
      return this._loader;
    }
    this._initialized = true;
    this._loader = Services.ppmm;
    if (!this._loader) {
      
      return null;
    }
    this._loader.loadProcessScript("resource://gre/modules/PerformanceStats-content.js",
      true);
    return this._loader;
  },

  




  broadcast: function(topic, payload) {
    if (!this.loader) {
      return;
    }
    this.loader.broadcastAsyncMessage("performance-stats-service-" + topic, {payload});
  },

  











  broadcastAndCollect: Task.async(function*(topic, payload) {
    if (!this.loader || this.loader.childCount == 1) {
      return undefined;
    }
    const TOPIC = "performance-stats-service-" + topic;
    let id = this._idcounter++;

    
    
    let expecting = this.loader.childCount;

    
    let collected = [];
    let deferred = PromiseUtils.defer();
    let observer = function({data}) {
      if (data.id != id) {
        
        
        return;
      }
      if (data.data) {
        collected.push(data.data)
      }
      if (--expecting > 0) {
        
        return;
      }
      deferred.resolve();
    };
    this.loader.addMessageListener(TOPIC, observer);
    this.loader.broadcastAsyncMessage(
      TOPIC,
      {id, payload}
    );

    
    
    let timeout = setTimeout(() => {
      if (expecting == 0) {
        return;
      }
      deferred.resolve();
    }, MAX_WAIT_FOR_CHILD_PROCESS_MS);

    deferred.promise.then(() => {
      clearTimeout(timeout);
    });

    yield deferred.promise;
    this.loader.removeMessageListener(TOPIC, observer);

    return collected;
  })
};
