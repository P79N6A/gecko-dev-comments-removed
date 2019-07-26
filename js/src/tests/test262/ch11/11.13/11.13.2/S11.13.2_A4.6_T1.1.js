










x = true;
x <<= true;
if (x !== 2) {
  $ERROR('#1: x = true; x <<= true; x === 2. Actual: ' + (x));
}


x = new Boolean(true);
x <<= true;
if (x !== 2) {
  $ERROR('#2: x = new Boolean(true); x <<= true; x === 2. Actual: ' + (x));
}


x = true;
x <<= new Boolean(true);
if (x !== 2) {
  $ERROR('#3: x = true; x <<= new Boolean(true); x === 2. Actual: ' + (x));
}


x = new Boolean(true);
x <<= new Boolean(true);
if (x !== 2) {
  $ERROR('#4: x = new Boolean(true); x <<= new Boolean(true); x === 2. Actual: ' + (x));
}

