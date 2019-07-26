










if (Number(null) !== 0) {
  $ERROR('#1.1: Number(null) === 0. Actual: ' + (Number(null))); 
} else {
  if (1/Number(null) !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: Number(null) === +0. Actual: -0');
  }	
}

