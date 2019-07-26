










x = true;
x /= true;
if (x !== 1) {
  $ERROR('#1: x = true; x /= true; x === 1. Actual: ' + (x));
}


x = new Boolean(true);
x /= true;
if (x !== 1) {
  $ERROR('#2: x = new Boolean(true); x /= true; x === 1. Actual: ' + (x));
}


x = true;
x /= new Boolean(true);
if (x !== 1) {
  $ERROR('#3: x = true; x /= new Boolean(true); x === 1. Actual: ' + (x));
}


x = new Boolean(true);
x /= new Boolean(true);
if (x !== 1) {
  $ERROR('#4: x = new Boolean(true); x /= new Boolean(true); x === 1. Actual: ' + (x));
}

