










if ((Number.NaN != true) !== true) {
  $ERROR('#1: (NaN != true) === true');
}


if ((Number.NaN != 1) !== true) {
  $ERROR('#2: (NaN != 1) === true');
}


if ((Number.NaN != Number.NaN) !== true) {
  $ERROR('#3: (NaN != NaN) === true');
}


if ((Number.NaN != Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#4: (NaN != +Infinity) === true');
}


if ((Number.NaN != Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#5: (NaN != -Infinity) === true');
}


if ((Number.NaN != Number.MAX_VALUE) !== true) {
  $ERROR('#6: (NaN != Number.MAX_VALUE) === true');
}


if ((Number.NaN != Number.MIN_VALUE) !== true) {
  $ERROR('#7: (NaN != Number.MIN_VALUE) === true');
}


if ((Number.NaN != "string") !== true) {
  $ERROR('#8: (NaN != "string") === true');
}


if ((Number.NaN != new Object()) !== true) {
  $ERROR('#9: (NaN != new Object()) === true');
}


