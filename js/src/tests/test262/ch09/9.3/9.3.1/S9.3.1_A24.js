










if (+("8") !== 8)  {
  $ERROR('#1: +("8") === 8. Actual: ' + (+("8")));
}


if (Number("0x8") !== 8)  {
  $ERROR('#2: Number("0x8") === 8. Actual: ' + (Number("0x8")));
}


if (Number("0X8") !== 8)  {
  $ERROR('#3: Number("0X8") === 8. Actual: ' + (Number("0X8")));
}

