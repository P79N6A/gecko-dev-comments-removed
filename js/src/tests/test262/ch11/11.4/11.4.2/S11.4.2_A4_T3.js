










var x = "1";
if (void x !== undefined) {
  $ERROR('#1: var x = "1"; void x === undefined. Actual: ' + (void x));
}


var x = "x"; 
if (isNaN(void x) !== true) {
  $ERROR('#2: var x = "x"; void x === undefined. Actual: ' + (void x));
}


var x = new String("-1");
if (void x !== undefined) {
  $ERROR('#3: var x = new String("-1"); void x === undefined. Actual: ' + (void x));
}

