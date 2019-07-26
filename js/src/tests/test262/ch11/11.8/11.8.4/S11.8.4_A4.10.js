










if (("x" >= "x ") !== false) {
  $ERROR('#1: ("x" >= "x ") === false');
}


if (("" >= "x") !== false) {
  $ERROR('#2: ("" >= "x") === false');
}


if (("ab" >= "abcd") !== false) {
  $ERROR('#3: ("ab" >= abcd") === false');
}


if (("abcd" >= "abc\u0064") !== true) {
  $ERROR('#4: ("abcd" >= abc\\u0064") === true');
}


if (("x" >= "x" + "y") !== false) {
  $ERROR('#5: ("x" >= "x" + "y") === false');
}


var x = "x";
if ((x >= x + "y") !== false) {
  $ERROR('#6: var x = "x"; (x >= x + "y") === false');
}

