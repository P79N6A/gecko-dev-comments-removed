










eval("// single line comment\u2028");


var x = 0;
eval("// single line comment\u2028 x = 1;");
if (x !== 1) {
  $ERROR('#1: var x = 0; eval("// single line comment\\u2028 x = 1;"); x === 1. Actual: ' + (x));
}

