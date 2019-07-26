










if (Boolean(+0) !== false) {
  $ERROR('#1: Boolean(+0) === false. Actual: ' + (Boolean(+0))); 	 
}


if (Boolean(-0) !== false) {
  $ERROR('#2: Boolean(-0) === false. Actual: ' + (Boolean(-0)));
}


if (Boolean(Number.NaN) !== false) {
  $ERROR('#3: Boolean(Number.NaN) === false. Actual: ' + (Boolean(Number.NaN)));
}

