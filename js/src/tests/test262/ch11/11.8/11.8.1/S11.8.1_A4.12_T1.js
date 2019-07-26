










if (("xx" < "xy") !== true) {
  $ERROR('#1: ("xx" < "xy") === true');
}


if (("xy" < "xx") !== false) {
  $ERROR('#2: ("xy" < "xx") === false');
}


if (("x" < "y") !== true) {
  $ERROR('#3: ("x" < y") === true');
}


if (("aab" < "aba") !== true) {
  $ERROR('#4: ("aab" < aba") === true');
}


if (("\u0061\u0061\u0061\u0062" < "\u0061\u0061\u0061\u0061") !== false) {
  $ERROR('#5: ("\\u0061\\u0061\\u0061\\u0062" < \\u0061\\u0061\\u0061\\u0061") === false');
}


if (("a\u0000a" < "a\u0000b") !== true) {
  $ERROR('#6: ("a\\u0000a" < "a\\u0000b") === true');
}


if (("aB" < "aa") !== true) {
  $ERROR('#7: ("aB" < aa") === true');
}

