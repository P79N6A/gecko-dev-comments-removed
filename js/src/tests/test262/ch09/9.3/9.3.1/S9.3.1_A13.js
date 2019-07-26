











if (+("12") !== Number("1")*10+Number("2"))  {
  $ERROR('#1: +("12") === Number("1")*10+Number("2")');
}


if (Number("123") !== Number("12")*10+Number("3"))  {
  $ERROR('#2: Number("123") === Number("12")*10+Number("3")');
}


if (Number("1234") !== Number("123")*10+Number("4"))  {
  $ERROR('#2: Number("1234") === Number("123")*10+Number("4")');
}

