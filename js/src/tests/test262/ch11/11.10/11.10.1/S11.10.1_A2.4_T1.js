










var x = 0; 
if (((x = 1) & x) !== 1) {
  $ERROR('#1: var x = 0; ((x = 1) & x) === 1. Actual: ' + (((x = 1) & x)));
}


var x = 0; 
if ((x & (x = 1)) !== 0) {
  $ERROR('#2: var x = 0; (x & (x = 1)) === 0. Actual: ' + ((x & (x = 1))));
}


