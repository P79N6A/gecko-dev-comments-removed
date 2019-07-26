










if (eval("delete\u00090") !== true) {
  $ERROR('#1: delete\\u00090 === true');
}


if (eval("delete\u000B0") !== true) {
  $ERROR('#2: delete\\u000B0 === true');  
}


if (eval("delete\u000C0") !== true) {
  $ERROR('#3: delete\\u000C0 === true');
}


if (eval("delete\u00200") !== true) {
  $ERROR('#4: delete\\u00200 === true');
}


if (eval("delete\u00A00") !== true) {
  $ERROR('#5: delete\\u00A00 === true');
}


if (eval("delete\u000A0") !== true) {
  $ERROR('#6: delete\\u000A0 === true');  
}


if (eval("delete\u000D0") !== true) {
  $ERROR('#7: delete\\u000D0 === true');
}


if (eval("delete\u20280") !== true) {
  $ERROR('#8: delete\\u20280 === true');
}


if (eval("delete\u20290") !== true) {
  $ERROR('#9: delete\\u20290 === true');
}


if (eval("delete\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u20290") !== true) {
  $ERROR('#10: delete\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u20290 === true');
}

