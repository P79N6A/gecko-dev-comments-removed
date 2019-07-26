











if ((new Boolean(true) != 1) !== false) {
  $ERROR('#1: (new Boolean(true) != 1) === false');
}


if ((new Number(-1) != -1) !== false) {
  $ERROR('#2: (new Number(-1) != -1) === false');
}


if ((new String("-1") != -1) !== false) {
  $ERROR('#3: (new String("-1") != -1) === false');
}

