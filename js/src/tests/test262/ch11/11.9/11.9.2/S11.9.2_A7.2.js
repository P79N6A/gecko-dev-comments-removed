











if ((new Boolean(true) != true) !== false) {
  $ERROR('#1: (new Boolean(true) != true) === false');
}


if ((new Number(1) != true) !== false) {
  $ERROR('#2: (new Number(1) != true) === false');
}


if ((new String("1") != true) !== false) {
  $ERROR('#3: (new String("1") != true) === false');
}

