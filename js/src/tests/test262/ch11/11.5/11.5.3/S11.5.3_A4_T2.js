










if (1 % 1 !== 0) {
  $ERROR('#1.1: 1 % 1 === 0. Actual: ' + (1 % 1));
} else {
  if (1 / (1 % 1) !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: 1 % 1 === + 0. Actual: -0');
  }
}


if (-1 % -1 !== -0) {
  $ERROR('#2.1: -1 % -1 === 0. Actual: ' + (-1 % -1));
} else {
  if (1 / (-1 % -1) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#2.2: -1 % -1 === - 0. Actual: +0');
  }
}


if (-1 % 1 !== -0) {
  $ERROR('#3.1: -1 % 1 === 0. Actual: ' + (-1 % 1));
} else {
  if (1 / (-1 % 1) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#3.2: -1 % 1 === - 0. Actual: +0');
  }
}


if (1 % -1 !== 0) {
  $ERROR('#4.1: 1 % -1 === 0. Actual: ' + (1 % -1));
} else {
  if (1 / (1 % -1) !== Number.POSITIVE_INFINITY) {
    $ERROR('#4.2: 1 % -1 === + 0. Actual: -0');
  }
}


if (101 % 51 !== 50) {
  $ERROR('#5: 101 % 51 === 50. Actual: ' + (101 % 51));
}


if (101 % -51 !== 50) {
  $ERROR('#6: 101 % -51 === 50. Actual: ' + (101 % -51));
}


if (-101 % 51 !== -50) {
  $ERROR('#7: -101 % 51 === -50. Actual: ' + (-101 % 51));
}


if (-101 % -51 !== -50) {
  $ERROR('#8: -101 % -51 === -50. Actual: ' + (-101 % -51));
}

