










if ((0 >= Number.NaN) !== false) {
  $ERROR('#1: (0 >= NaN) === false');
}


if ((1.1 >= Number.NaN) !== false) {
  $ERROR('#2: (1.1 >= NaN) === false');
}


if ((-1.1 >= Number.NaN) !== false) {
  $ERROR('#3: (-1.1 >= NaN) === false');
}


if ((Number.NaN >= Number.NaN) !== false) {
  $ERROR('#4: (NaN >= NaN) === false');
}


if ((Number.POSITIVE_INFINITY >= Number.NaN) !== false) {
  $ERROR('#5: (+Infinity >= NaN) === false');
}


if ((Number.NEGATIVE_INFINITY >= Number.NaN) !== false) {
  $ERROR('#6: (-Infinity >= NaN) === false');
}


if ((Number.MAX_VALUE >= Number.NaN) !== false) {
  $ERROR('#7: (Number.MAX_VALUE >= NaN) === false');
}


if ((Number.MIN_VALUE >= Number.NaN) !== false) {
  $ERROR('#8: (Number.MIN_VALUE >= NaN) === false');
}


