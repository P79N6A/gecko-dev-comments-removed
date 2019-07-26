










if (eval("!\u0009true") !== false) {
  $ERROR('#true: !\\u0009true === false');
}


if (eval("!\u000Btrue") !== false) {
  $ERROR('#2: !\\u000Btrue === false');  
}


if (eval("!\u000Ctrue") !== false) {
  $ERROR('#3: !\\u000Ctrue === false');
}


if (eval("!\u0020true") !== false) {
  $ERROR('#4: !\\u0020 === false');
}


if (eval("!\u00A0true") !== false) {
  $ERROR('#5: !\\u00A0true === false');
}


if (eval("!\u000Atrue") !== false) {
  $ERROR('#6: !\\u000Atrue === false');  
}


if (eval("!\u000Dtrue") !== false) {
  $ERROR('#7: !\\u000Dtrue === false');
}


if (eval("!\u2028true") !== false) {
  $ERROR('#8: !\\u2028true === false');
}


if (eval("!\u2029true") !== false) {
  $ERROR('#9: !\\u2029true === false');
}


if (eval("!\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029true") !== false) {
  $ERROR('#10: !\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029true === false');
}

