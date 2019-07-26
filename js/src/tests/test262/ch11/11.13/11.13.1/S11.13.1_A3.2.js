










var x = 0;
if ((x = 1) !== 1) {
  $ERROR('#1: var x = 0; (x = 1) === 1. Actual: ' + ((x = 1)));
}


x = 0;
if ((x = 1) !== 1) {
  $ERROR('#2: x = 0; (x = 1) === 1. Actual: ' + ((x = 1)));
}

