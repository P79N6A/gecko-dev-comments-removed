










if (-0 - -0 !== 0 ) {  
  $ERROR('#1.1: -0 - -0 === 0. Actual: ' + (-0 - -0));
} else {
  if (1 / (-0 - -0) !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: -0 - -0 === + 0. Actual: -0');
  }
}


if (0 - -0 !== 0 ) {  
  $ERROR('#2.1: 0 - -0 === 0. Actual: ' + (0 - -0));
} else {
  if (1 / (0 - -0) !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: 0 - -0 === + 0. Actual: -0');
  }
}


if (-0 - 0 !== -0 ) {  
  $ERROR('#3.1: -0 - 0 === 0. Actual: ' + (-0 - 0));
} else {
  if (1 / (-0 - 0) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#3.2: -0 - 0 === - 0. Actual: +0');
  }
}


if (0 - 0 !== 0 ) {  
  $ERROR('#4.1: 0 - 0 === 0. Actual: ' + (0 - 0));
} else {
  if (1 / (0 - 0) !== Number.POSITIVE_INFINITY) {
    $ERROR('#4.2: 0 - 0 === + 0. Actual: -0');
  }
}

