










if (eval("var x = 0; typeof\u0009x") !== "number") {
  $ERROR('#1: var x = 0; typeof\\u0009x; x === "number". Actual: ' + (x));
}


if (eval("var x = 0; typeof\u000Bx") !== "number") {
  $ERROR('#2: var x = 0; typeof\\u000Bx; x === "number". Actual: ' + (x));  
}


if (eval("var x = 0; typeof\u000Cx") !== "number") {
  $ERROR('#3: var x = 0; typeof\\u000Cx; x === "number". Actual: ' + (x));
}


if (eval("var x = 0; typeof\u0020x") !== "number") {
  $ERROR('#4: var x = 0; typeof\\u0020x; x === "number". Actual: ' + (x));
}


if (eval("var x = 0; typeof\u00A0x") !== "number") {
  $ERROR('#5: var x = 0; typeof\\u00A0x; x === "number". Actual: ' + (x));
}


if (eval("var x = 0; typeof\u000Ax") !== "number") {
  $ERROR('#6: var x = 0; typeof\\u000Ax; x === "number". Actual: ' + (x));  
}


if (eval("var x = 0; typeof\u000Dx") !== "number") {
  $ERROR('#7: var x = 0; typeof\\u000Dx; x === "number". Actual: ' + (x));
}


if (eval("var x = 0; typeof\u2028x") !== "number") {
  $ERROR('#8: var x = 0; typeof\\u2028x; x === "number". Actual: ' + (x));
}


if (eval("var x = 0; typeof\u2029x") !== "number") {
  $ERROR('#9: var x = 0; typeof\\u2029x; x === "number". Actual: ' + (x));
}


if (eval("var x = 0; typeof\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029x") !== "number") {
  $ERROR('#10: var x = 0; typeof\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029x; x === "number". Actual: ' + (x));
}

