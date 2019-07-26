










var x = 1.1;
var y = x--;
if (y !== 1.1) {
  $ERROR('#1: var x = 1.1; var y = x--; y === 1.1. Actual: ' + (y));
}


var x = new Number(-0.1);
var y = x--;
if (y !== -0.1) {
  $ERROR('#2: var x = new Number(-0.1); var y = x--; y === -0.1. Actual: ' + (y));
}

