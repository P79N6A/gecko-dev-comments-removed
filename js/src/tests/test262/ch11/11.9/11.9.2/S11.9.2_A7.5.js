











if ((1 != new Boolean(true)) !== false) {
  $ERROR('#1: (1 != new Boolean(true)) === false');
}


if ((-1 != new Number(-1)) !== false) {
  $ERROR('#2: (-1 != new Number(-1)) === false');
}


if ((-1 != new String("-1")) !== false) {
  $ERROR('#3: (-1 != new String("-1")) === false');
}

