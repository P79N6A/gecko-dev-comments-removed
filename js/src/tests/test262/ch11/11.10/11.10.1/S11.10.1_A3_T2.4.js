










if ((1 & undefined) !== 0) {
  $ERROR('#1: (1 & undefined) === 0. Actual: ' + ((1 & undefined)));
}


if ((undefined & 1) !== 0) {
  $ERROR('#2: (undefined & 1) === 0. Actual: ' + ((undefined & 1)));
}


if ((new Number(1) & undefined) !== 0) {
  $ERROR('#3: (new Number(1) & undefined) === 0. Actual: ' + ((new Number(1) & undefined)));
}


if ((undefined & new Number(1)) !== 0) {
  $ERROR('#4: (undefined & new Number(1)) === 0. Actual: ' + ((undefined & new Number(1))));
}

