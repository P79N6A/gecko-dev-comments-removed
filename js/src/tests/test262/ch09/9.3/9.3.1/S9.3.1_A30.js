










if (Number("0xe") !== 14)  {
  $ERROR('#1: Number("0xe") === 14. Actual: ' + (Number("0xe")));
}


if (Number("0xE") !== 14)  {
  $ERROR('#2: Number("0xE") === 14. Actual: ' + (Number("0xE")));
}


if (Number("0Xe") !== 14)  {
  $ERROR('#3: Number("0Xe") === 14. Actual: ' + (Number("0Xe")));
}


if (+("0XE") !== 14)  {
  $ERROR('#4: +("0XE") === 14. Actual: ' + (+("0XE")));
}

