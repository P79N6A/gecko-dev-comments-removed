










x = true;
x %= true;
if (x !== 0) {
  $ERROR('#1: x = true; x %= true; x === 0. Actual: ' + (x));
}


x = new Boolean(true);
x %= true;
if (x !== 0) {
  $ERROR('#2: x = new Boolean(true); x %= true; x === 0. Actual: ' + (x));
}


x = true;
x %= new Boolean(true);
if (x !== 0) {
  $ERROR('#3: x = true; x %= new Boolean(true); x === 0. Actual: ' + (x));
}


x = new Boolean(true);
x %= new Boolean(true);
if (x !== 0) {
  $ERROR('#4: x = new Boolean(true); x %= new Boolean(true); x === 0. Actual: ' + (x));
}

