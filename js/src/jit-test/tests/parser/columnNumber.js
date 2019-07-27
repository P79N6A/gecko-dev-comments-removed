

load(libdir + 'asserts.js');

assertEq(evaluate("saveStack().column"), 0);
assertEq(evaluate("saveStack().column", { columnNumber: 1729 }), 1729);
assertEq(evaluate("\nsaveStack().column", { columnNumber: 1729 }), 0);
assertEq(evaluate("saveStack().column", { columnNumber: "42" }), 42);
assertThrowsInstanceOf(() => evaluate("saveStack().column", { columnNumber: -10 }),
                       RangeError);
assertThrowsInstanceOf(() => evaluate("saveStack().column", { columnNumber: Math.pow(2,30) }),
                       RangeError);

if (helperThreadCount() > 0) {
  print("offThreadCompileScript 1");
  offThreadCompileScript("saveStack().column", { columnNumber: -10 });
  assertThrowsInstanceOf(runOffThreadScript, RangeError);

  print("offThreadCompileScript 2");
  offThreadCompileScript("saveStack().column", { columnNumber: Math.pow(2,30) });
  assertThrowsInstanceOf(runOffThreadScript, RangeError);

  print("offThreadCompileScript 3");
  offThreadCompileScript("saveStack().column", { columnNumber: 10000 });
  assertEq(runOffThreadScript(), 10000);
}




const maxColumn = Math.pow(2, 22) - 1;
assertEq(evaluate("saveStack().column", { columnNumber: maxColumn }),
         maxColumn);
assertEq(evaluate("saveStack().column", { columnNumber: maxColumn + 1 }),
         0);



assertEq(evaluate(" saveStack().column", { columnNumber: maxColumn }),
         0);
