










x = true;
x /= "1";
if (x !== 1) {
  $ERROR('#1: x = true; x /= "1"; x === 1. Actual: ' + (x));
}


x = "1";
x /= true;
if (x !== 1) {
  $ERROR('#2: x = "1"; x /= true; x === 1. Actual: ' + (x));
}


x = new Boolean(true);
x /= "1";
if (x !== 1) {
  $ERROR('#3: x = new Boolean(true); x /= "1"; x === 1. Actual: ' + (x));
}


x = "1";
x /= new Boolean(true);
if (x !== 1) {
  $ERROR('#4: x = "1"; x /= new Boolean(true); x === 1. Actual: ' + (x));
}


x = true;
x /= new String("1");
if (x !== 1) {
  $ERROR('#5: x = true; x /= new String("1"); x === 1. Actual: ' + (x));
}


x = new String("1");
x /= true;
if (x !== 1) {
  $ERROR('#6: x = new String("1"); x /= true; x === 1. Actual: ' + (x));
}


x = new Boolean(true);
x /= new String("1");
if (x !== 1) {
  $ERROR('#7: x = new Boolean(true); x /= new String("1"); x === 1. Actual: ' + (x));
}


x = new String("1");
x /= new Boolean(true);
if (x !== 1) {
  $ERROR('#8: x = new String("1"); x /= new Boolean(true); x === 1. Actual: ' + (x));
}

