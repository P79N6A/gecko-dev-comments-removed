










var x = {};
var y = x--; 
if (isNaN(y) !== true) {
  $ERROR('#1: var x = {}; var y = x--; y === Not-a-Number. Actual: ' + (y));
}


var x = function(){return 1};
var y = x--; 
if (isNaN(y) !== true) {
  $ERROR('#2: var x = function(){return 1}; var y = x--; y === Not-a-Number. Actual: ' + (y));
}

