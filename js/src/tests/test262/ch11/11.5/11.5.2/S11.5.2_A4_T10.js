











if (Number.MIN_VALUE / 2.1 !== 0) {
  $ERROR('#1: Number.MIN_VALUE / 2.1 === 0. Actual: ' + (Number.MIN_VALUE / 2.1));
}


if (Number.MIN_VALUE / -2.1 !== -0) {
  $ERROR('#2.1: Number.MIN_VALUE / -2.1 === 0. Actual: ' + (Number.MIN_VALUE / -2.1));
} else {
  if (1 / (Number.MIN_VALUE / -2.1) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#2.2: Number.MIN_VALUE / -2.1 === -0. Actual: +0');
  }
}


if (Number.MIN_VALUE / 2.0 !== 0) {
  $ERROR('#3: Number.MIN_VALUE / 2.0 === 0. Actual: ' + (Number.MIN_VALUE / 2.0));
}


if (Number.MIN_VALUE / -2.0 !== -0) {
  $ERROR('#4.1: Number.MIN_VALUE / -2.0 === -0. Actual: ' + (Number.MIN_VALUE / -2.0));
} else {
  if (1 / (Number.MIN_VALUE / -2.0) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#4.2: Number.MIN_VALUE / -2.0 === -0. Actual: +0');
  }
}


if (Number.MIN_VALUE / 1.9 !== Number.MIN_VALUE) {
  $ERROR('#5: Number.MIN_VALUE / 1.9 === Number.MIN_VALUE. Actual: ' + (Number.MIN_VALUE / 1.9));
}


if (Number.MIN_VALUE / -1.9 !== -Number.MIN_VALUE) {
  $ERROR('#6: Number.MIN_VALUE / -1.9 === -Number.MIN_VALUE. Actual: ' + (Number.MIN_VALUE / -1.9));
}


if (Number.MIN_VALUE / 1.1 !== Number.MIN_VALUE) {
  $ERROR('#7: Number.MIN_VALUE / 1.1 === Number.MIN_VALUE. Actual: ' + (Number.MIN_VALUE / 1.1));
}


if (Number.MIN_VALUE / -1.1 !== -Number.MIN_VALUE) {
  $ERROR('#8: Number.MIN_VALUE / -1.1 === -Number.MIN_VALUE. Actual: ' + (Number.MIN_VALUE / -1.1));
} 

