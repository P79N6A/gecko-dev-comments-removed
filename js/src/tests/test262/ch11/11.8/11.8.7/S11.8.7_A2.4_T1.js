










var NUMBER = 0;
if ((NUMBER = Number, "MAX_VALUE") in NUMBER !== true) {
  $ERROR('#1: var NUMBER = 0; (NUMBER = Number, "MAX_VALUE") in NUMBER === true');
}


var max_value = "MAX_VALUE"; 
if (max_value in (max_value = "none", Number) !== true) {
  $ERROR('#2: var max_value = "MAX_VALUE"; max_value in (max_value = "none", Number) === true');
}


