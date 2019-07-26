










x = true;
x &= undefined;
if (x !== 0) {
  $ERROR('#1: x = true; x &= undefined; x === 0. Actual: ' + (x));
}


x = undefined;
x &= true;
if (x !== 0) {
  $ERROR('#2: x = undefined; x &= true; x === 0. Actual: ' + (x));
}


x = new Boolean(true);
x &= undefined;
if (x !== 0) {
  $ERROR('#3: x = new Boolean(true); x &= undefined; x === 0. Actual: ' + (x));
}


x = undefined;
x &= new Boolean(true);
if (x !== 0) {
  $ERROR('#4: x = undefined; x &= new Boolean(true); x === 0. Actual: ' + (x));
}

