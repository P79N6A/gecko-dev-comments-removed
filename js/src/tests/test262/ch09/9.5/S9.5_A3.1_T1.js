










if ((new Boolean(true) << 0) !== 1) {
  $ERROR('#1: (new Boolean(true) << 0) === 1. Actual: ' + ((new Boolean(true) << 0)));
}


if ((false << 0) !== 0) {
  $ERROR('#2: (false << 0) === 0. Actual: ' + ((false << 0)));
}

