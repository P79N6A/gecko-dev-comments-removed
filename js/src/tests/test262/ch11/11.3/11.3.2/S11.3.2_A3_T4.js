










var x; 
x--;
if (isNaN(x) !== true) {
  $ERROR('#1: var x; x--; x === Not-a-Number. Actual: ' + (x));
}


var x = null; 
x--;
if (x !== -1) {
  $ERROR('#2: var x = null; x--; x === -1. Actual: ' + (x));
}

