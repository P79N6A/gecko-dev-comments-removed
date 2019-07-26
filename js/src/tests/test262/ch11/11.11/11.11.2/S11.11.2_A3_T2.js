










if ((0 || -0) !== 0) {
  $ERROR('#1.1: (0 || -0) === 0');
} else {
  if ((1 / (0 || -0)) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#1.2: (0 || -0) === -0');
  }
}


if ((-0 || 0) !== 0) {
  $ERROR('#2.1: (-0 || 0) === 0');
} else {
  if ((1 / (-0 || 0)) !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: (-0 || 0) === +0');
  }
}


var y = new Number(-1);
if ((0 || y) !== y) {
  $ERROR('#3: (var y = new Number(-1); 0 || y) === y');
} 


var y = new Number(0);
if ((NaN || y) !== y) {
  $ERROR('#4: (var y = new Number(0); NaN || y) === y');
}

