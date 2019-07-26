










var x = false; 
x++;
if (x !== 0 + 1) {
  $ERROR('#1: var x = false; x++; x === 0 + 1. Actual: ' + (x));
}


var x = new Boolean(true); 
x++; 
if (x !== 1 + 1) {
  $ERROR('#2: var x = new Boolean(true); x++; x === 1 + 1. Actual: ' + (x));
}

