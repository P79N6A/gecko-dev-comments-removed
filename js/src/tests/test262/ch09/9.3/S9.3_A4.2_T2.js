











if (isNaN(+(Number.NaN)) !== true) {
  $ERROR('#1: +(NaN) === Not-a-Number. Actual: ' + (+(NaN))); 
}


if (+(+0) !== +0) {
  $ERROR('#2.1: +(+0) === 0. Actual: ' + (+(+0))); 
} else {
  if (1/+(+0) !== Number.POSITIVE_INFINITY) {
    $ERROR('#2.2: +(+0) === +0. Actual: -0');
  }	
}


if (+(-0) !== -0) {
  $ERROR('#3.1: +(-0) === 0. Actual: ' + (+(-0))); 
} else {
  if (1/+(-0) !== Number.NEGATIVE_INFINITY) {
    $ERROR('#3.2: +(-0) === -0. Actual: +0');
  }	
}


if (+(Number.POSITIVE_INFINITY) !== Number.POSITIVE_INFINITY) {
  $ERROR('#4: +(+Infinity) === +Infinity. Actual: ' + (+(+Infinity))); 
}


if (+(Number.NEGATIVE_INFINITY) !== Number.NEGATIVE_INFINITY) {
  $ERROR('#5: +(-Infinity) === -Infinity. Actual: ' + (+(-Infinity))); 
}


if (+(Number.MAX_VALUE) !== Number.MAX_VALUE) {
  $ERROR('#6: +(Number.MAX_VALUE) === Number.MAX_VALUE. Actual: ' + (+(Number.MAX_VALUE))); 
}


if (+(Number.MIN_VALUE) !== Number.MIN_VALUE) {
  $ERROR('#7: +(Number.MIN_VALUE) === Number.MIN_VALUE. Actual: ' + (+(Number.MIN_VALUE))); 
}

