










if ((Number.NaN >= 0) !== false) {
  $ERROR('#1: (NaN >= 0) === false');
}


if ((Number.NaN >= 1.1) !== false) {
  $ERROR('#2: (NaN >= 1.1) === false');
}


if ((Number.NaN >= -1.1) !== false) {
  $ERROR('#3: (NaN >= -1.1) === false');
}


if ((Number.NaN >= Number.NaN) !== false) {
  $ERROR('#4: (NaN >= NaN) === false');
}


if ((Number.NaN >= Number.POSITIVE_INFINITY) !== false) {
  $ERROR('#5: (NaN >= +Infinity) === false');
}


if ((Number.NaN >= Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#6: (NaN >= -Infinity) === false');
}


if ((Number.NaN >= Number.MAX_VALUE) !== false) {
  $ERROR('#7: (NaN >= Number.MAX_VALUE) === false');
}


if ((Number.NaN >= Number.MIN_VALUE) !== false) {
  $ERROR('#8: (NaN >= Number.MIN_VALUE) === false');
}


