










if (Number(13) !== 13) {
  $ERROR('#1: Number(13) === 13. Actual: ' + (Number(13)));
}


if (Number(-13) !== -13) { 
  $ERROR('#2: Number(-13) === -13. Actual: ' + (Number(-13)));
}


if (Number(1.3) !== 1.3) {
  $ERROR('#3: Number(1.3) === 1.3. Actual: ' + (Number(1.3)));
}


if (Number(-1.3) !== -1.3) {
  $ERROR('#4: Number(-1.3) === -1.3. Actual: ' + (Number(-1.3)));
}


if (Number(Number.MAX_VALUE) !== 1.7976931348623157e308) {
  $ERROR('#5: Number(Number.MAX_VALUE) === 1.7976931348623157e308. Actual: ' + (Number(Number.MAX_VALUE)));
}


if (Number(Number.MIN_VALUE) !== 5e-324) {
  $ERROR('#6: Number(Number.MIN_VALUE) === 5e-324. Actual: ' + (Number(Number.MIN_VALUE)));
}	

