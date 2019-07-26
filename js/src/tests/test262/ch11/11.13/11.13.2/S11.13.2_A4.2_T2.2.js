










x = "1";
x /= 1;
if (x !== 1) {
  $ERROR('#1: x = "1"; x /= 1; x === 1. Actual: ' + (x));
}


x = 1;
x /= "1";
if (x !== 1) {
  $ERROR('#2: x = 1; x /= "1"; x === 1. Actual: ' + (x));
}


x = new String("1");
x /= 1;
if (x !== 1) {
  $ERROR('#3: x = new String("1"); x /= 1; x === 1. Actual: ' + (x));
}


x = 1;
x /= new String("1");
if (x !== 1) {
  $ERROR('#4: x = 1; x /= new String("1"); x === 1. Actual: ' + (x));
}


x = "1";
x /= new Number(1);
if (x !== 1) {
  $ERROR('#5: x = "1"; x /= new Number(1); x === 1. Actual: ' + (x));
}


x = new Number(1);
x /= "1";
if (x !== 1) {
  $ERROR('#6: x = new Number(1); x /= "1"; x === 1. Actual: ' + (x));
}


x = new String("1");
x /= new Number(1);
if (x !== 1) {
  $ERROR('#7: x = new String("1"); x /= new Number(1); x === 1. Actual: ' + (x));
}


x = new Number(1);
x /= new String("1");
if (x !== 1) {
  $ERROR('#8: x = new Number(1); x /= new String("1"); x === 1. Actual: ' + (x));
}


x = "x";
x /= 1;
if (isNaN(x) !== true) {
  $ERROR('#9: x = "x"; x /= 1; x === Not-a-Number. Actual: ' + (x));
}


x = 1;
x /= "x";
if (isNaN(x) !== true) {
  $ERROR('#10: x = 1; x /= "x"; x === Not-a-Number. Actual: ' + (x));
}

