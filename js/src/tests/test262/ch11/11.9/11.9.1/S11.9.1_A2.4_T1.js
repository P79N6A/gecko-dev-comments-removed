










var x = 0; 
if (((x = 1) == x) !== true) {
  $ERROR('#1: var x = 0; ((x = 1) == x) === true');
}


var x = 0; 
if ((x == (x = 1)) !== false) {
  $ERROR('#2: var x = 0; (x == (x = 1)) === false');
}


