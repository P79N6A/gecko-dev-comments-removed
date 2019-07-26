










var x = false; 
if (((x = true) && x) !== true) {
  $ERROR('#1: var x = false; ((x = true) && x) === true');
}


var x = false; 
if ((x && (x = true)) !== false) {
  $ERROR('#2: var x = false; (x && (x = true)) === false');
}


