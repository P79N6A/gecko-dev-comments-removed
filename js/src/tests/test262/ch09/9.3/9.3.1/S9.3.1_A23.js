










if (Number("7") !== 7)  {
  $ERROR('#1: Number("7") === 7. Actual: ' + (Number("7")));
}


if (Number("0x7") !== 7)  {
  $ERROR('#2: Number("0x7") === 7. Actual: ' + (Number("0x7")));
}


if (+("0X7") !== 7)  {
  $ERROR('#3: +("0X7") === 7. Actual: ' + (+("0X7")));
}

