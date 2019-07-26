










var x = 0.1;
if (void x !== undefined) {
  $ERROR('#1: var x = 0.1; void x === undefined. Actual: ' + (void x));
}


var x = new Number(-1.1);
if (void x !== undefined) {
  $ERROR('#2: var x = new Number(-1.1); void x === undefined. Actual: ' + (void x));
}

