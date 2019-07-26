










if (Number("4") !== 4)  {
  $ERROR('#1: Number("4") === 4. Actual: ' + (Number("4")));
}


if (Number("0x4") !== 4)  {
  $ERROR('#2: Number("0x4") === 4. Actual: ' + (Number("0x4")));
}


if (+("0X4") !== 4)  {
  $ERROR('#3: +("0X4") === 4. Actual: ' + (+("0X4")));
}

