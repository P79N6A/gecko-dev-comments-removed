










if (-Number.MIN_VALUE + Number.MIN_VALUE !== +0) {  
  $ERROR('#1.1: -Number.MIN_VALUE + Number.MIN_VALUE === 0. Actual: ' + (-Number.MIN_VALUE + Number.MIN_VALUE));
} else {
  if (1 / (-Number.MIN_VALUE + Number.MIN_VALUE) !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: -Number.MIN_VALUE + Number.MIN_VALUE === + 0. Actual: -0');
  }
}


if (-Number.MAX_VALUE + Number.MAX_VALUE !== +0) {  
  $ERROR('#2.1: -Number.MAX_VALUE + Number.MAX_VALUE === 0. Actual: ' + (-Number.MAX_VALUE + Number.MAX_VALUE));
} else {
  if (1 / (-Number.MAX_VALUE + Number.MAX_VALUE) !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: -Number.MAX_VALUE + Number.MAX_VALUE === + 0. Actual: -0');
  }
}


if (-1 / Number.MAX_VALUE + 1 / Number.MAX_VALUE !== +0) {  
  $ERROR('#3.1: -1 / Number.MAX_VALUE + 1 / Number.MAX_VALUE === 0. Actual: ' + (-1 / Number.MAX_VALUE + 1 / Number.MAX_VALUE));
} else {
  if (1 / (-1 / Number.MAX_VALUE + 1 / Number.MAX_VALUE) !== Number.POSITIVE_INFINITY) {
    $ERROR('#3.2: -1 / Number.MAX_VALUE + 1 / Number.MAX_VALUE === + 0. Actual: -0');
  }
}

