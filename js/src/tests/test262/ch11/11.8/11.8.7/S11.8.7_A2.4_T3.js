










try {
  max_value in (max_value = "MAX_VALUE", Number);
  $ERROR('#1.1: max_value in (max_value = "MAX_VALUE", Number) throw ReferenceError. Actual: ' + (max_value in (max_value = "MAX_VALUE", Number)));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: max_value in (max_value = "MAX_VALUE", Number) throw ReferenceError. Actual: ' + (e));  
  }
}


if ((NUMBER = Number, "MAX_VALUE") in NUMBER !== true) {
  $ERROR('#2: (NUMBER = Number, "MAX_VALUE") in NUMBER !== true');
}


