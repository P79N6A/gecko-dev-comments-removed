










var x = 1; 
if (((x = 0) | x) !== 0) {
  $ERROR('#1: var x = 1; ((x = 0) | x) === 0. Actual: ' + (((x = 0) | x)));
}


var x = 1; 
if ((x | (x = 0)) !== 1) {
  $ERROR('#2: var x = 1; (x | (x = 0)) === 1. Actual: ' + ((x | (x = 0))));
}

