




load(libdir + "jitopts.js");

if (!jitTogglesMatch(Opts_Ion2NoParallelCompilation))
  quit(0);

withJitOptions(Opts_Ion2NoParallelCompilation, function () {
  var g = newGlobal();
  var dbg = new Debugger;

  
  
  g.toggle = function toggle(d) {
    if (d) {
      dbg.addDebuggee(g);
      var frame = dbg.getNewestFrame();
      assertEq(frame.implementation, "ion");
      
      assertEq(frame.environment.getVariable("x").optimizedOut, true);
      assertEq(frame.arguments[1].optimizedOut, true);
    }
  };

  g.eval("" + function f(d, x) { "use strict"; g(d, x); });

  g.eval("" + function g(d, x) {
    "use strict";
    for (var i = 0; i < 200; i++);
    
    function inner() { i = 42; };
    toggle(d);
  });

  g.eval("(" + function test() {
    for (i = 0; i < 5; i++)
      f(false, 42);
    f(true, 42);
  } + ")();");
});
