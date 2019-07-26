











if (isNaN(Number(Number.NaN)) !== true) {
  $ERROR('#1: Number(NaN) === Not-a-Number. Actual: ' + (Number(NaN))); 
}


if (Number(+0) !== +0) {
  $ERROR('#2.1: Number(+0) === 0. Actual: ' + (Number(+0))); 
} else {
  if (1/Number(+0) !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: Number(+0) === +0. Actual: -0');
  }	
}


if (Number(-0) !== -0) {
  $ERROR('#3.1: Number(-0) === 0. Actual: ' + (Number(-0))); 
} else {
  if (1/Number(-0) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#3.2: Number(-0) === -0. Actual: +0');
  }	
}


if (Number(Number.POSITIVE_INFINITY) !== Number.POSITIVE_INFINITY) {
  $ERROR('#4: Number(+Infinity) === +Infinity. Actual: ' + (Number(+Infinity))); 
}


if (Number(Number.NEGATIVE_INFINITY) !== Number.NEGATIVE_INFINITY) {
  $ERROR('#5: Number(-Infinity) === -Infinity. Actual: ' + (Number(-Infinity))); 
}


if (Number(Number.MAX_VALUE) !== Number.MAX_VALUE) {
  $ERROR('#6: Number(Number.MAX_VALUE) === Number.MAX_VALUE. Actual: ' + (Number(Number.MAX_VALUE))); 
}


if (Number(Number.MIN_VALUE) !== Number.MIN_VALUE) {
  $ERROR('#7: Number(Number.MIN_VALUE) === Number.MIN_VALUE. Actual: ' + (Number(Number.MIN_VALUE))); 
}

