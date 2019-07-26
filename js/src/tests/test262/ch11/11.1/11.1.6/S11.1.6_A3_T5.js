










(x) = 1;
if (x !== 1) {
  $ERROR('#1: (x) = 1; x === 1. Actual: ' + (x));
}


var y = 1; (y)++; ++(y); (y)--; --(y);
if (y !== 1) {
  $ERROR('#2: var y = 1; (y)++; ++(y); (y)--; --(y); y === 1. Actual: ' + (y));
}

