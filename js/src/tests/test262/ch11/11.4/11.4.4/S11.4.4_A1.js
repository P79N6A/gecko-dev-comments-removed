










if (eval("var x = 0; ++\u0009x") !== 1) {
  $ERROR('#1: var x = 0; ++\\u0009x; x === 1. Actual: ' + (x));
}


if (eval("var x = 0; ++\u000Bx") !== 1) {
  $ERROR('#2: var x = 0; ++\\u000Bx; x === 1. Actual: ' + (x));  
}


if (eval("var x = 0; ++\u000Cx") !== 1) {
  $ERROR('#3: var x = 0; ++\\u000Cx; x === 1. Actual: ' + (x));
}


if (eval("var x = 0; ++\u0020x") !== 1) {
  $ERROR('#4: var x = 0; ++\\u0020x; x === 1. Actual: ' + (x));
}


if (eval("var x = 0; ++\u00A0x") !== 1) {
  $ERROR('#5: var x = 0; ++\\u00A0x; x === 1. Actual: ' + (x));
}


if (eval("var x = 0; ++\u000Ax") !== 1) {
  $ERROR('#6: var x = 0; ++\\u000Ax; x === 1. Actual: ' + (x));  
}


if (eval("var x = 0; ++\u000Dx") !== 1) {
  $ERROR('#7: var x = 0; ++\\u000Dx; x === 1. Actual: ' + (x));
}


if (eval("var x = 0; ++\u2028x") !== 1) {
  $ERROR('#8: var x = 0; ++\\u2028x; x === 1. Actual: ' + (x));
}


if (eval("var x = 0; ++\u2029x") !== 1) {
  $ERROR('#9: var x = 0; ++\\u2029x; x === 1. Actual: ' + (x));
}


if (eval("var x = 0; ++\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029x") !== 1) {
  $ERROR('#10: var x = 0; ++\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029x; x === 1. Actual: ' + (x));
}

