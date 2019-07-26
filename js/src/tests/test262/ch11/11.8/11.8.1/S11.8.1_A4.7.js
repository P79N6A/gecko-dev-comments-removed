










if ((Number.NEGATIVE_INFINITY < 0) !== true) {
  $ERROR('#1: (-Infinity < 0) === true');
}


if ((Number.NEGATIVE_INFINITY < 1.1) !== true) {
  $ERROR('#2: (-Infinity < 1.1) === true');
}


if ((Number.NEGATIVE_INFINITY < -1.1) !== true) {
  $ERROR('#3: (-Infinity < -1.1) === true');
}


if ((Number.NEGATIVE_INFINITY < Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#4: (-Infinity < +Infinity) === true');
}


if ((Number.NEGATIVE_INFINITY < Number.MAX_VALUE) !== true) {
  $ERROR('#5: (-Infinity < Number.MAX_VALUE) === true');
}


if ((Number.NEGATIVE_INFINITY < Number.MIN_VALUE) !== true) {
  $ERROR('#6: (-Infinity < Number.MIN_VALUE) === true');
}


