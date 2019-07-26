










var x = 0; 
if ((x = 1) % x !== 0) {
  $ERROR('#1: var x = 0; (x = 1) % x === 0. Actual: ' + ((x = 1) % x));
}


var x = 1; 
if (x % (x = 2) !== 1) {
  $ERROR('#2: var x = 1; x % (x = 2) === 1. Actual: ' + (x % (x = 2)));
}


