










eval("/*\u000B multi line \u000B comment \u000B*/");


var x = 0;
eval("/*\u000B multi line \u000B comment \u000B x = 1;*/");
if (x !== 0) {
  $ERROR('#1: var x = 0; eval("/*\\u000B multi line \\u000B comment \\u000B x = 1;*/"); x === 0. Actual: ' + (x));
}

