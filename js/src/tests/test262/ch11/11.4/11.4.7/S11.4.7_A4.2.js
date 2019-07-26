










var x = 0; 
x = -x;
if (x !== -0) {
  $ERROR('#1.1: var x = 0; x = -x; x === 0. Actual: ' + (x));
} else {
  if (1/x !== Number.NEGATIVE_INFINITY) {
    $ERROR('#1.2: var x = 0; x = -x; x === - 0. Actual: +0');
  }
}


var x = -0; 
x = -x;
if (x !== 0) {
  $ERROR('#2.1: var x = -0; x = -x; x === 0. Actual: ' + (x));
} else {
  if (1/x !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: var x = -0; x = -x; x === + 0. Actual: -0');
  }
}


