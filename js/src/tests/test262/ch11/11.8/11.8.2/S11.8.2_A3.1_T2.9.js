










if (true > null !== true) {
  $ERROR('#1: true > null === true');
}


if (null > true !== false) {
  $ERROR('#2: null > true === false');
}


if (new Boolean(true) > null !== true) {
  $ERROR('#3: new Boolean(true) > null === true');
}


if (null > new Boolean(true) !== false) {
  $ERROR('#4: null > new Boolean(true) === false');
}

