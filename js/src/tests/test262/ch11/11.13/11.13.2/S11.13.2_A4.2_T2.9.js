










x = true;
x /= null;
if (x !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: x = true; x /= null; x === +Infinity. Actual: ' + (x));
}


x = null;
x /= true;
if (x !== 0) {
  $ERROR('#2: x = null; x /= true; x === 0. Actual: ' + (x));
}


x = new Boolean(true);
x /= null;
if (x !== Number.POSITIVE_INFINITY) {
  $ERROR('#3: x = new Boolean(true); x /= null; x === +Infinity. Actual: ' + (x));
}


x = null;
x /= new Boolean(true);
if (x !== 0) {
  $ERROR('#4: x = null; x /= new Boolean(true); x === 0. Actual: ' + (x));
}

