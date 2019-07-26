










if (eval("1\u0009>=\u00091") !== true) {
  $ERROR('#1: (1\\u0009>=\\u00091) === true');
}


if (eval("1\u000B>=\u000B1") !== true) {
  $ERROR('#2: (1\\u000B>=\\u000B1) === true');  
}


if (eval("1\u000C>=\u000C1") !== true) {
  $ERROR('#3: (1\\u000C>=\\u000C1) === true');
}


if (eval("1\u0020>=\u00201") !== true) {
  $ERROR('#4: (1\\u0020>=\\u00201) === true');
}


if (eval("1\u00A0>=\u00A01") !== true) {
  $ERROR('#5: (1\\u00A0>=\\u00A01) === true');
}


if (eval("1\u000A>=\u000A1") !== true) {
  $ERROR('#6: (1\\u000A>=\\u000A1) === true');  
}


if (eval("1\u000D>=\u000D1") !== true) {
  $ERROR('#7: (1\\u000D>=\\u000D1) === true');
}


if (eval("1\u2028>=\u20281") !== true) {
  $ERROR('#8: (1\\u2028>=\\u20281) === true');
}


if (eval("1\u2029>=\u20291") !== true) {
  $ERROR('#9: (1\\u2029>=\\u20291) === true');
}


if (eval("1\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029>=\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u20291") !== true) {
  $ERROR('#10: (1\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029>=\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u20291) === true');
}

