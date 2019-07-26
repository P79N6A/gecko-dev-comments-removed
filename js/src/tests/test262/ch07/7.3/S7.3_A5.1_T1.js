










eval("/*\u000A multi line \u000A comment \u000A*/");


var x = 0;
eval("/*\u000A multi line \u000A comment \u000A x = 1;*/");
if (x !== 0) {
  $ERROR('#1: var x = 0; eval("/*\\u000A multi line \\u000A comment \\u000A x = 1;*/"); x === 0. Actual: ' + (x));
}

