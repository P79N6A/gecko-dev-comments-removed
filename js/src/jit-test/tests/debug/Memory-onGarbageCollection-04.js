
























gczeal(0);

const debuggeree = newGlobal();
const debuggee = debuggeree.debuggee = newGlobal();

debuggeree.eval(
  `
  const dbg = new Debugger(this.debuggee);
  let fired = 0;
  dbg.memory.onGarbageCollection = _ => {
    fired++;
    gc(this);
  };
  `
);

const dbg = new Debugger(debuggeree);
let fired = 0;
dbg.memory.onGarbageCollection = _ => {
  fired++;
};

debuggee.eval(`gc(this)`);

assertEq(debuggeree.fired, 1);
assertEq(fired, 1);
