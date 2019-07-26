










x = true;
x += 1;
if (x !== 2) {
  $ERROR('#1: x = true; x += 1; x === 2. Actual: ' + (x));
}


x = 1;
x += true;
if (x !== 2) {
  $ERROR('#2: x = 1; x += true; x === 2. Actual: ' + (x));
}


x = new Boolean(true);
x += 1;
if (x !== 2) {
  $ERROR('#3: x = new Boolean(true); x += 1; x === 2. Actual: ' + (x));
}


x = 1;
x += new Boolean(true);
if (x !== 2) {
  $ERROR('#4: x = 1; x += new Boolean(true); x === 2. Actual: ' + (x));
}


x = true;
x += new Number(1);
if (x !== 2) {
  $ERROR('#5: x = true; x += new Number(1); x === 2. Actual: ' + (x));
}


x = new Number(1);
x += true;
if (x !== 2) {
  $ERROR('#6: x = new Number(1); x += true; x === 2. Actual: ' + (x));
}


x = new Boolean(true);
x += new Number(1);
if (x !== 2) {
  $ERROR('#7: x = new Boolean(true); x += new Number(1); x === 2. Actual: ' + (x));
}


x = new Number(1);
x += new Boolean(true);
if (x !== 2) {
  $ERROR('#8: x = new Number(1); x += new Boolean(true); x === 2. Actual: ' + (x));
}

