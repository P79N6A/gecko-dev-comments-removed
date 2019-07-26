










if (Number("1") !== 1)  {
  $ERROR('#1: Number("1") === 1. Actual: ' + (Number("1")));
}


if (Number("0x1") !== 1)  {
  $ERROR('#2: Number("0x1") === 1. Actual: ' + (Number("0x1")));
}


if (+("0X1") !== 1)  {
  $ERROR('#3: +("0X1") === 1. Actual: ' + (+("0X1")));
}

