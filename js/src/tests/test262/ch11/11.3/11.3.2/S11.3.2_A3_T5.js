










var x = {}; 
x--; 
if (isNaN(x) !== true) {
  $ERROR('#1: var x = {}; x--; x === Not-a-Number. Actual: ' + (x));
}


var x = function(){return 1}; 
x--; 
if (isNaN(x) !== true) {
  $ERROR('#2: var x = function(){return 1}; x--; x === Not-a-Number. Actual: ' + (x));
}

