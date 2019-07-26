










var x = 0; 
if ((x = 1) <= x !== true) {
  $ERROR('#1: var x = 0; (x = 1) <= x === true');
}


var x = 1; 
if (x <= (x = 0) !== false) {
  $ERROR('#2: var x = 1; x <= (x = 0) === false');
}


