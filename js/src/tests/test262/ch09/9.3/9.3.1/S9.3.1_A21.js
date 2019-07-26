










if (+("5") !== 5)  {
  $ERROR('#1: +("5") === 5. Actual: ' + (+("5")));
}


if (Number("0x5") !== 5)  {
  $ERROR('#2: Number("0x5") === 5. Actual: ' + (Number("0x5")));
}


if (Number("0X5") !== 5)  {
  $ERROR('#3: Number("0X5") === 5. Actual: ' + (Number("0X5")));
}

