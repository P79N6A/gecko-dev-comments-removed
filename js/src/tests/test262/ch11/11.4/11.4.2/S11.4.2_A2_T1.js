










if (void 0 !== undefined) {
  $ERROR('#1: void 0 === undefined. Actual: ' + (void 0));
}


var x = 0;
if (void x !== undefined) {
  $ERROR('#2: var x = 0; void x === undefined. Actual: ' + (void x));
}


var x = new Object();
if (void x !== undefined) {
  $ERROR('#3: var x = new Object(); void x === undefined. Actual: ' + (void x));
}

