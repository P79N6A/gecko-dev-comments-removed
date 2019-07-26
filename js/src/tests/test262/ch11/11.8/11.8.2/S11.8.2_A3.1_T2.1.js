










if (true > 1 !== false) {
  $ERROR('#1: true > 1 === false');
}


if (1 > true !== false) {
  $ERROR('#2: 1 > true === false');
}


if (new Boolean(true) > 1 !== false) {
  $ERROR('#3: new Boolean(true) > 1 === false');
}


if (1 > new Boolean(true) !== false) {
  $ERROR('#4: 1 > new Boolean(true) === false');
}


if (true > new Number(1) !== false) {
  $ERROR('#5: true > new Number(1) === false');
}


if (new Number(1) > true !== false) {
  $ERROR('#6: new Number(1) > true === false');
}


if (new Boolean(true) > new Number(1) !== false) {
  $ERROR('#7: new Boolean(true) > new Number(1) === false');
}


if (new Number(1) > new Boolean(true) !== false) {
  $ERROR('#8: new Number(1) > new Boolean(true) === false');
}

