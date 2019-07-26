










if (eval("'MAX_VALUE'\u0009in\u0009Number") !== true) {
  $ERROR('#1: "MAX_VALUE"\\u0009in\\u0009Number === true');
}


if (eval("'MAX_VALUE'\u000Bin\u000BNumber") !== true) {
  $ERROR('#2: "MAX_VALUE"\\u000Bin\\u000BNumber === true');  
}


if (eval("'MAX_VALUE'\u000Cin\u000CNumber") !== true) {
  $ERROR('#3: "MAX_VALUE"\\u000Cin\\u000CNumber === true');
}


if (eval("'MAX_VALUE'\u0020in\u0020Number") !== true) {
  $ERROR('#4: "MAX_VALUE"\\u0020in\\u0020Number === true');
}


if (eval("'MAX_VALUE'\u00A0in\u00A0Number") !== true) {
  $ERROR('#5: "MAX_VALUE"\\u00A0in\\u00A0Number === true');
}


if (eval("'MAX_VALUE'\u000Ain\u000ANumber") !== true) {
  $ERROR('#6: "MAX_VALUE"\\u000Ain\\u000ANumber === true');  
}


if (eval("'MAX_VALUE'\u000Din\u000DNumber") !== true) {
  $ERROR('#7: "MAX_VALUE"\\u000Din\\u000DNumber === true');
}


if (eval("'MAX_VALUE'\u2028in\u2028Number") !== true) {
  $ERROR('#8: "MAX_VALUE"\\u2028in\\u2028Number === true');
}


if (eval("'MAX_VALUE'\u2029in\u2029Number") !== true) {
  $ERROR('#9: "MAX_VALUE"\\u2029in\\u2029Number === true');
}


if (eval("'MAX_VALUE'\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029in\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029Number") !== true) {
  $ERROR('#10: "MAX_VALUE"\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029in\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029Number === true');
}

