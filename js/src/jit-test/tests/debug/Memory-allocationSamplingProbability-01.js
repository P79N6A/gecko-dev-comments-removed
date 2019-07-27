


load(libdir + "asserts.js");

const root = newGlobal();

const dbg = new Debugger();
const wrappedRoot = dbg.addDebuggee(root);



assertThrowsInstanceOf(() => dbg.memory.allocationSamplingProbability = -1,
                       TypeError);
assertThrowsInstanceOf(() => dbg.memory.allocationSamplingProbability = 2,
                       TypeError);


dbg.memory.allocationSamplingProbability = 0;
dbg.memory.allocationSamplingProbability = 1;
dbg.memory.allocationSamplingProbability = .5;
