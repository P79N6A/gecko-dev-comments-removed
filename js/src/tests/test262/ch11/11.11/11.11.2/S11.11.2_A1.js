










if ((eval("false\u0009||\u0009true")) !== true) {
  $ERROR('#1: (false\\u0009||\\u0009true) === true');
}


if ((eval("false\u000B||\u000Btrue")) !== true) {
  $ERROR('#2: (false\\u000B||\\u000Btrue) === true');  
}


if ((eval("false\u000C||\u000Ctrue")) !== true) {
  $ERROR('#3: (false\\u000C||\\u000Ctrue) === true');
}


if ((eval("false\u0020||\u0020true")) !== true) {
  $ERROR('#4: (false\\u0020||\\u0020true) === true');
}


if ((eval("false\u00A0||\u00A0true")) !== true) {
  $ERROR('#5: (false\\u00A0||\\u00A0true) === true');
}


if ((eval("false\u000A||\u000Atrue")) !== true) {
  $ERROR('#6: (false\\u000A||\\u000Atrue) === true');  
}


if ((eval("false\u000D||\u000Dtrue")) !== true) {
  $ERROR('#7: (false\\u000D||\\u000Dtrue) === true');
}


if ((eval("false\u2028||\u2028true")) !== true) {
  $ERROR('#8: (false\\u2028||\\u2028true) === true');
}


if ((eval("false\u2029||\u2029true")) !== true) {
  $ERROR('#9: (false\\u2029||\\u2029true) === true');
}



if ((eval("false\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029||\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029true")) !== true) {
  $ERROR('#10: (false\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029||\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029true) === true');
}

