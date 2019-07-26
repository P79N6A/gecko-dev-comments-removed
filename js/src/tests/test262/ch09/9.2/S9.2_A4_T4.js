











if (!(Number.POSITIVE_INFINITY) !== false) {
  $ERROR('#1: !(+Infinity) === false. Actual: ' + (!(+Infinity))); 	
}


if (!(Number.NEGATIVE_INFINITY) !== false) {
  $ERROR('#2: !(-Infinity) === false. Actual: ' + (!(-Infinity))); 	
}


if (!(Number.MAX_VALUE) !== false) {
  $ERROR('#3: !(Number.MAX_VALUE) === false. Actual: ' + (!(Number.MAX_VALUE))); 	
}


if (!(Number.MIN_VALUE) !== false) {
  $ERROR('#4: !(Number.MIN_VALUE) === false. Actual: ' + (!(Number.MIN_VALUE))); 	
}


if (!(13) !== false) {
  $ERROR('#5: !(13) === false. Actual: ' + (!(13)));	
}


if (!(-13) !== false) {
  $ERROR('#6: !(-13) === false. Actual: ' + (!(-13)));	
}


if (!(1.3) !== false) {
  $ERROR('#7: !(1.3) === false. Actual: ' + (!(1.3)));	
}


if (!(-1.3) !== false) {
  $ERROR('#8: !(-1.3) === false. Actual: ' + (!(-1.3)));	
}	

