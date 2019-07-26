











var d1 = new Date(Number.NaN);
if (!isNaN(d1.valueOf())) {
  $ERROR('#1: var d1 = new Date(Number.NaN); d1.valueOf() === Number.NaN;');
}


var d2 = new Date(Infinity);
if (!isNaN(d2.valueOf())) {
  $ERROR('#2: var d2 = new Date(Infinity); d2.valueOf() === Number.NaN;');
}


var d3 = new Date(-Infinity);
if (!isNaN(d3.valueOf())) {
  $ERROR('#3: var d3 = new Date(-Infinity); d3.valueOf() === Number.NaN;');
}


var d4 = new Date(0);
if (d4.valueOf() !== 0) {
  $ERROR('#4: var d4 = new Date(0); d4.valueOf() === 0;');
}


var d5 = new Date(-0);
if (d5.valueOf() !== -0) {
  $ERROR('#5: var d5 = new Date(-0); d5.valueOf() === -0;');
}

