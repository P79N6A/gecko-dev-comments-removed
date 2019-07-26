











if ((Number.POSITIVE_INFINITY != Number.POSITIVE_INFINITY) !== false) {
  $ERROR('#1: (+Infinity != +Infinity) === false');
}


if ((Number.NEGATIVE_INFINITY != Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#2: (-Infinity != -Infinity) === false');
}


if ((Number.POSITIVE_INFINITY != -Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#3: (+Infinity != -(-Infinity)) === false');
}


if ((1 != 0.999999999999) !== true) {
  $ERROR('#4: (1 != 0.999999999999) === true');
}


if ((1.0 != 1) !== false) {
  $ERROR('#5: (1.0 != 1) === false');
}

