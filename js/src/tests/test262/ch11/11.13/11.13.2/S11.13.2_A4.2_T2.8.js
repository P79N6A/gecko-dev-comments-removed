










x = true;
x /= undefined;
if (isNaN(x) !== true) {
  $ERROR('#1: x = true; x /= undefined; x === Not-a-Number. Actual: ' + (x));
}


x = undefined;
x /= true;
if (isNaN(x) !== true) {
  $ERROR('#2: x = undefined; x /= true; x === Not-a-Number. Actual: ' + (x));
}


x = new Boolean(true);
x /= undefined;
if (isNaN(x) !== true) {
  $ERROR('#3: x = new Boolean(true); x /= undefined; x === Not-a-Number. Actual: ' + (x));
}


x = undefined;
x /= new Boolean(true);
if (isNaN(x) !== true) {
  $ERROR('#4: x = undefined; x /= new Boolean(true); x === Not-a-Number. Actual: ' + (x));
}

