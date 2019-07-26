










if (+(false) !== +0) {
  $ERROR('#1.1: +(false) === 0. Actual: ' + (+(false)));
} else {
  if (1/+(false) !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: +(false) === +0. Actual: -0');
  }
}


if (+(true) !== 1) {
  $ERROR('#2: +(true) === 1. Actual: ' + (+(true)));	
}

