











if ((eval("x\u0009=\u0009true")) !== true) {
  $ERROR('#1: (x\\u0009=\\u0009true) === true');
}


if ((eval("x\u000B=\u000Btrue")) !== true) {
  $ERROR('#2: (x\\u000B=\\u000Btrue) === true');
}


if ((eval("x\u000C=\u000Ctrue")) !== true) {
  $ERROR('#3: (x\\u000C=\\u000Ctrue) === true');
}


if ((eval("x\u0020=\u0020true")) !== true) {
  $ERROR('#4: (x\\u0020=\\u0020true) === true');
}


if ((eval("x\u00A0=\u00A0true")) !== true) {
  $ERROR('#5: (x\\u00A0=\\u00A0true) === true');
}


if ((eval("x\u000A=\u000Atrue")) !== true) {
  $ERROR('#6: (x\\u000A=\\u000Atrue) === true');
}


if ((eval("x\u000D=\u000Dtrue")) !== true) {
  $ERROR('#7: (x\\u000D=\\u000Dtrue) === true');
}


if ((eval("x\u2028=\u2028true")) !== true) {
  $ERROR('#8: (x\\u2028=\\u2028true) === true');
}


if ((eval("x\u2029=\u2029true")) !== true) {
  $ERROR('#9: (x\\u2029=\\u2029true) === true');
}



if ((eval("x\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029=\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029true")) !== true) {
  $ERROR('#10: (x\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029=\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029true) === true');
}

