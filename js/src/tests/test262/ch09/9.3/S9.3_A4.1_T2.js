










if (+(13) !== 13) {
  $ERROR('#1: +(13) === 13. Actual: ' + (+(13)));
}


if (+(-13) !== -13) { 
  $ERROR('#2: +(-13) === -13. Actual: ' + (+(-13)));
}


if (+(1.3) !== 1.3) {
  $ERROR('#3: +(1.3) === 1.3. Actual: ' + (+(1.3)));
}


if (+(-1.3) !== -1.3) {
  $ERROR('#4: +(-1.3) === -1.3. Actual: ' + (+(-1.3)));
}


if (+(Number.MAX_VALUE) !== 1.7976931348623157e308) {
  $ERROR('#5: +(Number.MAX_VALUE) === 1.7976931348623157e308. Actual: ' + (+(Number.MAX_VALUE)));
}


if (+(Number.MIN_VALUE) !== 5e-324) {
  $ERROR('#6: +(Number.MIN_VALUE) === 5e-324. Actual: ' + (+(Number.MIN_VALUE)));
}	

