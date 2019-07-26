










if (Number.NEGATIVE_INFINITY / 0 !== Number.NEGATIVE_INFINITY) {
  $ERROR('#1: Infinity / 0 === Infinity. Actual: ' + (Infinity / 0));
}


if (Number.NEGATIVE_INFINITY / -0 !== Number.POSITIVE_INFINITY) {
  $ERROR('#2: -Infinity / -0 === Infinity. Actual: ' + (-Infinity / -0));
}


if (Number.POSITIVE_INFINITY / 0 !== Number.POSITIVE_INFINITY) {
  $ERROR('#3: Infinity / 0 === Infinity. Actual: ' + (Infinity / 0));
}


if (Number.POSITIVE_INFINITY / -0 !== Number.NEGATIVE_INFINITY) {
  $ERROR('#4: Infinity / -0 === -Infinity. Actual: ' + (Infinity / -0));
}

