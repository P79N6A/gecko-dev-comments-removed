










var x = 0;
var y = 0;
var z = 0;
if ((x = 1, y = 2, z = 3) !== 3) {
  $ERROR('#1: var x = 0; var y = 0; var z = 0; (x = 1, y = 2, z = 3) === 3. Actual: ' + ((x = 1, y = 2, z = 3)));
}

var x = 0;
var y = 0;
var z = 0;
x = 1, y = 2, z = 3;


if (x !== 1) {
  $ERROR('#2: var x = 0; var y = 0; var z = 0; x = 1, y = 2, z = 3; x === 1. Actual: ' + (x));
}


if (y !== 2) {
  $ERROR('#3: var x = 0; var y = 0; var z = 0; x = 1, y = 2, z = 3; y === 2. Actual: ' + (y));
}


if (z !== 3) {
  $ERROR('#4: var x = 0; var y = 0; var z = 0; x = 1, y = 2, z = 3; z === 3. Actual: ' + (z));
}

