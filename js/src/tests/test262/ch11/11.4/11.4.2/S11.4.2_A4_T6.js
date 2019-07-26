










var x = 0;
if (void (x = 1) !== undefined) {
  $ERROR('#1: var x = 0; void (x = 1) === undefined. Actual: ' + (void (x = 1)));
} else {
  if (x !== 1) {
    $ERROR('#1: var x = 0; void (x = 1); x === 1. Actual: ' + (x));
  } 
}

