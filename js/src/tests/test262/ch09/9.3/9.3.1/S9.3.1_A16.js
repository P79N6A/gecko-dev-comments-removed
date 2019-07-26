










if (Number("0") !== 0) {
  $ERROR('#1: Number("0") === 0. Actual: ' + (Number("0")));
}


if (+("0x0") !== 0) {
  $ERROR('#2: +("0x0") === 0. Actual: ' + (+("0x0")));
}


if (Number("0X0") !== 0) {
  $ERROR('#3: Number("0X0") === 0. Actual: ' + (Number("0X0")));
}

