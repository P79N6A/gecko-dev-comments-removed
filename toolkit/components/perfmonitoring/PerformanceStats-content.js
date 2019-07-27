











"use strict";

const { utils: Cu, classes: Cc, interfaces: Ci } = Components;
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});

XPCOMUtils.defineLazyModuleGetter(this, "PerformanceStats",
  "resource://gre/modules/PerformanceStats.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");












let gMonitor = PerformanceStats.getMonitor([]);




let isContent = Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT;















Services.cpmm.addMessageListener("performance-stats-service-acquire", function(msg) {
  if (!isContent) {
    return;
  }
  let name = msg.data.payload;
  ensureAcquired(name);
});
















Services.cpmm.addMessageListener("performance-stats-service-release", function(msg) {
  if (!isContent) {
    return;
  }
  
  let probes = gMonitor.getProbeNames
    .filter(x => msg.data.payload.indexOf(x) == -1);
  gMonitor = PerformanceStats.getMonitor(probes);
});







function ensureAcquired(probeNames) {
  let alreadyAcquired = gMonitor.probeNames;

  
  let shouldAcquire = [];
  for (let probeName of probeNames) {
    if (alreadyAcquired.indexOf(probeName) == -1) {
      shouldAcquire.push(probeName)
    }
  }

  if (shouldAcquire.length == 0) {
    return;
  }
  gMonitor = PerformanceStats.getMonitor([...alreadyAcquired, ...shouldAcquire]);
}











Services.cpmm.addMessageListener("performance-stats-service-collect", Task.async(function*(msg) {
  let {id, payload: {probeNames}} = msg.data;
  if (!isContent) {
    
    
    Services.cpmm.sendAsyncMessage("performance-stats-service-collect", {
      id,
      data: null
    });
    return;
  }

  
  
  ensureAcquired(probeNames);

  
  let data = yield gMonitor.promiseSnapshot({probeNames});
  Services.cpmm.sendAsyncMessage("performance-stats-service-collect", {
    id,
    data
  });
}));
