










var x;


if (x !== undefined) {
  $ERROR('#1: var x; x === undefined. Actual: ' + (x));
}


x++;


if (x === undefined) {
  $ERROR('#2: var x; x++; x !== undefined');
}



