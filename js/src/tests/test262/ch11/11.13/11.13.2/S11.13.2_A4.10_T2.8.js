










x = true;
x ^= undefined;
if (x !== 1) {
  $ERROR('#1: x = true; x ^= undefined; x === 1. Actual: ' + (x));
}


x = undefined;
x ^= true;
if (x !== 1) {
  $ERROR('#2: x = undefined; x ^= true; x === 1. Actual: ' + (x));
}


x = new Boolean(true);
x ^= undefined;
if (x !== 1) {
  $ERROR('#3: x = new Boolean(true); x ^= undefined; x === 1. Actual: ' + (x));
}


x = undefined;
x ^= new Boolean(true);
if (x !== 1) {
  $ERROR('#4: x = undefined; x ^= new Boolean(true); x === 1. Actual: ' + (x));
}

