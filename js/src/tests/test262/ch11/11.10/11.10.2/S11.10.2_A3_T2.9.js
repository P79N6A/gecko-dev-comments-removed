










if ((true ^ null) !== 1) {
  $ERROR('#1: (true ^ null) === 1. Actual: ' + ((true ^ null)));
}


if ((null ^ true) !== 1) {
  $ERROR('#2: (null ^ true) === 1. Actual: ' + ((null ^ true)));
}


if ((new Boolean(true) ^ null) !== 1) {
  $ERROR('#3: (new Boolean(true) ^ null) === 1. Actual: ' + ((new Boolean(true) ^ null)));
}


if ((null ^ new Boolean(true)) !== 1) {
  $ERROR('#4: (null ^ new Boolean(true)) === 1. Actual: ' + ((null ^ new Boolean(true))));
}

