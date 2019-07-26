











if (("1" != new Boolean(true)) !== false) {
  $ERROR('#1: ("1" != new Boolean(true)) === false');
}


if (("-1" != new Number(-1)) !== false) {
  $ERROR('#2: ("-1" != new Number(-1)) === false');
}


if (("x" != new String("x")) !== false) {
  $ERROR('#3: ("x" != new String("x")) === false');
}

