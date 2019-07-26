










eval("/*\u000C multi line \u000C comment \u000C*/");


var x = 0;
eval("/*\u000C multi line \u000C comment \u000C x = 1;*/");
if (x !== 0) {
  $ERROR('#1: var x = 0; eval("/*\\u000C multi line \\u000C comment \\u000C x = 1;*/"); x === 0. Actual: ' + (x));
}

