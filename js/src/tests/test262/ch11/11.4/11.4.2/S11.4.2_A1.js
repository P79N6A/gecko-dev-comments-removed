










if (eval("void\u00090") !== undefined) {
  $ERROR('#1: void\\u00090 === undefined');
}


if (eval("void\u000B0") !== undefined) {
  $ERROR('#2: void\\u000B0 === undefined');  
}


if (eval("void\u000C0") !== undefined) {
  $ERROR('#3: void\\u000C0 === undefined');
}


if (eval("void\u00200") !== undefined) {
  $ERROR('#4: void\\u00200 === undefined');
}


if (eval("void\u00A00") !== undefined) {
  $ERROR('#5: void\\u00A00 === undefined');
}


if (eval("void\u000A0") !== undefined) {
  $ERROR('#6: void\\u000A0 === undefined');  
}


if (eval("void\u000D0") !== undefined) {
  $ERROR('#7: void\\u000D0 === undefined');
}


if (eval("void\u20280") !== undefined) {
  $ERROR('#8: void\\u20280 === undefined');
}


if (eval("void\u20290") !== undefined) {
  $ERROR('#9: void\\u20290 === undefined');
}


if (eval("void\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u20290") !== undefined) {
  $ERROR('#10: void\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u20290 === undefined');
}

