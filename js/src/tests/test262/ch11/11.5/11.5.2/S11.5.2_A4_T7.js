










if (isNaN(+0 / +0) !== true) {
  $ERROR('#1: +0 / +0 === Not-a-Number. Actual: ' + (+0 / +0));
}  


if (isNaN(-0 / +0) !== true) {
  $ERROR('#2: -0 / +0 === Not-a-Number. Actual: ' + (-0 / +0)); 
} 


if (isNaN(+0 / -0) !== true) {
  $ERROR('#3: +0 / -0 === Not-a-Number. Actual: ' + (+0 / -0)); 
} 


if (isNaN(-0 / -0) !== true) {
  $ERROR('#4: -0 / -0 === Not-a-Number. Actual: ' + (-0 / -0));
} 

