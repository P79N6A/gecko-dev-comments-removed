











if (Number("1234e5") !== Number("1234")*1e5)  {
  $ERROR('#1: Number("1234e5") === Number("1234")*1e5');
}


if (Number("1234.e5") !== +("1234")*1e5)  {
  $ERROR('#2: Number("1234.e5") === +("1234")*1e5');
}

