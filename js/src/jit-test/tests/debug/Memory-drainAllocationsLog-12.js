

load(libdir + "asserts.js");

const dbg = new Debugger();
const root = newGlobal();
dbg.addDebuggee(root);

dbg.memory.trackingAllocationSites = true;
dbg.enabled = false;

root.eval("this.alloc = {}");



let allocs = dbg.memory.drainAllocationsLog();
assertEq(allocs.length, 0);
