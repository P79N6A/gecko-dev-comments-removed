










var x = -1;
var z = (x %= 2);
if (z !== -1) {
  $ERROR('#1: var x = -1; var z = (x %= 2); z === -1. Actual: ' + (z));
}


var x = -1;
var y = 2;
var z = (x %= y);
if (z !== -1) {
  $ERROR('#2: var x = -1; var y = 2; var z = (x %= y); z === -1. Actual: ' + (z));
}


