










x = null;
x -= undefined;
if (isNaN(x) !== true) {
  $ERROR('#1: x = null; x -= undefined; x === Not-a-Number. Actual: ' + (x));
}


x = undefined;
x -= null;
if (isNaN(x) !== true) {
  $ERROR('#2: x = undefined; x -= null; x === Not-a-Number. Actual: ' + (x));
}


x = undefined;
x -= undefined;
if (isNaN(x) !== true) {
  $ERROR('#3: x = undefined; x -= undefined; x === Not-a-Number. Actual: ' + (x));
}


x = null;
x -= null;
if (x !== 0) {
  $ERROR('#4: x = null; x -= null; x === 0. Actual: ' + (x));
}

