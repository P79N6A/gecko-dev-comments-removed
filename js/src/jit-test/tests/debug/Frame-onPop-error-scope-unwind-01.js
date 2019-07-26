

var g = newGlobal();
var dbg = new Debugger(g);
var correct;
dbg.onEnterFrame = function (f) {
  if (f.callee && f.callee.name == "f") {
    f.onPop = function() {
      
      correct = (f.environment.getVariable("e") === undefined &&
                 f.environment.getVariable("outer") === 43);
    };
  }
};
g.eval("" + function f() {
  var outer = 43;
  try {
    eval("");
    throw 42;
  } catch (e) {
    noSuchFn(e);
  }
});


try {
  g.eval("f();");
} catch (e) {
  
}

assertEq(correct, true);
