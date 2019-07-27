

load(libdir + 'census.js');

var dbg = new Debugger;
var census0 = dbg.memory.takeCensus();
Census.walkCensus(census0, "census0", Census.assertAllZeros);

var g1 = newGlobal();
g1.eval('var a = [];');
g1.eval('function add(f) { a.push({}); a.push(f ? (() => undefined) : null); }');
g1.eval('function remove() { a.pop(); a.pop(); }');
g1.add();
g1.remove();


dbg.addDebuggee(g1);
var census1 = dbg.memory.takeCensus();
Census.walkCensus(census1, "census1", Census.assertAllNotLessThan(census0));

function pointCheck(label, lhs, rhs, objComp, funComp) {
  print(label);
  try {
    assertEq(objComp(lhs.objects.Object.count, rhs.objects.Object.count), true);
    assertEq(funComp(lhs.objects.Function.count, rhs.objects.Function.count), true);
  } catch (ex) {
    print("pointCheck failed: " + ex);
    print("lhs: " + JSON.stringify(lhs, undefined, 2));
    print("rhs: " + JSON.stringify(rhs, undefined, 2));

    
    
    var upload_dir = os.getenv("MOZ_UPLOAD_DIR") || ".";
    redirect(upload_dir + "/Memory-takeCensus-02.txt");
    print("pointCheck failed: " + ex);
    print("lhs: " + JSON.stringify(lhs, undefined, 2));
    print("rhs: " + JSON.stringify(rhs, undefined, 2));

    throw ex;
  }
}

function eq(lhs, rhs) { return lhs === rhs; }
function lt(lhs, rhs) { return lhs < rhs; }
function gt(lhs, rhs) { return lhs > rhs; }



g1.add(false);
var census2 = dbg.memory.takeCensus();
pointCheck("census2", census2, census1, gt, eq);

g1.add(true);
var census3 = dbg.memory.takeCensus();
pointCheck("census3", census3, census2, gt, gt);

g1.add(false);
var census4 = dbg.memory.takeCensus();
pointCheck("census4", census4, census3, gt, eq);




g1.remove();
var census5 = dbg.memory.takeCensus();
pointCheck("census5", census5, census4, lt, eq);

g1.remove();
var census6 = dbg.memory.takeCensus();
pointCheck("census6", census6, census5, lt, lt);
