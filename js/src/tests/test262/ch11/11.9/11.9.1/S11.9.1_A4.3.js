











if ((Number.POSITIVE_INFINITY == Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#1: (+Infinity == +Infinity) === true');
}


if ((Number.NEGATIVE_INFINITY == Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#2: (-Infinity == -Infinity) === true');
}


if ((Number.POSITIVE_INFINITY == -Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#3: (+Infinity == -(-Infinity)) === true');
}


if ((1 == 0.999999999999) !== false) {
  $ERROR('#4: (1 == 0.999999999999) === false');
}


if ((1.0 == 1) !== true) {
  $ERROR('#5: (1.0 == 1) === true');
}

