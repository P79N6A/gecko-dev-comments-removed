










if (1 > null !== true) {
  $ERROR('#1: 1 > null === true');
}


if (null > 1 !== false) {
  $ERROR('#2: null > 1 === false');
}


if (new Number(1) > null !== true) {
  $ERROR('#3: new Number(1) > null === true');
}


if (null > new Number(1) !== false) {
  $ERROR('#4: null > new Number(1) === false');
}

