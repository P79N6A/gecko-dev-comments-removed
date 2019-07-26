










var x = false; 
if (void x !== undefined) {
  $ERROR('#1: var x = false; void x === undefined. Actual: ' + (void x));
}


var x = new Boolean(true);
if (void x !== undefined) {
  $ERROR('#2: var x = new Boolean(true); void x === undefined. Actual: ' + (void x));
}

