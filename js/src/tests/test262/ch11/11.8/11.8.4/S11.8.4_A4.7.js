










if ((Number.NEGATIVE_INFINITY >= 0) !== false) {
  $ERROR('#1: (-Infinity >= 0) === false');
}


if ((Number.NEGATIVE_INFINITY >= 1.1) !== false) {
  $ERROR('#2: (-Infinity >= 1.1) === false');
}


if ((Number.NEGATIVE_INFINITY >= -1.1) !== false) {
  $ERROR('#3: (-Infinity >= -1.1) === false');
}


if ((Number.NEGATIVE_INFINITY >= Number.POSITIVE_INFINITY) !== false) {
  $ERROR('#4: (-Infinity >= +Infinity) === false');
}


if ((Number.NEGATIVE_INFINITY >= Number.MAX_VALUE) !== false) {
  $ERROR('#5: (-Infinity >= Number.MAX_VALUE) === false');
}


if ((Number.NEGATIVE_INFINITY >= Number.MIN_VALUE) !== false) {
  $ERROR('#6: (-Infinity >= Number.MIN_VALUE) === false');
}


