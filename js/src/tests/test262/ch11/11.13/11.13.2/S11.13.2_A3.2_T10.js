










var x = 0;
var x1 = (x ^= 1);
if (x1 !== 1) {
  $ERROR('#1: var x = 0; var x1 = (x ^= 1); x1 === 1. Actual: ' + (x1));
}


y = 1;
y1 = (y ^= 0);
if (y1 !== 1) {
  $ERROR('#2: y = 1; y1 = (y ^= 0); y1 === 1. Actual: ' + (y1));
}

