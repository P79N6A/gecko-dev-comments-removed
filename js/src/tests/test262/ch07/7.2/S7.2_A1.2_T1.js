










eval("\u000Bvar\u000Bx\u000B=\u000B1\u000B");
if (x !== 1) {
  $ERROR('#1: eval("\\u000Bvar\\u000Bx\\u000B=\\u000B1\\u000B"); x === 1. Actual: ' + (x));
}


eval("\u000B" + "var" + "\u000B" + "x" + "\u000B" + "=" + "\u000B" + "1" + "\u000B");
if (x !== 1) {
  $ERROR('#2: eval("\\u000B" + "var" + "\\u000B" + "x" + "\\u000B" + "=" + "\\u000B" + "1" + "\\u000B"); x === 1. Actual: ' + (x));
}


eval("\vvar\vx\v=\v1\v");
if (x !== 1) {
  $ERROR('#3: eval("\\vvar\\vx\\v=\\v1\\v"); x === 1. Actual: ' + (x));
}


eval("\v" + "var" + "\v" + "x" + "\v" + "=" + "\v" + "1" + "\v");
if (x !== 1) {
  $ERROR('#4: eval("\\v" + "var" + "\\v" + "x" + "\\v" + "=" + "\\v" + "1" + "\\v"); x === 1. Actual: ' + (x));
}


eval("\u000B" + "var" + "\v" + "x" + "\u000B" + "=" + "\v" + "1" + "\u000B");
if (x !== 1) {
  $ERROR('#5: eval("\\u000B" + "var" + "\\v" + "x" + "\\u000B" + "=" + "\\v" + "1" + "\\u000B"); x === 1. Actual: ' + (x));
}

