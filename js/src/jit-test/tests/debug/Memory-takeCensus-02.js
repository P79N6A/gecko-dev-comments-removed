

load(libdir + 'census.js');

var dbg = new Debugger;
var census0 = dbg.memory.takeCensus();
Census.walkCensus(census0, "census0", Census.assertAllZeros);

var g1 = newGlobal();
g1.eval('var a = [];');
g1.eval('function add() { a.push({}); }');
g1.eval('function remove() { a.pop({}); }');
g1.add();
g1.remove();


dbg.addDebuggee(g1);
var census1 = dbg.memory.takeCensus();
Census.walkCensus(census1, "census1", Census.assertAllNotLessThan(census0));



g1.add();
var census2 = dbg.memory.takeCensus();
assertEq(census2.count > census1.count, true);

g1.add();
var census3 = dbg.memory.takeCensus();
assertEq(census3.count > census2.count, true);

g1.add();
var census4 = dbg.memory.takeCensus();
assertEq(census4.count > census3.count, true);



g1.remove();
var census5 = dbg.memory.takeCensus();
assertEq(census5.count < census4.count, true);

g1.remove();
var census6 = dbg.memory.takeCensus();
assertEq(census6.count < census5.count, true);
