










if ((true & null) !== 0) {
  $ERROR('#1: (true & null) === 0. Actual: ' + ((true & null)));
}


if ((null & true) !== 0) {
  $ERROR('#2: (null & true) === 0. Actual: ' + ((null & true)));
}


if ((new Boolean(true) & null) !== 0) {
  $ERROR('#3: (new Boolean(true) & null) === 0. Actual: ' + ((new Boolean(true) & null)));
}


if ((null & new Boolean(true)) !== 0) {
  $ERROR('#4: (null & new Boolean(true)) === 0. Actual: ' + ((null & new Boolean(true))));
}

