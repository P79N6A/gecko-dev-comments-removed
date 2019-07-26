










x = 1;
x += 1;
if (x !== 2) {
  $ERROR('#1: x = 1; x += 1; x === 2. Actual: ' + (x));
}


x = new Number(1);
x += 1;
if (x !== 2) {
  $ERROR('#2: x = new Number(1); x += 1; x === 2. Actual: ' + (x));
}


x = 1;
x += new Number(1);
if (x !== 2) {
  $ERROR('#3: x = 1; x += new Number(1); x === 2. Actual: ' + (x));
}


x = new Number(1);
x += new Number(1);
if (x !== 2) {
  $ERROR('#4: x = new Number(1); x += new Number(1); x === 2. Actual: ' + (x));
}


