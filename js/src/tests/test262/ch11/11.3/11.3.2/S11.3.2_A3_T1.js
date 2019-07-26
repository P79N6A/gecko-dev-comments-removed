










var x = true; 
x--;
if (x !== 0) {
  $ERROR('#1: var x = true; x--; x === 0. Actual: ' + (x));
}


var x = new Boolean(false); 
x--;
if (x !== 0 - 1) {
  $ERROR('#2: var x = new Boolean(false); x--; x === 0 - 1. Actual: ' + (x));
}

