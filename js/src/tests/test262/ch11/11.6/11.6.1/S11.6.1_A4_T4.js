










if (Number.POSITIVE_INFINITY + 1 !== Number.POSITIVE_INFINITY ) {
  $ERROR('#1: Infinity + 1 === Infinity. Actual: ' + (Infinity + 1));
}


if (-1 + Number.POSITIVE_INFINITY !== Number.POSITIVE_INFINITY ) {
  $ERROR('#2: -1 + Infinity === Infinity. Actual: ' + (-1 + Infinity));
}


if (Number.NEGATIVE_INFINITY + 1 !== Number.NEGATIVE_INFINITY ) {
  $ERROR('#3: -Infinity + 1 === -Infinity. Actual: ' + (-Infinity + 1));
}


if (-1 + Number.NEGATIVE_INFINITY !== Number.NEGATIVE_INFINITY ) {
  $ERROR('#4: -1 + -Infinity === -Infinity. Actual: ' + (-1 + -Infinity));
}


if (Number.POSITIVE_INFINITY + Number.MAX_VALUE !== Number.POSITIVE_INFINITY ) {
  $ERROR('#5: Infinity + Number.MAX_VALUE === Infinity. Actual: ' + (Infinity + Number.MAX_VALUE));
}


if (-Number.MAX_VALUE + Number.POSITIVE_INFINITY !== Number.POSITIVE_INFINITY ) {
  $ERROR('#6: -Number.MAX_VALUE + Infinity === Infinity. Actual: ' + (-Number.MAX_VALUE + Infinity));
}


if (Number.NEGATIVE_INFINITY + Number.MAX_VALUE !== Number.NEGATIVE_INFINITY ) {
  $ERROR('#7: -Infinity + Number.MAX_VALUE === -Infinity. Actual: ' + (-Infinity + Number.MAX_VALUE));
}


if (-Number.MAX_VALUE + Number.NEGATIVE_INFINITY !== Number.NEGATIVE_INFINITY ) {
  $ERROR('#8: -Number.MAX_VALUE + -Infinity === -Infinity. Actual: ' + (-Number.MAX_VALUE + -Infinity));
}




