










if (("x" >= "x") !== true) {
  $ERROR('#1: ("x" >= "x") === true');
}


if (("x" >= "") !== true) {
  $ERROR('#2: ("x" >= "") === true');
}


if (("abcd" >= "ab") !== true) {
  $ERROR('#3: ("abcd" >= ab") === true');
}


if (("abc\u0064" >= "abcd") !== true) {
  $ERROR('#4: ("abc\\u0064" >= abc") === true');
}


if (("x" + "y" >= "x") !== true) {
  $ERROR('#5: ("x" + "y" >= "x") === true');
}


var x = "x";
if ((x + 'y' >= x) !== true) {
  $ERROR('#6: var x = "x"; (x + "y" >= x) === true');
}


if (("a\u0000a" >= "a\u0000") !== true) {
  $ERROR('#7: ("a\\u0000a" >= "a\\u0000") === true');
}


if ((" x" >= "x") !== false) {
  $ERROR('#8: (" x" >= "x") === false');
}


