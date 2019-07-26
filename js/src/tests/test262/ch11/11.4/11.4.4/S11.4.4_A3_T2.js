










var x = 0.1; 
++x;
if (x !== 0.1 + 1) {
  $ERROR('#1: var x = 0.1; ++x; x === 0.1 + 1. Actual: ' + (x));
}


var x = new Number(-1.1); 
++x;
if (x !== -1.1 + 1) {
  $ERROR('#2: var x = new Number(-1.1); ++x; x === -1.1 + 1. Actual: ' + (x));
}

