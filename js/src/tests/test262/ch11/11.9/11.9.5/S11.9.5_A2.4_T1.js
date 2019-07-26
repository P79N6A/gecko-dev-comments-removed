










var x = 0; 
if ((x = 1) !== x) {
  $ERROR('#1: var x = 0; (x = 1) === x');
}


var x = 0; 
if (!(x !== (x = 1))) {
  $ERROR('#2: var x = 0; x !== (x = 1)');
}


