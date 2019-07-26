










if (1 * 1 !== 1) {
  $ERROR('#1: 1 * 1 === 1. Actual: ' + (1 * 1));
}


if (1 * -1 !== -1) {
  $ERROR('#2: 1 * -1 === -1. Actual: ' + (1 * -1));
}


if (-1 * 1 !== -1) {
  $ERROR('#3: -1 * 1 === -1. Actual: ' + (-1 * 1));
}


if (-1 * -1 !== 1) {
  $ERROR('#4: -1 * -1 === 1. Actual: ' + (-1 * -1));
}


if (0 * 0 !== 0) {
  $ERROR('#5.1: 0 * 0 === 0. Actual: ' + (0 * 0));
} else {
  if (1 / (0 * 0) !== Number.POSITIVE_INFINITY) {
    $ERROR('#5.2: 0 * 0 === + 0. Actual: -0');
  }
}


if (0 * -0 !== -0) {
  $ERROR('#6.1: 0 * -0 === 0. Actual: ' + (0 * -0));
} else {
  if (1 / (0 * -0) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#6.2: 0 * -0 === - 0. Actual: +0');
  }
}


if (-0 * 0 !== -0) {
  $ERROR('#7.1: -0 * 0 === 0. Actual: ' + (-0 * 0));
} else {
  if (1 / (-0 * 0) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#7.2: -0 * 0 === - 0. Actual: +0');
  }
}


if (-0 * -0 !== 0) {
  $ERROR('#8.1: -0 * -0 === 0. Actual: ' + (-0 * -0));
} else {
  if (1 / (-0 * -0) !== Number.POSITIVE_INFINITY) {
    $ERROR('#8.2: 0 * -0 === - 0. Actual: +0');
  }
}

