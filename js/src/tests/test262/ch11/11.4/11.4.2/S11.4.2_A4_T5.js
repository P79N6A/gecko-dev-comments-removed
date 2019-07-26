










var x = {}; 
if (isNaN(void x) !== true) {
  $ERROR('#1: var x = {}; void x === undefined. Actual: ' + (void x));
}


var x = function(){return 1}; 
if (isNaN(void x) !== true) {
  $ERROR('#2: var x = function(){return 1}; void x === undefined. Actual: ' + (void x));
}

