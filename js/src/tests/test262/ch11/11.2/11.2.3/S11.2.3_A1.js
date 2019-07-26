










if (eval("Number\u0009()") !== 0) {
  $ERROR('#1: Number\\u0009() === 0');
}


if (eval("Number\u000B()") !== 0) {
  $ERROR('#2: Number\\u000B() === 0');  
}


if (eval("Number\u000C()") !== 0) {
  $ERROR('#3: Number\\u000C() === 0');
}


if (eval("Number\u0020()") !== 0) {
  $ERROR('#4: Number\\u0020 === 0');
}


if (eval("Number\u00A0()") !== 0) {
  $ERROR('#5: Number\\u00A0() === 0');
}


if (eval("Number\u000A()") !== 0) {
  $ERROR('#6: Number\\u000A() === 0');  
}


if (eval("Number\u000D()") !== 0) {
  $ERROR('#7: Number\\u000D() === 0');
}


if (eval("Number\u2028()") !== 0) {
  $ERROR('#8: Number\\u2028() === 0');
}


if (eval("Number\u2029()") !== 0) {
  $ERROR('#9: Number\\u2029() === 0');
}


if (eval("Number\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029()") !== 0) {
  $ERROR('#10: Number\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029() === 0');
}

