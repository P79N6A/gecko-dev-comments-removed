




"use strict";

this.EXPORTED_SYMBOLS = ["PerformanceStats"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;









Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

let performanceStatsService =
  Cc["@mozilla.org/toolkit/performance-stats-service;1"].
  getService(Ci.nsIPerformanceStatsService);


const PROPERTIES_NUMBERED = ["totalUserTime", "totalSystemTime", "totalCPOWTime", "ticks"];
const PROPERTIES_META = ["name", "addonId", "isSystem"];
const PROPERTIES_FLAT = [...PROPERTIES_NUMBERED, ...PROPERTIES_META];


































function PerformanceData(xpcom) {
  for (let k of PROPERTIES_FLAT) {
    this[k] = xpcom[k];
  }
  this.durations = xpcom.getDurations();
}
PerformanceData.prototype = {
  




  equals: function(to) {
    if (!(to instanceof PerformanceData)) {
      throw new TypeError();
    }
    for (let k of PROPERTIES_FLAT) {
      if (this[k] != to[k]) {
        return false;
      }
    }
    for (let i = 0; i < this.durations.length; ++i) {
      if (to.durations[i] != this.durations[i]) {
        return false;
      }
    }
    return true;
  },

  







  substract: function(to = null) {
    return new PerformanceDiff(this, to);
  }
};
















function PerformanceDiff(current, old = null) {
  for (let k of PROPERTIES_META) {
    this[k] = current[k];
  }

  if (old) {
    if (!(old instanceof PerformanceData)) {
      throw new TypeError();
    }
    if (current.durations.length != old.durations.length) {
      throw new TypeError("Internal error: mismatched length for `durations`.");
    }

    this.durations = [];

    this.longestDuration = -1;

    for (let i = 0; i < current.durations.length; ++i) {
      let delta = current.durations[i] - old.durations[i];
      this.durations[i] = delta;
      if (delta > 0) {
        this.longestDuration = i;
      }
    }
    for (let k of PROPERTIES_NUMBERED) {
      this[k] = current[k] - old[k];
    }
  } else {
    this.durations = current.durations.slice(0);

    for (let k of PROPERTIES_NUMBERED) {
      this[k] = current[k];
    }

    this.longestDuration = -1;
    for (let i = this.durations.length - 1; i >= 0; --i) {
      if (this.durations[i] > 0) {
        this.longestDuration = i;
        break;
      }
    }
  }
}




function Snapshot(xpcom) {
  this.componentsData = [];
  let enumeration = xpcom.getComponentsData().enumerate();
  while (enumeration.hasMoreElements()) {
    let stat = enumeration.getNext().QueryInterface(Ci.nsIPerformanceStats);
    this.componentsData.push(new PerformanceData(stat));
  }
  this.processData = new PerformanceData(xpcom.getProcessData());
}


this.PerformanceStats = {
  


  init() {
    
    
    
    
    
    
    
    
    
  },

  




  getSnapshot() {
    return new Snapshot(performanceStatsService.getSnapshot());
  },
};

performanceStatsService.isStopwatchActive = true;
