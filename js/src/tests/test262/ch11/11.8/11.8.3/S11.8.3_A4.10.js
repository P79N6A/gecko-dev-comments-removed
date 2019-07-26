










if (("x " <= "x") !== false) {
  $ERROR('#1: ("x " <= "x") === false');
}


if (("x" <= "") !== false) {
  $ERROR('#2: ("x" <= "") === false');
}


if (("abcd" <= "ab") !== false) {
  $ERROR('#3: ("abcd" <= ab") === false');
}


if (("abc\u0064" <= "abcd") !== true) {
  $ERROR('#4: ("abc\\u0064" <= abcd") === true');
}


if (("x" + "y" <= "x") !== false) {
  $ERROR('#5: ("x" + "y" <= "x") === false');
}


var x = "x";
if ((x + 'y' <= x) !== false) {
  $ERROR('#6: var x = "x"; (x + "y" <= x) === false');
}


