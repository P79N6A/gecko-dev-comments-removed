










if (Number.MAX_VALUE + Number.MAX_VALUE !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: Number.MAX_VALUE + Number.MAX_VALUE === Number.POSITIVE_INFINITY. Actual: ' + (Number.MAX_VALUE + Number.MAX_VALUE));
}


if (-Number.MAX_VALUE - Number.MAX_VALUE !== Number.NEGATIVE_INFINITY) {
  $ERROR('#2: -Number.MAX_VALUE - Number.MAX_VALUE === Number.NEGATIVE_INFINITY. Actual: ' + (-Number.MAX_VALUE - Number.MAX_VALUE));
}


if (1e+308 + 1e+308 !== Number.POSITIVE_INFINITY) {
  $ERROR('#3: 1e+308 + 1e+308 === Number.POSITIVE_INFINITY. Actual: ' + (1e+308 + 1e+308));
}


if (-8.99e+307 - 8.99e+307 !== Number.NEGATIVE_INFINITY) {
  $ERROR('#4: -8.99e+307 - 8.99e+307 === Number.NEGATIVE_INFINITY. Actual: ' + (-8.99e+307 - 8.99e+307));
}

