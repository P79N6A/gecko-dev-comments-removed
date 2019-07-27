




load(libdir + "asserts.js");

let root1 = newGlobal();
let root2 = newGlobal();

let dbg1 = new Debugger();
let dbg2 = new Debugger();

let d1r1 = dbg1.addDebuggee(root1);
let d2r1 = dbg2.addDebuggee(root1);

let wrappedObj, allocationSite;

function isTrackingAllocations(global, dbgObj) {
  const site = dbgObj.makeDebuggeeValue(global.eval("({})")).allocationSite;
  if (site) {
    assertEq(typeof site, "object");
  }
  return !!site;
}


dbg1.memory.trackingAllocationSites = true;
assertThrowsInstanceOf(() => dbg2.memory.trackingAllocationSites = true,
                       Error);


dbg1.removeDebuggee(root1);
assertEq(isTrackingAllocations(root1, d1r1), false);


dbg2.memory.trackingAllocationSites = true;
assertEq(isTrackingAllocations(root1, d2r1), true);



assertThrowsInstanceOf(() => dbg1.addDebuggee(root1),
                       Error);
assertEq(dbg1.hasDebuggee(root1), false);



dbg2.removeDebuggee(root1);
d1r1 = dbg1.addDebuggee(root1);
assertEq(isTrackingAllocations(root1, d1r1), true);



dbg1.memory.trackingAllocationSites = true;
dbg1.addDebuggee(root1);
dbg2.memory.trackingAllocationSites = false;
let d2r2 = dbg2.addDebuggee(root2);
dbg2.addDebuggee(root1);
assertThrowsInstanceOf(() => dbg2.memory.trackingAllocationSites = true,
                       Error);



assertEq(isTrackingAllocations(root2, d2r2), false);
