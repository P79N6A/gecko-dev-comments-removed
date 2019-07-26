










if (Number.NEGATIVE_INFINITY * Number.NEGATIVE_INFINITY !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: -Infinity * -Infinity === Infinity. Actual: ' + (-Infinity * -Infinity));
}


if (Number.POSITIVE_INFINITY * Number.POSITIVE_INFINITY !== Number.POSITIVE_INFINITY) {
  $ERROR('#2: Infinity * Infinity === Infinity. Actual: ' + (Infinity * Infinity));
}


if (Number.NEGATIVE_INFINITY * Number.POSITIVE_INFINITY !== Number.NEGATIVE_INFINITY) {
  $ERROR('#3: -Infinity * Infinity === -Infinity. Actual: ' + (-Infinity * Infinity));
}


if (Number.POSITIVE_INFINITY * Number.NEGATIVE_INFINITY !== Number.NEGATIVE_INFINITY) {
  $ERROR('#4: Infinity * -Infinity === -Infinity. Actual: ' + (Infinity * -Infinity));
}

