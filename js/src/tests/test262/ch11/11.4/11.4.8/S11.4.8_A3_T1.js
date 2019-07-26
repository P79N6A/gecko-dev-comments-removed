










if (~false !== -1) {
  $ERROR('#1: ~false === -1. Actual: ' + (~false));
}


if (~new Boolean(true) !== -2) {
  $ERROR('#2: ~new Boolean(true) === -2. Actual: ' + (~new Boolean(true)));
}


if (~new Boolean(false) !== -1) {
  $ERROR('#3: ~new Boolean(false) === -1. Actual: ' + (~new Boolean(false)));
}

