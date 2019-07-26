










if ((true == Number.NaN) !== false) {
  $ERROR('#1: (true == NaN) === false');
}


if ((-1 == Number.NaN) !== false) {
  $ERROR('#2: (-1 == NaN) === false');
}


if ((Number.NaN == Number.NaN) !== false) {
  $ERROR('#3: (NaN == NaN) === false');
}


if ((Number.POSITIVE_INFINITY == Number.NaN) !== false) {
  $ERROR('#4: (+Infinity == NaN) === false');
}


if ((Number.NEGATIVE_INFINITY == Number.NaN) !== false) {
  $ERROR('#5: (-Infinity == NaN) === false');
}


if ((Number.MAX_VALUE == Number.NaN) !== false) {
  $ERROR('#6: (Number.MAX_VALUE == NaN) === false');
}


if ((Number.MIN_VALUE == Number.NaN) !== false) {
  $ERROR('#7: (Number.MIN_VALUE == NaN) === false');
}


if (("string" == Number.NaN) !== false) {
  $ERROR('#8: ("string" == NaN) === false');
}


if ((new Object() == Number.NaN) !== false) {
  $ERROR('#9: (new Object() == NaN) === false');
}

