










eval("// single line comment\u000D");


var x = 0;
eval("// single line comment\u000D x = 1;");
if (x !== 1) {
  $ERROR('#1: var x = 0; eval("// single line comment\\u000D x = 1;"); x === 1. Actual: ' + (x));
}

