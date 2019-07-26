










if ((1 > 1) !== false) {
  $ERROR('#1: (1 > 1) === false');
}


if ((1.1 > 1.1) !== false) {
  $ERROR('#2: (1.1 > 1.1) === false');
}


if ((-1.1 > -1.1) !== false) {
  $ERROR('#3: (-1.1 > -1.1) === false');
}


if ((Number.NEGATIVE_INFINITY > Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#4: (-Infinity > -Infinity) === false');
}


if ((Number.POSITIVE_INFINITY > Number.POSITIVE_INFINITY) !== false) {
  $ERROR('#5: (+Infinity > +Infinity) === false');
}


if ((Number.MAX_VALUE > Number.MAX_VALUE) !== false) {
  $ERROR('#6: (Number.MAX_VALUE > Number.MAX_VALUE) === false');
}


if ((Number.MIN_VALUE > Number.MIN_VALUE) !== false) {
  $ERROR('#7: (Number.MIN_VALUE > Number.MIN_VALUE) === false');
}



