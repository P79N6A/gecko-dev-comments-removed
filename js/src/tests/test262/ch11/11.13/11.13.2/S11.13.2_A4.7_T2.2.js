










x = "1";
x >>= 1;
if (x !== 0) {
  $ERROR('#1: x = "1"; x >>= 1; x === 0. Actual: ' + (x));
}


x = 1;
x >>= "1";
if (x !== 0) {
  $ERROR('#2: x = 1; x >>= "1"; x === 0. Actual: ' + (x));
}


x = new String("1");
x >>= 1;
if (x !== 0) {
  $ERROR('#3: x = new String("1"); x >>= 1; x === 0. Actual: ' + (x));
}


x = 1;
x >>= new String("1");
if (x !== 0) {
  $ERROR('#4: x = 1; x >>= new String("1"); x === 0. Actual: ' + (x));
}


x = "1";
x >>= new Number(1);
if (x !== 0) {
  $ERROR('#5: x = "1"; x >>= new Number(1); x === 0. Actual: ' + (x));
}


x = new Number(1);
x >>= "1";
if (x !== 0) {
  $ERROR('#6: x = new Number(1); x >>= "1"; x === 0. Actual: ' + (x));
}


x = new String("1");
x >>= new Number(1);
if (x !== 0) {
  $ERROR('#7: x = new String("1"); x >>= new Number(1); x === 0. Actual: ' + (x));
}


x = new Number(1);
x >>= new String("1");
if (x !== 0) {
  $ERROR('#8: x = new Number(1); x >>= new String("1"); x === 0. Actual: ' + (x));
}


x = "x";
x >>= 1;
if (x !== 0) {
  $ERROR('#9: x = "x"; x >>= 1; x === 0. Actual: ' + (x));
}


x = 1;
x >>= "x";
if (x !== 1) {
  $ERROR('#10: x = 1; x >>= "x"; x === 1. Actual: ' + (x));
}

