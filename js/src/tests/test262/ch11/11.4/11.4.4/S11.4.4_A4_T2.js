










var x = 0.1;
if (++x !== 0.1 + 1) {
  $ERROR('#1: var x = 0.1; ++x === 0.1 + 1. Actual: ' + (++x));
}


var x = new Number(-1.1);
if (++x !== -1.1 + 1) {
  $ERROR('#2: var x = new Number(-1.1); ++x === -1.1 + 1. Actual: ' + (++x));
}

