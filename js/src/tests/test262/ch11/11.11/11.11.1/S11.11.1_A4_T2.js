










if ((-1 && -0) !== 0) {
  $ERROR('#1.1: (-1 && -0) === 0');
} else {
  if ((1 / (-1 && -0)) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#1.2: (-1 && -0) === -0');
  }
}


if ((-1 && 0) !== 0) {
  $ERROR('#2.1: (-1 && 0) === 0');
} else {
  if ((1 / (-1 && 0)) !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: (-1 && 0) === +0');
  }
}


if ((isNaN(0.1 && NaN)) !== true) {
  $ERROR('#3: (0.1 && NaN) === Not-a-Number');
}


var y = new Number(0);
if ((new Number(-1) && y) !== y) {
  $ERROR('#4: (var y = new Number(0); (new Number(-1) && y) === y');
}


var y = new Number(NaN);
if ((new Number(0) && y) !== y) {
  $ERROR('#5: (var y = new Number(NaN); (new Number(0) && y) === y');
}


var y = new Number(-1);
if ((new Number(NaN) && y) !== y) {
  $ERROR('#6: (var y = new Number(-1); (new Number(NaN) && y) === y');
}

