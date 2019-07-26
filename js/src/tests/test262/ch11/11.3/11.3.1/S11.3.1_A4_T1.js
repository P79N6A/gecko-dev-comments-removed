










var x = false;
var y = x++;
if (y !== 0) {
  $ERROR('#1: var x = false; var y = x++; y === 0. Actual: ' + (y));
}


var x = new Boolean(true);
var y = x++;
if (y !== 1) {
  $ERROR('#2: var x = new Boolean(true); var y = x++; y === 1. Actual: ' + (y));
}

