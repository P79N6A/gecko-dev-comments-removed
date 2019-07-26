










if (Number("6") !== 6)  {
  $ERROR('#1: Number("6") === 6. Actual: ' + (Number("6")));
}


if (+("0x6") !== 6)  {
  $ERROR('#2: +("0x6") === 6. Actual: ' + (+("0x6")));
}


if (Number("0X6") !== 6)  {
  $ERROR('#3: Number("0X6") === 6. Actual: ' + (Number("0X6")));
}

