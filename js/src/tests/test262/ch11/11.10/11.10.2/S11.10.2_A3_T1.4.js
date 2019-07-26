










if ((null ^ undefined) !== 0) {
  $ERROR('#1: (null ^ undefined) === 0. Actual: ' + ((null ^ undefined)));
}


if ((undefined ^ null) !== 0) {
  $ERROR('#2: (undefined ^ null) === 0. Actual: ' + ((undefined ^ null)));
}


if ((undefined ^ undefined) !== 0) {
  $ERROR('#3: (undefined ^ undefined) === 0. Actual: ' + ((undefined ^ undefined)));
}


if ((null ^ null) !== 0) {
  $ERROR('#4: (null ^ null) === 0. Actual: ' + ((null ^ null)));
}

