











if (Number("Infinity") !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: Number("Infinity") === Number.POSITIVE_INFINITY');
}


if (Number("Infinity") !== 10e10000) {
  $ERROR('#2: Number("Infinity") === 10e10000');
}


if (Number("Infinity") !== 10E10000) {
  $ERROR('#3: Number("Infinity") === 10E10000');
}


if (Number("Infinity") !== Number("10e10000")) {
  $ERROR('#4: Number("Infinity") === Number("10e10000")');
}

