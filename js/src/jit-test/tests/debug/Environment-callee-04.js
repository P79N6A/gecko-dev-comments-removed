



var g = newGlobal();
var dbg = new Debugger(g);

dbg.onDebuggerStatement = function (frame) {
  assertEq(frame.older.environment.parent.callee, null);
}

g.evaluate(`

           function h() { debugger; }
           (function () {
             return function () {
               h();
               return 1;
             }
           })()();

           `);
