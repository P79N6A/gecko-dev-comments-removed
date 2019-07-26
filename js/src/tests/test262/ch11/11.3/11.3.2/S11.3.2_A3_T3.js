










var x = "1"; 
x--; 
if (x !== 1 - 1) {
  $ERROR('#1: var x = "1"; x--; x === 1 - 1. Actual: ' + (x));
}


var x = "x"; 
x--; 
if (isNaN(x) !== true) {
  $ERROR('#2: var x = "x"; x--; x === Not-a-Number. Actual: ' + (x));
}


var x = new Number("-1"); 
x--;
if (x !== -1 - 1) {
  $ERROR('#3: var x = new String("-1"); x--; x === -1 - 1. Actual: ' + (x));
}

