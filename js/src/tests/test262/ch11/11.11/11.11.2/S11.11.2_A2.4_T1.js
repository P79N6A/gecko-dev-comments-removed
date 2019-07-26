










var x = true; 
if (((x = false) || x) !== false) {
  $ERROR('#1: var x = true; ((x = false) || x) === false');
}


var x = true; 
if ((x || (x = false)) !== true) {
  $ERROR('#2: var x = true; (x || (x = false)) === true');
}

