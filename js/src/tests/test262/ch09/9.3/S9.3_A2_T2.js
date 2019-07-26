










if (+(null) !== 0) {
  $ERROR('#1.1: +(null) === 0. Actual: ' + (+(null))); 
} else {
  if (1/+(null) !== Number.POSITIVE_INFINITY) {
    $ERROR('#1.2: +(null) === +0. Actual: -0');
  }	
}

