










if (eval("new\u0009Number()") != 0) {
  $ERROR('#1: new\\u0009Number == 0');
}


if (eval("new\u000BNumber()") != 0) {
  $ERROR('#2: new\\u000BNumber == 0');  
}


if (eval("new\u000CNumber()") != 0) {
  $ERROR('#3: new\\u000CNumber == 0');
}


if (eval("new\u0020Number()") != 0) {
  $ERROR('#4: new\\u0020Number == 0');
}


if (eval("new\u00A0Number()") != 0) {
  $ERROR('#5: new\\u00A0Number == 0');
}


if (eval("new\u000ANumber()") != 0) {
  $ERROR('#6: new\\u000ANumber == 0');  
}


if (eval("new\u000DNumber()") != 0) {
  $ERROR('#7: new\\u000DNumber == 0');
}


if (eval("new\u2028Number()") != 0) {
  $ERROR('#8: new\\u2028Number == 0');
}


if (eval("new\u2029Number()") != 0) {
  $ERROR('#9: new\\u2029Number == 0');
}


if (eval("new\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029Number()") != 0) {
  $ERROR('#10: new\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029Number == 0');
}

