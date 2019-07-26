










var x = 1; 
if (((x = 0) ^ x) !== 0) {
  $ERROR('#1: var x = 0; ((x = 1) ^ x) === 0. Actual: ' + (((x = 1) ^ x)));
}


var x = 0; 
if ((x ^ (x = 1)) !== 1) {
  $ERROR('#2: var x = 0; (x ^ (x = 1)) === 1. Actual: ' + ((x ^ (x = 1))));
}



