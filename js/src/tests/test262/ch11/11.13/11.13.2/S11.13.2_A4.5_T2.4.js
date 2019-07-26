










x = 1;
x -= undefined;
if (isNaN(x) !== true) {
  $ERROR('#1: x = 1; x -= undefined; x === Not-a-Number. Actual: ' + (x));
}


x = undefined;
x -= 1;
if (isNaN(x) !== true) {
  $ERROR('#2: x = undefined; x -= 1; x === Not-a-Number. Actual: ' + (x));
}


x = new Number(1);
x -= undefined;
if (isNaN(x) !== true) {
  $ERROR('#3: x = new Number(1); x -= undefined; x === Not-a-Number. Actual: ' + (x));
}


x = undefined;
x -= new Number(1);
if (isNaN(x) !== true) {
  $ERROR('#4: x = undefined; x -= new Number(1); x === Not-a-Number. Actual: ' + (x));
}

