










eval("\u2028var\u2028x\u2028=\u20281\u2028");
if (x !== 1) {
  $ERROR('#1: eval("\\u2028var\\u2028x\\u2028=\\u20281\\u2028"); x === 1. Actual: ' + (x));
}


eval("\u2028" + "var" + "\u2028" + "x" + "\u2028" + "=" + "\u2028" + "1" + "\u2028");
if (x !== 1) {
  $ERROR('#2: eval("\\u2028" + "var" + "\\u2028" + "x" + "\\u2028" + "=" + "\\u2028" + "1" + "\\u2028"); x === 1. Actual: ' + (x));
}


