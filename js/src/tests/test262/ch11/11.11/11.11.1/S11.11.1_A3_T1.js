










if ((false && true) !== false) {
  $ERROR('#1: (false && true) === false');
}


if ((false && false) !== false) {
  $ERROR('#2: (false && false) === false');
}


if ((false && new Boolean(true)) !== false) {
  $ERROR('#3: (false && new Boolean(true)) === false');
}


if ((false && new Boolean(false)) !== false) {
  $ERROR('#4: (false && new Boolean(false)) === false');
}

