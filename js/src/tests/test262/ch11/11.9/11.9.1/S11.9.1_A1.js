










if (eval("true\u0009==\u00091") !== true) {
  $ERROR('#1: (true\\u0009==\\u00091) === true');
}


if (eval("true\u000B==\u000B1") !== true) {
  $ERROR('#2: (true\\u000B==\\u000B1) === true');  
}


if (eval("true\u000C==\u000C1") !== true) {
  $ERROR('#3: (true\\u000C==\\u000C1) === true');
}


if (eval("true\u0020==\u00201") !== true) {
  $ERROR('#4: (true\\u0020==\\u00201) === true');
}


if (eval("true\u00A0==\u00A01") !== true) {
  $ERROR('#5: (true\\u00A0==\\u00A01) === true');
}


if (eval("true\u000A==\u000A1") !== true) {
  $ERROR('#6: (true\\u000A==\\u000A1) === true');  
}


if (eval("true\u000D==\u000D1") !== true) {
  $ERROR('#7: (true\\u000D==\\u000D1) === true');
}


if (eval("true\u2028==\u20281") !== true) {
  $ERROR('#8: (true\\u2028==\\u20281) === true');
}


if (eval("true\u2029==\u20291") !== true) {
  $ERROR('#9: (true\\u2029==\\u20291) === true');
}


if (eval("true\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029==\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u20291") !== true) {
  $ERROR('#10: (true\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029==\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u20291) === true');
}

