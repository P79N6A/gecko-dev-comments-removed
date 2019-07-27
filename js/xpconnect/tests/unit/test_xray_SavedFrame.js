


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);

const lowP = Cc["@mozilla.org/nullprincipal;1"].createInstance(Ci.nsIPrincipal);
const midP = [lowP, "http://other.com"];
const highP = Cc["@mozilla.org/systemprincipal;1"].createInstance(Ci.nsIPrincipal);

const low  = new Cu.Sandbox(lowP);
const mid  = new Cu.Sandbox(midP);
const high = new Cu.Sandbox(highP);

function run_test() {
  
  
  
  
  
  
  
  
  

  Cu.evalInSandbox("function highF() { return saveStack(); }", high);

  mid.highF = () => high.highF();
  Cu.evalInSandbox("function midF() { return highF(); }", mid);

  low.midF = () => mid.midF();
  Cu.evalInSandbox("function lowF() { return midF(); }", low);

  const expected = [
    {
      sandbox: low,
      frames: ["lowF"],
    },
    {
      sandbox: mid,
      frames: ["midF", "lowF"],
    },
    {
      sandbox: high,
      frames: ["getSavedFrameInstanceFromSandbox",
               "saveStack",
               "highF",
               "run_test/mid.highF",
               "midF",
               "run_test/low.midF",
               "lowF",
               "run_test",
               "_execute_test",
               null],
    }
  ];

  for (let { sandbox, frames } of expected) {
    high.saveStack = function saveStack() {
      return getSavedFrameInstanceFromSandbox(sandbox);
    };

    const xrayStack = low.lowF();
    equal(xrayStack.functionDisplayName, "getSavedFrameInstanceFromSandbox",
          "Xrays should always be able to see everything.");

    let waived = Cu.waiveXrays(xrayStack);
    do {
      ok(frames.length,
         "There should still be more expected frames while we have actual frames.");
      equal(waived.functionDisplayName, frames.shift(),
            "The waived wrapper should give us the stack's compartment's view.");
      waived = waived.parent;
    } while (waived);
  }
}







function getSavedFrameInstanceFromSandbox(sandbox) {
  const dbg = new Debugger(sandbox);

  dbg.memory.trackingAllocationSites = true;
  Cu.evalInSandbox("new Object", sandbox);
  const allocs = dbg.memory.drainAllocationsLog();
  dbg.memory.trackingAllocationSites = false;

  ok(allocs[0], "We should observe the allocation");
  const { frame } = allocs[0];

  if (sandbox !== high) {
    ok(Cu.isXrayWrapper(frame), "`frame` should be an xray...");
    equal(Object.prototype.toString.call(Cu.waiveXrays(frame)),
          "[object SavedFrame]",
          "...and that xray should wrap a SavedFrame");
  }

  return frame;
}

