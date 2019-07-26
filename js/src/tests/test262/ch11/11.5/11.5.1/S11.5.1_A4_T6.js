










if (Number.MAX_VALUE * 1.1 !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: Number.MAX_VALUE * 1.1 === Number.POSITIVE_INFINITY. Actual: ' + (Number.MAX_VALUE * 1.1));
}


if (-1.1 * Number.MAX_VALUE !== Number.NEGATIVE_INFINITY) {
  $ERROR('#2: -1.1 * Number.MAX_VALUE === Number.NEGATIVE_INFINITY. Actual: ' + (-1.1 * Number.MAX_VALUE));
} 


if (Number.MAX_VALUE * 1 !== Number.MAX_VALUE) {
  $ERROR('#3: Number.MAX_VALUE * 1 === Number.MAX_VALUE. Actual: ' + (Number.MAX_VALUE * 1));
}


if (-1 * Number.MAX_VALUE !== -Number.MAX_VALUE) {
  $ERROR('#4: -1 * Number.MAX_VALUE === -Number.MAX_VALUE. Actual: ' + (-1 * Number.MAX_VALUE));
} 

