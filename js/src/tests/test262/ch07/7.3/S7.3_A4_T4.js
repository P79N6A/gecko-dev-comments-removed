










eval("// single line comment\u2029");


var x = 0;
eval("// single line comment\u2029 x = 1;");
if (x !== 1) {
  $ERROR('#1: var x = 0; eval("// single line comment\\u2029 x = 1;"); x === 1. Actual: ' + (x));
}

