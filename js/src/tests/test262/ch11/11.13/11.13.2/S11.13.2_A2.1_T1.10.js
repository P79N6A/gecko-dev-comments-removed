










var x = 1;
var z = (x ^= 1);
if (z !== 0) {
  $ERROR('#1: var x = 1; var z = (x ^= 1); z === 0. Actual: ' + (z));
}


var x = 1;
var y = 1;
var z = (x ^= y);
if (z !== 0) {
  $ERROR('#2: var x = 1; var y = 1; var z = (x ^= y); z === 0. Actual: ' + (z));
}


