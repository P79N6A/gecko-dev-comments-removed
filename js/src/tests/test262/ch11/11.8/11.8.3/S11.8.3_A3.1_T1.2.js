










if (1 <= 1 !== true) {
  $ERROR('#1: 1 <= 1 === true');
}


if (new Number(1) <= 1 !== true) {
  $ERROR('#2: new Number(1) <= 1 === true');
}


if (1 <= new Number(1) !== true) {
  $ERROR('#3: 1 <= new Number(1) === true');
}


if (new Number(1) <= new Number(1) !== true) {
  $ERROR('#4: new Number(1) <= new Number(1) === true');
}


