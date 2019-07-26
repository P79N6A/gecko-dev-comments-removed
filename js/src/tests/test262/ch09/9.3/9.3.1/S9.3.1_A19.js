










if (Number("3") !== 3)  {
  $ERROR('#1: Number("3") === 3. Actual: ' + (Number("3")));
}


if (+("0x3") !== 3)  {
  $ERROR('#2: +("0x3") === 3. Actual: ' + (+("0x3")));
}


if (Number("0X3") !== 3)  {
  $ERROR('#3: Number("0X3") === 3. Actual: ' + (Number("0X3")));
}

