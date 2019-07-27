

const root1 = newGlobal();
const root2 = newGlobal();
const dbg = new Debugger();

function tenureObjectsAndDrainLog(root) {
  gc();

  dbg.memory.trackingAllocationSites = true;
  dbg.memory.trackingTenurePromotions = true;

  root.eval(
    `
    (function immediate() {
      this.tests = [
        { name: "({})",       cls: "Object", obj: ({})                  },
        { name: "[]",         cls: "Array",  obj: []                    },
        { name: "new Object", cls: "Object", obj: new Object()          },
        { name: "new Array",  cls: "Array",  obj: new Array()           },
        { name: "new Date",   cls: "Date",   obj: new Date()            },
      ];
    }());
    minorgc();
    `
  );

  const promotions = dbg.memory.drainTenurePromotionsLog();

  dbg.memory.trackingAllocationSites = false;
  dbg.memory.trackingTenurePromotions = false;

  return promotions;
}

{
  dbg.addDebuggee(root1);
  dbg.addDebuggee(root2);

  
  assertEq(!!tenureObjectsAndDrainLog(root1).length, true);
  
  assertEq(!!tenureObjectsAndDrainLog(root2).length, true);
}

{
  dbg.removeAllDebuggees();

  
  assertEq(!!tenureObjectsAndDrainLog(root1).length, false);
  
  assertEq(!!tenureObjectsAndDrainLog(root2).length, false);
}

{
  dbg.addDebuggee(root1);

  
  assertEq(!!tenureObjectsAndDrainLog(root1).length, true);
  
  assertEq(!!tenureObjectsAndDrainLog(root2).length, false);
}

{
  dbg.removeAllDebuggees();
  dbg.addDebuggee(root2);

  
  assertEq(!!tenureObjectsAndDrainLog(root1).length, false);
  
  assertEq(!!tenureObjectsAndDrainLog(root2).length, true);
}
