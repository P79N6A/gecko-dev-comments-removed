










if (("xy" >= "xx") !== true) {
  $ERROR('#1: ("xy" >= "xx") === true');
}


if (("xx" >= "xy") !== false) {
  $ERROR('#2: ("xx" >= "xy") === false');
}


if (("y" >= "x") !== true) {
  $ERROR('#3: ("y" >= "x") === true');
}


if (("aba" >= "aab") !== true) {
  $ERROR('#4: ("aba" >= aab") === true');
}


if (("\u0061\u0061\u0061\u0061" >= "\u0061\u0061\u0061\u0062") !== false) {
  $ERROR('#5: ("\\u0061\\u0061\\u0061\\u0061" >= \\u0061\\u0061\\u0061\\u0062") === false');
}


if (("a\u0000b" >= "a\u0000a") !== true) {
  $ERROR('#6: ("a\\u0000b" >= "a\\u0000a") === true');
}


if (("aa" >= "aB") !== true) {
  $ERROR('#7: ("aa" >= aB") === true');
}

