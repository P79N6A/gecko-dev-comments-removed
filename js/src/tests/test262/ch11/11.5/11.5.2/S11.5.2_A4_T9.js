










if (Number.MAX_VALUE / 0.9 !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: Number.MAX_VALUE / 0.9 === Number.POSITIVE_INFINITY. Actual: ' + (Number.MAX_VALUE / 0.9));
}


if (Number.MAX_VALUE / -0.9 !== Number.NEGATIVE_INFINITY) {
  $ERROR('#2: Number.MAX_VALUE / -0.9 === Number.NEGATIVE_INFINITY. Actual: ' + (Number.MAX_VALUE / -0.9));
} 


if (Number.MAX_VALUE / 1 !== Number.MAX_VALUE) {
  $ERROR('#3: Number.MAX_VALUE / 1 === Number.MAX_VALUE. Actual: ' + (Number.MAX_VALUE / 1));
}


if (Number.MAX_VALUE / -1 !== -Number.MAX_VALUE) {
  $ERROR('#4: Number.MAX_VALUE / -1 === -Number.MAX_VALUE. Actual: ' + (Number.MAX_VALUE / -1));
} 


if (Number.MAX_VALUE / (Number.MAX_VALUE / 0.9) === (Number.MAX_VALUE / Number.MAX_VALUE) / 0.9) {
  $ERROR('#5: Number.MAX_VALUE / (Number.MAX_VALUE / 0.9) !== (Number.MAX_VALUE / Number.MAX_VALUE) / 0.9');
}

