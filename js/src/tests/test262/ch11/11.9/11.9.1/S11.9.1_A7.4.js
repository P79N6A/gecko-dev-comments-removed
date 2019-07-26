











if ((new Boolean(true) == 1) !== true) {
  $ERROR('#1: (new Boolean(true) == 1) === true');
}


if ((new Number(-1) == -1) !== true) {
  $ERROR('#2: (new Number(-1) == -1) === true');
}


if ((new String("-1") == -1) !== true) {
  $ERROR('#3: (new String("-1") == -1) === true');
}

