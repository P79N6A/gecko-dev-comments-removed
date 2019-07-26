










if ((true != Number.NaN) !== true) {
  $ERROR('#1: (true != NaN) === true');
}


if ((-1 != Number.NaN) !== true) {
  $ERROR('#2: (-1 != NaN) === true');
}


if ((Number.NaN != Number.NaN) !== true) {
  $ERROR('#3: (NaN != NaN) === true');
}


if ((Number.POSITIVE_INFINITY != Number.NaN) !== true) {
  $ERROR('#4: (+Infinity != NaN) === true');
}


if ((Number.NEGATIVE_INFINITY != Number.NaN) !== true) {
  $ERROR('#5: (-Infinity != NaN) === true');
}


if ((Number.MAX_VALUE != Number.NaN) !== true) {
  $ERROR('#6: (Number.MAX_VALUE != NaN) === true');
}


if ((Number.MIN_VALUE != Number.NaN) !== true) {
  $ERROR('#7: (Number.MIN_VALUE != NaN) === true');
}


if (("string" != Number.NaN) !== true) {
  $ERROR('#8: ("string" != NaN) === true');
}


if ((new Object() != Number.NaN) !== true) {
  $ERROR('#9: (new Object() != NaN) === true');
}

