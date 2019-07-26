










if (eval("var x = 1; --\u0009x") !== 0) {
  $ERROR('#1: var x = 1; --\\u0009x; x === 0. Actual: ' + (x));
}


if (eval("var x = 1; --\u000Bx") !== 0) {
  $ERROR('#2: var x = 1; --\\u000Bx; x === 0. Actual: ' + (x));  
}


if (eval("var x = 1; --\u000Cx") !== 0) {
  $ERROR('#3: var x = 1; --\\u000Cx; x === 0. Actual: ' + (x));
}


if (eval("var x = 1; --\u0020x") !== 0) {
  $ERROR('#4: var x = 1; --\\u0020x; x === 0. Actual: ' + (x));
}


if (eval("var x = 1; --\u00A0x") !== 0) {
  $ERROR('#5: var x = 1; --\\u00A0x; x === 0. Actual: ' + (x));
}


if (eval("var x = 1; --\u000Ax") !== 0) {
  $ERROR('#6: var x = 1; --\\u000Ax; x === 0. Actual: ' + (x));  
}


if (eval("var x = 1; --\u000Dx") !== 0) {
  $ERROR('#7: var x = 1; --\\u000Dx; x === 0. Actual: ' + (x));
}


if (eval("var x = 1; --\u2028x") !== 0) {
  $ERROR('#8: var x = 1; --\\u2028x; x === 0. Actual: ' + (x));
}


if (eval("var x = 1; --\u2029x") !== 0) {
  $ERROR('#9: var x = 1; --\\u2029x; x === 0. Actual: ' + (x));
}


if (eval("var x = 1; --\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029x") !== 0) {
  $ERROR('#10: var x = 1; --\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029x; x === 0. Actual: ' + (x));
}

