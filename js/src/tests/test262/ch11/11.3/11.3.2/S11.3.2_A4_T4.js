










var x; 
var y = x--;
if (isNaN(y) !== true) {
  $ERROR('#1: var x; var y = x--; y === Not-a-Number. Actual: ' + (y));
}


var x = null;
var y = x--;
if (y !== 0) {
  $ERROR('#2: var x = null; var y = x--; y === 0. Actual: ' + (y));
}

