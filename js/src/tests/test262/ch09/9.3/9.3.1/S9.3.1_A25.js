










if (Number("9") !== 9)  {
  $ERROR('#1: Number("9") === 9. Actual: ' + (Number("9")));
}


if (+("0x9") !== 9)  {
  $ERROR('#2: +("0x9") === 9. Actual: ' + (+("0x9")));
}


if (Number("0X9") !== 9)  {
  $ERROR('#3: Number("0X9") === 9. Actual: ' + (Number("0X9")));
}

