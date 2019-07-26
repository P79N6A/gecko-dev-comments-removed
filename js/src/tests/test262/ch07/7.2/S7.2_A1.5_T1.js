










eval("\u00A0var\u00A0x\u00A0=\u00A01\u00A0");
if (x !== 1) {
  $ERROR('#1: eval("\\u00A0var\\u00A0x\\u00A0=\\u00A01\\u00A0"); x === 1. Actual: ' + (x));
}


eval("\u00A0" + "var" + "\u00A0" + "x" + "\u00A0" + "=" + "\u00A0" + "1" + "\u00A0");
if (x !== 1) {
  $ERROR('#2: eval("\\u00A0" + "var" + "\\u00A0" + "x" + "\\u00A0" + "=" + "\\u00A0" + "1" + "\\u00A0"); x === 1. Actual: ' + (x));
}

