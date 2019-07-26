











if ((1 == new Boolean(true)) !== true) {
  $ERROR('#1: (1 == new Boolean(true)) === true');
}


if ((-1 == new Number(-1)) !== true) {
  $ERROR('#2: (-1 == new Number(-1)) === true');
}


if ((-1 == new String("-1")) !== true) {
  $ERROR('#3: (-1 == new String("-1")) === true');
}

