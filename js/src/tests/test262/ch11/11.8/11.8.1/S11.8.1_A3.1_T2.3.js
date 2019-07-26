










if (1 < null !== false) {
  $ERROR('#1: 1 < null === false');
}


if (null < 1 !== true) {
  $ERROR('#2: null < 1 === true');
}


if (new Number(1) < null !== false) {
  $ERROR('#3: new Number(1) < null === false');
}


if (null < new Number(1) !== true) {
  $ERROR('#4: null < new Number(1) === true');
}

