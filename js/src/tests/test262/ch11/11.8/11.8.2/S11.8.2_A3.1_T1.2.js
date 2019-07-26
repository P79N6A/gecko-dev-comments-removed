










if (1 > 1 !== false) {
  $ERROR('#1: 1 > 1 === false');
}


if (new Number(1) > 1 !== false) {
  $ERROR('#2: new Number(1) > 1 === false');
}


if (1 > new Number(1) !== false) {
  $ERROR('#3: 1 > new Number(1) === false');
}


if (new Number(1) > new Number(1) !== false) {
  $ERROR('#4: new Number(1) > new Number(1) === false');
}


