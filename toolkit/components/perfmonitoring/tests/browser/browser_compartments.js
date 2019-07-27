


"use strict";






Cu.import("resource://gre/modules/PerformanceStats.jsm", this);
Cu.import("resource://testing-common/ContentTask.jsm", this);

const URL = "http://example.com/browser/toolkit/components/perfmonitoring/tests/browser/browser_compartments.html?test=" + Math.random();
const PARENT_TITLE = `Main frame for test browser_compartments.js ${Math.random()}`;
const FRAME_TITLE = `Subframe for test browser_compartments.js ${Math.random()}`;


function frameScript() {
  try {
    "use strict";

    const { utils: Cu, classes: Cc, interfaces: Ci } = Components;
    Cu.import("resource://gre/modules/PerformanceStats.jsm");

    let performanceStatsService =
      Cc["@mozilla.org/toolkit/performance-stats-service;1"].
      getService(Ci.nsIPerformanceStatsService);

    
    let monitor = PerformanceStats.getMonitor(["jank", "cpow", "ticks"]);

    addMessageListener("compartments-test:getStatistics", () => {
      try {
        monitor.promiseSnapshot().then(snapshot => {
          sendAsyncMessage("compartments-test:getStatistics", snapshot);
        });
      } catch (ex) {
        Cu.reportError("Error in content (getStatistics): " + ex);
        Cu.reportError(ex.stack);
      }
    });

    addMessageListener("compartments-test:setTitles", titles => {
      try {
        content.document.title = titles.data.parent;
        for (let i = 0; i < content.frames.length; ++i) {
          content.frames[i].postMessage({title: titles.data.frames}, "*");
        }
        console.log("content", "Done setting titles", content.document.title);
        sendAsyncMessage("compartments-test:setTitles");
      } catch (ex) {
        Cu.reportError("Error in content (setTitles): " + ex);
        Cu.reportError(ex.stack);
      }
    });
  } catch (ex) {
    Cu.reportError("Error in content (setup): " + ex);
    Cu.reportError(ex.stack);    
  }
}



let SilentAssert = {
  equal: function(a, b, msg) {
    if (a == b) {
      return;
    }
    Assert.equal(a, b, msg);
  },
  notEqual: function(a, b, msg) {
    if (a != b) {
      return;
    }
    Assert.notEqual(a, b, msg);
  },
  ok: function(a, msg) {
    if (a) {
      return;
    }
    Assert.ok(a, msg);
  },
  leq: function(a, b, msg) {
    this.ok(a <= b, `${msg}: ${a} <= ${b}`);
  }
};


function monotinicity_tester(source, testName) {
  
  
  
  
  
  
  let previous = {
    processData: null,
    componentsMap: new Map(),
  };

  let sanityCheck = function(prev, next) {
    if (prev == null) {
      return;
    }
    for (let k of ["name", "addonId", "isSystem"]) {
      SilentAssert.equal(prev[k], next[k], `Sanity check (${testName}): ${k} hasn't changed.`);
    }
    for (let [probe, k] of [
      ["jank", "totalUserTime"],
      ["jank", "totalSystemTime"],
      ["cpow", "totalCPOWTime"],
      ["ticks", "ticks"]
    ]) {
      SilentAssert.equal(typeof next[probe][k], "number", `Sanity check (${testName}): ${k} is a number.`);
      SilentAssert.leq(prev[probe][k], next[probe][k], `Sanity check (${testName}): ${k} is monotonic.`);
      SilentAssert.leq(0, next[probe][k], `Sanity check (${testName}): ${k} is >= 0.`)
    }
    SilentAssert.equal(prev.jank.durations.length, next.jank.durations.length);
    for (let i = 0; i < next.jank.durations.length; ++i) {
      SilentAssert.ok(typeof next.jank.durations[i] == "number" && next.jank.durations[i] >= 0,
        `Sanity check (${testName}): durations[${i}] is a non-negative number.`);
      SilentAssert.leq(prev.jank.durations[i], next.jank.durations[i],
        `Sanity check (${testName}): durations[${i}] is monotonic.`);
    }
    for (let i = 0; i < next.jank.durations.length - 1; ++i) {
      SilentAssert.leq(next.jank.durations[i + 1], next.jank.durations[i],
        `Sanity check (${testName}): durations[${i}] >= durations[${i + 1}].`)
    }
  };
  let iteration = 0;
  let frameCheck = Task.async(function*() {
    let name = `${testName}: ${iteration++}`;
    let snapshot = yield source();
    if (!snapshot) {
      
      
      window.clearInterval(interval);
      return;
    }

    
    sanityCheck(previous.processData, snapshot.processData);
    SilentAssert.equal(snapshot.processData.isSystem, true);
    SilentAssert.equal(snapshot.processData.name, "<process>");
    SilentAssert.equal(snapshot.processData.addonId, "");
    previous.procesData = snapshot.processData;

    
    let set = new Set();
    let map = new Map();
    for (let item of snapshot.componentsData) {
	 for (let [probe, k] of [
        ["jank", "totalUserTime"],
        ["jank", "totalSystemTime"],
        ["cpow", "totalCPOWTime"]
      ]) {
        SilentAssert.leq(item[probe][k], snapshot.processData[probe][k],
          `Sanity check (${testName}): component has a lower ${k} than process`);
      }

      let key = `{name: ${item.name}, window: ${item.windowId}, addonId: ${item.addonId}, isSystem: ${item.isSystem}}`;
      if (set.has(key)) {
        
        
        
        map.delete(key);
        continue;
      }
      map.set(key, item);
      set.add(key);
    }
    for (let [key, item] of map) {
      sanityCheck(previous.componentsMap.get(key), item);
      previous.componentsMap.set(key, item);
    }
    info(`Deactivating deduplication check (Bug 1150045)`);
    if (false) {
      SilentAssert.equal(set.size, snapshot.componentsData.length);
    }
  });
  let interval = window.setInterval(frameCheck, 300);
  registerCleanupFunction(() => {
    window.clearInterval(interval);
  });
}

add_task(function* test() {
  let monitor = PerformanceStats.getMonitor(["jank", "cpow", "ticks"]);

  info("Extracting initial state");
  let stats0 = yield monitor.promiseSnapshot();
  Assert.notEqual(stats0.componentsData.length, 0, "There is more than one component");
  Assert.ok(!stats0.componentsData.find(stat => stat.name.indexOf(URL) != -1),
    "The url doesn't appear yet");

  let newTab = gBrowser.addTab();
  let browser = newTab.linkedBrowser;
  
  info("Setting up monitoring in the tab");
  yield ContentTask.spawn(newTab.linkedBrowser, null, frameScript);

  info("Opening URL");
  newTab.linkedBrowser.loadURI(URL);

  if (Services.sysinfo.getPropertyAsAString("name") == "Windows_NT") {
    info("Deactivating sanity checks under Windows (bug 1151240)");
  } else {
    info("Setting up sanity checks");
    monotinicity_tester(() => monitor.promiseSnapshot(), "parent process");
    monotinicity_tester(() => promiseContentResponseOrNull(browser, "compartments-test:getStatistics", null), "content process" );
  }

  let skipTotalUserTime = hasLowPrecision();


  while (true) {
    yield new Promise(resolve => setTimeout(resolve, 100));

    
    
    
    info("Setting titles");
    yield promiseContentResponse(browser, "compartments-test:setTitles", {
      parent: PARENT_TITLE,
      frames: FRAME_TITLE
    });
    info("Titles set");

    let stats = (yield promiseContentResponse(browser, "compartments-test:getStatistics", null));

    let titles = [for(stat of stats.componentsData) stat.title];

    for (let stat of stats.componentsData) {
      info(`Compartment: ${stat.name} => ${stat.title} (${stat.isSystem?"system":"web"})`);
    }

    
    
    
    
    info(`Searching for frame title '${FRAME_TITLE}' in ${JSON.stringify(titles)} (I hope not to find it)`);
    Assert.ok(!titles.includes(FRAME_TITLE), "Searching by title, the frames don't show up in the list of components");

    info(`Searching for window title '${PARENT_TITLE}' in ${JSON.stringify(titles)} (I hope to find it)`);
    let parent = stats.componentsData.find(x => x.title == PARENT_TITLE);
    if (!parent) {
      info("Searching by title, we didn't find the main frame");
      continue;
    }

    if (skipTotalUserTime || parent.jank.totalUserTime > 1000) {
      break;
    } else {
      info(`Not enough CPU time detected: ${parent.jank.totalUserTime}`)
    }
  }

  
  gBrowser.removeTab(newTab);
});
