










if (Number.MIN_VALUE * 0.1 !== 0) {
  $ERROR('#1: Number.MIN_VALUE * 0.1 === 0. Actual: ' + (Number.MIN_VALUE * 0.1));
}


if (-0.1 * Number.MIN_VALUE !== -0) {
  $ERROR('#2.1: -0.1 * Number.MIN_VALUE === -0. Actual: ' + (-0.1 * Number.MIN_VALUE));
} else {
  if (1 / (-0.1 * Number.MIN_VALUE) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#2.2: -0.1 * Number.MIN_VALUE === -0. Actual: +0');
  }
}


if (Number.MIN_VALUE * 0.5 !== 0) {
  $ERROR('#3: Number.MIN_VALUE * 0.5 === 0. Actual: ' + (Number.MIN_VALUE * 0.5));
}


if (-0.5 * Number.MIN_VALUE !== -0) {
  $ERROR('#4.1: -0.5 * Number.MIN_VALUE === -0. Actual: ' + (-0.5 * Number.MIN_VALUE));
} else {
  if (1 / (-0.5 * Number.MIN_VALUE) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#4.2: -0.5 * Number.MIN_VALUE === -0. Actual: +0');
  }
}


if (Number.MIN_VALUE * 0.51 !== Number.MIN_VALUE) {
  $ERROR('#5: Number.MIN_VALUE * 0.51 === Number.MIN_VALUE. Actual: ' + (Number.MIN_VALUE * 0.51));
}


if (-0.51 * Number.MIN_VALUE !== -Number.MIN_VALUE) {
  $ERROR('#6: -0.51 * Number.MIN_VALUE === -Number.MIN_VALUE. Actual: ' + (-0.51 * Number.MIN_VALUE));
}


if (Number.MIN_VALUE * 0.9 !== Number.MIN_VALUE) {
  $ERROR('#7: Number.MIN_VALUE * 0.9 === Number.MIN_VALUE. Actual: ' + (Number.MIN_VALUE * 0.9));
}


if (-0.9 * Number.MIN_VALUE !== -Number.MIN_VALUE) {
  $ERROR('#8: -0.9 * Number.MIN_VALUE === -Number.MIN_VALUE. Actual: ' + (-0.9 * Number.MIN_VALUE));
} 

