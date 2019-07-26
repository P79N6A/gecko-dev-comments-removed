










if ((eval("true\u0009&&\u0009true")) !== true) {
  $ERROR('#1: (true\\u0009&&\\u0009true) === true');
}


if ((eval("true\u000B&&\u000Btrue")) !== true) {
  $ERROR('#2: (true\\u000B&&\\u000Btrue) === true');  
}


if ((eval("true\u000C&&\u000Ctrue")) !== true) {
  $ERROR('#3: (true\\u000C&&\\u000Ctrue) === true');
}


if ((eval("true\u0020&&\u0020true")) !== true) {
  $ERROR('#4: (true\\u0020&&\\u0020true) === true');
}


if ((eval("true\u00A0&&\u00A0true")) !== true) {
  $ERROR('#5: (true\\u00A0&&\\u00A0true) === true');
}


if ((eval("true\u000A&&\u000Atrue")) !== true) {
  $ERROR('#6: (true\\u000A&&\\u000Atrue) === true');  
}


if ((eval("true\u000D&&\u000Dtrue")) !== true) {
  $ERROR('#7: (true\\u000D&&\\u000Dtrue) === true');
}


if ((eval("true\u2028&&\u2028true")) !== true) {
  $ERROR('#8: (true\\u2028&&\\u2028true) === true');
}


if ((eval("true\u2029&&\u2029true")) !== true) {
  $ERROR('#9: (true\\u2029&&\\u2029true) === true');
}



if ((eval("true\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029&&\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029true")) !== true) {
  $ERROR('#10: (true\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029&&\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029true) === true');
}

