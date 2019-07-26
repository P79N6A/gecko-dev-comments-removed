










if (String(NaN) !== "NaN") {
  $ERROR('#1: String(NaN) === Not-a-Number Actual: ' + (String(NaN)));
}


if (String(Number.NaN) !== "NaN") {
  $ERROR('#2: String(Number.NaN) === Not-a-Number Actual: ' + (String(Number.NaN)));
}


if (String(Number("asasa")) !== "NaN") {
  $ERROR('#3: String(Number("asasa")) === Not-a-Number Actual: ' + (String(Number("asasa"))));
}

