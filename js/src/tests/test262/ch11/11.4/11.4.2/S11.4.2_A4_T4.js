










var x; 
if (isNaN(void x) !== true) {
  $ERROR('#1: var x; void x === undefined. Actual: ' + (void x));
}


var x = null;
if (void x !== undefined) {
  $ERROR('#2: var x = null; void x === undefined. Actual: ' + (void x));
}

