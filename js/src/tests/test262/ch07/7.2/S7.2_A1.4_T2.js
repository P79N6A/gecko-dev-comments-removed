










eval("\u0020var x\u0020= 1\u0020");
if (x !== 1) {
  $ERROR('#1: eval("\\u0020var x\\u0020= 1\\u0020"); x === 1. Actual: ' + (x));
}


 var x = 1 ;
if (x !== 1) {
  $ERROR('#2:  var x = 1 ; x === 1. Actual: ' + (x));
}


