










var x = -1;
x %= 2; 
if (x !== -1) {
  $ERROR('#1: var x = -1; x %= 2; x === -1. Actual: ' + (x));
}


y = -1;
y %= 2;
if (y !== -1) {
  $ERROR('#2: y = -1; y %= 2; y === -1. Actual: ' + (y));
}

