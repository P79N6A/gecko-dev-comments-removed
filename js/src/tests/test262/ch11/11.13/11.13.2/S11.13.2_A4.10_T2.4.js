










x = 1;
x ^= undefined;
if (x !== 1) {
  $ERROR('#1: x = 1; x ^= undefined; x === 1. Actual: ' + (x));
}


x = undefined;
x ^= 1;
if (x !== 1) {
  $ERROR('#2: x = undefined; x ^= 1; x === 1. Actual: ' + (x));
}


x = new Number(1);
x ^= undefined;
if (x !== 1) {
  $ERROR('#3: x = new Number(1); x ^= undefined; x === 1. Actual: ' + (x));
}


x = undefined;
x ^= new Number(1);
if (x !== 1) {
  $ERROR('#4: x = undefined; x ^= new Number(1); x === 1. Actual: ' + (x));
}

