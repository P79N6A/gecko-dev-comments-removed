










var x; 
if (isNaN(++x) !== true) {
  $ERROR('#1: var x; ++x === Not-a-Number. Actual: ' + (++x));
}


var x = null;
if (++x !== 1) {
  $ERROR('#2: var x = null; ++x === 1. Actual: ' + (++x));
}

