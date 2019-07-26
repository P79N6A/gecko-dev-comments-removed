










if (+("2") !== 2)  {
  $ERROR('#1: +("2") === 2. Actual: ' + (+("2")));
}


if (Number("0x2") !== 2)  {
  $ERROR('#2: Number("0x2") === 2. Actual: ' + (Number("0x2")));
}


if (Number("0X2") !== 2)  {
  $ERROR('#3: Number("0X2") === 2. Actual: ' + (Number("0X2")));
}

