










var x = "1";
var y = x++;
if (y !== 1) {
  $ERROR('#1: var x = "1"; var y = x++; y === 1. Actual: ' + (y));
}


var x = "x";
var y = x++; 
if (isNaN(y) !== true) {
  $ERROR('#2: var x = "x"; var y = x++; y === Not-a-Number. Actual: ' + (y));
}


var x = new String("-1"); 
var y = x++;
if (y !== -1) {
  $ERROR('#3: var x = new String("-1"); var y = x++; y === -1. Actual: ' + (y));
}

