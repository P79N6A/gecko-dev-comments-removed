










if (eval("var x = 0; x\u0009--; x") !== -1) {
  $ERROR('#1: var x = 0; x\\u0009--; x === -1. Actual: ' + (x));
}


if (eval("var x = 0; x\u000B--; x") !== -1) {
  $ERROR('#2: var x = 0; x\\u000B--; x === -1. Actual: ' + (x));  
}


if (eval("var x = 0; x\u000C--; x") !== -1) {
  $ERROR('#3: var x = 0; x\\u000C--; x === -1. Actual: ' + (x));
}


if (eval("var x = 0; x\u0020--; x") !== -1) {
  $ERROR('#4: var x = 0; x\\u0020--; x === -1. Actual: ' + (x));
}


if (eval("var x = 0; x\u00A0--; x") !== -1) {
  $ERROR('#5: var x = 0; x\\u00A0--; x === -1. Actual: ' + (x));
}


if (eval("var x = 0; x\u0009\u000B\u000C\u0020\u00A0--; x") !== -1) {
    $ERROR('#6: var x = 0; x\\u0009\\u000B\\u000C\\u0020\\u00A0--; x === -1. Actual: ' + (x));
}

