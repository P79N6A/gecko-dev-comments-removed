










if (true <= 1 !== true) {
  $ERROR('#1: true <= 1 === true');
}


if (1 <= true !== true) {
  $ERROR('#2: 1 <= true === true');
}


if (new Boolean(true) <= 1 !== true) {
  $ERROR('#3: new Boolean(true) <= 1 === true');
}


if (1 <= new Boolean(true) !== true) {
  $ERROR('#4: 1 <= new Boolean(true) === true');
}


if (true <= new Number(1) !== true) {
  $ERROR('#5: true <= new Number(1) === true');
}


if (new Number(1) <= true !== true) {
  $ERROR('#6: new Number(1) <= true === true');
}


if (new Boolean(true) <= new Number(1) !== true) {
  $ERROR('#7: new Boolean(true) <= new Number(1) === true');
}


if (new Number(1) <= new Boolean(true) !== true) {
  $ERROR('#8: new Number(1) <= new Boolean(true) === true');
}

