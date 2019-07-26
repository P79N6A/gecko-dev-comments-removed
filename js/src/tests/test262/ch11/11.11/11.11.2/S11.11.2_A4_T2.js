










if ((-1 || 1) !== -1) {
  $ERROR('#1: (-1 || 1) === -1');
}


if ((1 || new Number(0)) !== 1) {
  $ERROR('#2: (1 || new Number(0)) === 1');
} 


if ((-1 || NaN) !== -1) {
  $ERROR('#3: (-1 || NaN) === -1');
}


var x = new Number(-1);
if ((x || new Number(0)) !== x) {
  $ERROR('#4: (var x = new Number(-1); (x || new Number(-1)) === x');
}


var x = new Number(NaN);
if ((x || new Number(1)) !== x) {
  $ERROR('#5: (var x = new Number(NaN); (x || new Number(1)) === x');
}


var x = new Number(0);
if ((x || new Number(NaN)) !== x) {
  $ERROR('#6: (var x = new Number(0); (x || new Number(NaN)) === x');
}

