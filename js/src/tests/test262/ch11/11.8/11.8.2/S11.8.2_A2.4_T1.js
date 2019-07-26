










var x = 0; 
if ((x = 1) > x !== false) {
  $ERROR('#1: var x = 0; (x = 1) > x === false');
}


var x = 1; 
if (x > (x = 0) !== true) {
  $ERROR('#2: var x = 1; x > (x = 0) === true');
}


