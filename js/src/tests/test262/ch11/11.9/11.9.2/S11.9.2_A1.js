










if (eval("true\u0009!=\u00091") !== false) {
  $ERROR('#1: (true\\u0009!=\\u00091) === false');
}


if (eval("true\u000B!=\u000B1") !== false) {
  $ERROR('#2: (true\\u000B!=\\u000B1) === false');  
}


if (eval("true\u000C!=\u000C1") !== false) {
  $ERROR('#3: (true\\u000C!=\\u000C1) === false');
}


if (eval("true\u0020!=\u00201") !== false) {
  $ERROR('#4: (true\\u0020!=\\u00201) === false');
}


if (eval("true\u00A0!=\u00A01") !== false) {
  $ERROR('#5: (true\\u00A0!=\\u00A01) === false');
}


if (eval("true\u000A!=\u000A1") !== false) {
  $ERROR('#6: (true\\u000A!=\\u000A1) === false');  
}


if (eval("true\u000D!=\u000D1") !== false) {
  $ERROR('#7: (true\\u000D!=\\u000D1) === false');
}


if (eval("true\u2028!=\u20281") !== false) {
  $ERROR('#8: (true\\u2028!=\\u20281) === false');
}


if (eval("true\u2029!=\u20291") !== false) {
  $ERROR('#9: (true\\u2029!=\\u20291) === false');
}


if (eval("true\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029!=\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u20291") !== false) {
  $ERROR('#10: (true\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029!=\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u20291) === false');
}

