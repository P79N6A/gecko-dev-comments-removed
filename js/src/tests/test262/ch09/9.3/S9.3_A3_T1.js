










if (Number(false) !== +0) {
  $ERROR('#1.1: Number(false) === 0. Actual: ' + (Number(false)));
} else {
  if (1/Number(false) !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: Number(false) === +0. Actual: -0');
  }
}


if (Number(true) !== 1) {
  $ERROR('#2: Number(true) === 1. Actual: ' + (Number(true)));	
}

