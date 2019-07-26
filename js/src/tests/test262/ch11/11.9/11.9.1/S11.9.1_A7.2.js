











if ((new Boolean(true) == true) !== true) {
  $ERROR('#1: (new Boolean(true) == true) === true');
}


if ((new Number(1) == true) !== true) {
  $ERROR('#2: (new Number(1) == true) === true');
}


if ((new String("1") == true) !== true) {
  $ERROR('#3: (new String("1") == true) === true');
}

