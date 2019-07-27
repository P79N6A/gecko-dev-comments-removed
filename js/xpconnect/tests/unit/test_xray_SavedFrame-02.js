


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);

const lowP = Cc["@mozilla.org/nullprincipal;1"].createInstance(Ci.nsIPrincipal);
const highP = Cc["@mozilla.org/systemprincipal;1"].createInstance(Ci.nsIPrincipal);

const low  = new Cu.Sandbox(lowP);
const high = new Cu.Sandbox(highP);

function run_test() {
  
  high.stack = getSavedFrameInstanceFromSandbox(low);
  Cu.evalInSandbox("this.parent = stack.parent", high);
  Cu.evalInSandbox("this.asyncParent = stack.asyncParent", high);
  Cu.evalInSandbox("this.source = stack.source", high);
  Cu.evalInSandbox("this.functionDisplayName = stack.functionDisplayName", high);

  
  low.stack = getSavedFrameInstanceFromSandbox(high);
  try {
    Cu.evalInSandbox("this.parent = stack.parent", low);
  } catch (e) { }
  try {
    Cu.evalInSandbox("this.asyncParent = stack.asyncParent", low);
  } catch (e) { }
  try {
    Cu.evalInSandbox("this.source = stack.source", low);
  } catch (e) { }
  try {
    Cu.evalInSandbox("this.functionDisplayName = stack.functionDisplayName", low);
  } catch (e) { }

  
  let stack = getSavedFrameInstanceFromSandbox(high);
  let parent = stack.parent;
  let asyncParent = stack.asyncParent;
  let source = stack.source;
  let functionDisplayName = stack.functionDisplayName;

  ok(true, "Didn't crash");
}







function getSavedFrameInstanceFromSandbox(sandbox) {
  const dbg = new Debugger(sandbox);

  dbg.memory.trackingAllocationSites = true;
  Cu.evalInSandbox("(function iife() { return new RegExp }())", sandbox);
  const allocs = dbg.memory.drainAllocationsLog().filter(e => e.class === "RegExp");
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
