










if (!(+0) !== true) {
  $ERROR('#1: !(+0) === true. Actual: ' + (!(+0))); 	 
}


if (!(-0) !== true) {
  $ERROR('#2: !(-0) === true. Actual: ' + (!(-0)));
}


if (!(Number.NaN) !== true) {
  $ERROR('#3: !(Number.NaN) === true. Actual: ' + (!(Number.NaN)));
}

