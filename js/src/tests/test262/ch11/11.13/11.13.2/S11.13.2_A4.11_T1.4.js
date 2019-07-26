










x = null;
x |= undefined;
if (x !== 0) {
  $ERROR('#1: x = null; x |= undefined; x === 0. Actual: ' + (x));
}


x = undefined;
x |= null;
if (x !== 0) {
  $ERROR('#2: x = undefined; x |= null; x === 0. Actual: ' + (x));
}


x = undefined;
x |= undefined;
if (x !== 0) {
  $ERROR('#3: x = undefined; x |= undefined; x === 0. Actual: ' + (x));
}


x = null;
x |= null;
if (x !== 0) {
  $ERROR('#4: x = null; x |= null; x === 0. Actual: ' + (x));
}

