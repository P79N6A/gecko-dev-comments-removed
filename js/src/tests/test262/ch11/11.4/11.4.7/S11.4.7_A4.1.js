










if (isNaN(-NaN) !== true) {
  $ERROR('#1: -NaN === Not-a-Number. Actual: ' + (-NaN));
}


var x = NaN; 
if (isNaN(-x) != true) {
  $ERROR('#2: var x = NaN; -x === Not-a-Number. Actual: ' + (-x));
}

