










var x = 1; 
if ((x = 0) >= x !== true) {
  $ERROR('#1: var x = 1; (x = 0) >= x === true');
}


var x = 0; 
if (x >= (x = 1) !== false) {
  $ERROR('#2: var x = 0; x >= (x = 1) === false');
}


