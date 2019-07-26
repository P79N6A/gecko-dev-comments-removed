










if (isNaN(Number.NaN * Number.NaN) !== true) {
  $ERROR('#1: NaN * NaN === Not-a-Number. Actual: ' + (NaN * NaN));
}  


if (isNaN(Number.NaN * +0) !== true) {
  $ERROR('#2: NaN * +0 === Not-a-Number. Actual: ' + (NaN * +0)); 
} 


if (isNaN(Number.NaN * -0) !== true) {
  $ERROR('#3: NaN * -0 === Not-a-Number. Actual: ' + (NaN * -0)); 
} 


if (isNaN(Number.NaN * Number.POSITIVE_INFINITY) !== true) {
  $ERROR('#4: NaN * Infinity === Not-a-Number. Actual: ' + (NaN * Infinity));
} 


if (isNaN(Number.NaN * Number.NEGATIVE_INFINITY) !== true) {
  $ERROR('#5: NaN * -Infinity === Not-a-Number. Actual: ' + (NaN * -Infinity)); 
} 


if (isNaN(Number.NaN * Number.MAX_VALUE) !== true) {
  $ERROR('#6: NaN * Number.MAX_VALUE === Not-a-Number. Actual: ' + (NaN * Number.MAX_VALUE));
} 


if (isNaN(Number.NaN * Number.MIN_VALUE) !== true) {
  $ERROR('#7: NaN * Number.MIN_VALUE === Not-a-Number. Actual: ' + (NaN * Number.MIN_VALUE)); 
}


if (isNaN(Number.NaN * 1) !== true) {
  $ERROR('#8: NaN * 1 === Not-a-Number. Actual: ' + (NaN * 1));  
} 

