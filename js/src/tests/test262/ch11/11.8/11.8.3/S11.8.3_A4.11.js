










if (("x" <= "x") !== true) {
  $ERROR('#1: ("x" <= "x") === true');
}


if (("" <= "x") !== true) {
  $ERROR('#2: ("" <= "x") === true');
}


if (("ab" <= "abcd") !== true) {
  $ERROR('#3: ("ab" <= abcd") === true');
}


if (("abcd" <= "abc\u0064") !== true) {
  $ERROR('#4: ("abcd" <= abc\\u0064") === true');
}


if (("x" <= "x" + "y") !== true) {
  $ERROR('#5: ("x" <= "x" + "y") === true');
}


var x = "x";
if ((x <= x + "y") !== true) {
  $ERROR('#6: var x = "x"; (x <= x + "y") === true');
}


if (("a\u0000" <= "a\u0000a") !== true) {
  $ERROR('#7: ("a\\u0000" <= "a\\u0000a") === true');
}


if (("x" <= " x") !== false) {
  $ERROR('#8: ("x" <= " x") === false');
}



