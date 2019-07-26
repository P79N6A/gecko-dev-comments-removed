










x = "1";
x >>= undefined;
if (x !== 1) {
  $ERROR('#1: x = "1"; x >>= undefined; x === 1. Actual: ' + (x));
}


x = undefined;
x >>= "1";
if (x !== 0) {
  $ERROR('#2: x = undefined; x >>= "1"; x === 0. Actual: ' + (x));
}


x = new String("1");
x >>= undefined;
if (x !== 1) {
  $ERROR('#3: x = new String("1"); x >>= undefined; x === 1. Actual: ' + (x));
}


x = undefined;
x >>= new String("1");
if (x !== 0) {
  $ERROR('#4: x = undefined; x >>= new String("1"); x === 0. Actual: ' + (x));
}

