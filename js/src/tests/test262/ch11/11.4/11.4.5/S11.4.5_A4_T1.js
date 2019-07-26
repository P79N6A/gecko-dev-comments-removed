










var x = true;
if (--x !== 1 - 1) {
  $ERROR('#1: var x = true; --x === 1 - 1. Actual: ' + (--x));
}


var x = new Boolean(false);
if (--x !== 0 - 1) {
  $ERROR('#2: var x = new Boolean(false); --x === 0 - 1. Actual: ' + (--x));
}

