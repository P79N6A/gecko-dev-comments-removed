










if (isNaN(Number.NaN % Number.NaN) !== true) {
  $ERROR('#1: NaN % NaN === Not-a-Number. Actual: ' + (NaN % NaN));
}  


if (isNaN(+0 % Number.NaN) !== true) {
  $ERROR('#2: +0 % NaN === Not-a-Number. Actual: ' + (+0 % NaN)); 
} 


if (isNaN(-0 % Number.NaN) !== true) {
  $ERROR('#3: -0 % NaN === Not-a-Number. Actual: ' + (-0 % NaN)); 
} 


if (isNaN(Number.POSITIVE_INFINITY % Number.NaN) !== true) {
  $ERROR('#4: Infinity % NaN === Not-a-Number. Actual: ' + (Infinity % NaN));
} 


if (isNaN(Number.NEGATIVE_INFINITY % Number.NaN) !== true) {
  $ERROR('#5:  -Infinity % NaN === Not-a-Number. Actual: ' + ( -Infinity % NaN)); 
} 


if (isNaN(Number.MAX_VALUE % Number.NaN) !== true) {
  $ERROR('#6: Number.MAX_VALUE % NaN === Not-a-Number. Actual: ' + (Number.MAX_VALUE % NaN));
} 


if (isNaN(Number.MIN_VALUE % Number.NaN) !== true) {
  $ERROR('#7: Number.MIN_VALUE % NaN === Not-a-Number. Actual: ' + (Number.MIN_VALUE % NaN)); 
}


if (isNaN(1 % Number.NaN) !== true) {
  $ERROR('#8: 1 % NaN === Not-a-Number. Actual: ' + (1 % NaN));  
}

