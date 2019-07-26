










if ((new String(1) << 0) !== 1) {
  $ERROR('#1: (new String(1) << 0) === 1. Actual: ' + ((new String(1) << 0)));
}


if (("-1.234" << 0) !== -1) {
  $ERROR('#2: ("-1.234" << 0) === -1. Actual: ' + (("-1.234" << 0)));
}

