










if ((1 ? 0 : 1) !== 0) {
  $ERROR('#1: (1 ? 0 : 1) === 0');
}


var y = new Number(1);
if ((1 ? y : 0) !== y) {
  $ERROR('#2: (var y = new Number(1); (1 ? y : 0) === y');
}


var y = new Number(NaN);
if ((y ? y : 1) !== y) {
  $ERROR('#3: (var y = new Number(NaN); (y ? y : 1) === y');
}

