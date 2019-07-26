










x = true;
x += null;
if (x !== 1) {
  $ERROR('#1: x = true; x += null; x === 1. Actual: ' + (x));
}


x = null;
x += true;
if (x !== 1) {
  $ERROR('#2: x = null; x += true; x === 1. Actual: ' + (x));
}


x = new Boolean(true);
x += null;
if (x !== 1) {
  $ERROR('#3: x = new Boolean(true); x += null; x === 1. Actual: ' + (x));
}


x = null;
x += new Boolean(true);
if (x !== 1) {
  $ERROR('#4: x = null; x += new Boolean(true); x === 1. Actual: ' + (x));
}

