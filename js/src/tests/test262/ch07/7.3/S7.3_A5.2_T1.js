










eval("/*\u000D multi line \u000D comment \u000D*/");


var x = 0;
eval("/*\u000D multi line \u000D comment \u000D x = 1;*/");
if (x !== 0) {
  $ERROR('#1: var x = 0; eval("/*\\u000D multi line \\u000D comment \\u000D x = 1;*/"); x === 0. Actual: ' + (x));
}

