










if ((0 < Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#1: (0 < -Infinity) === false');
}


if ((1.1 < Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#2: (1.1 < -Infinity) === false');
}


if ((-1.1 < Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#3: (-1.1 < -Infinity) === false');
}


if ((Number.POSITIVE_INFINITY < Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#4: (+Infinity < -Infinity) === false');
}


if ((Number.MAX_VALUE < Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#5: (Number.MAX_VALUE < -Infinity) === false');
}


if ((Number.MIN_VALUE < Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#6: (Number.MIN_VALUE < -Infinity) === false');
}


