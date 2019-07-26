










if (eval("0\u0009<\u00091") !== true) {
  $ERROR('#1: (0\\u0009<\\u00091) === true');
}


if (eval("0\u000B<\u000B1") !== true) {
  $ERROR('#2: (0\\u000B<\\u000B1) === true');  
}


if (eval("0\u000C<\u000C1") !== true) {
  $ERROR('#3: (0\\u000C<\\u000C1) === true');
}


if (eval("0\u0020<\u00201") !== true) {
  $ERROR('#4: (0\\u0020<\\u00201) === true');
}


if (eval("0\u00A0<\u00A01") !== true) {
  $ERROR('#5: (0\\u00A0<\\u00A01) === true');
}


if (eval("0\u000A<\u000A1") !== true) {
  $ERROR('#6: (0\\u000A<\\u000A1) === true');  
}


if (eval("0\u000D<\u000D1") !== true) {
  $ERROR('#7: (0\\u000D<\\u000D1) === true');
}


if (eval("0\u2028<\u20281") !== true) {
  $ERROR('#8: (0\\u2028<\\u20281) === true');
}


if (eval("0\u2029<\u20291") !== true) {
  $ERROR('#9: (0\\u2029<\\u20291) === true');
}


if (eval("0\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029<\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u20291") !== true) {
  $ERROR('#10: (0\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029<\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u20291) === true');
}

