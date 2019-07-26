










if ((-0 && -1) !== 0) {
  $ERROR('#1.1: (-0 && -1) === 0');
} else {
  if ((1 / (-0 && -1)) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#1.2: (-0 && -1) === -0');
  }
}


if ((0 && new Number(-1)) !== 0) {
  $ERROR('#2.1: (0 && new Number(-1)) === 0');
} else {
  if ((1 / (0 && new Number(-1))) !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: (0 && new Number(-1)) === +0');
  }
}


if ((isNaN(NaN && 1)) !== true) {
  $ERROR('#3: (NaN && 1) === Not-a-Number');
}

