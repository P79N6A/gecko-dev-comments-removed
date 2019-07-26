










try {
  "MAX_VALUE" in NUMBER;
  $ERROR('#1.1: "MAX_VALUE" in NUMBER throw ReferenceError. Actual: ' + ("MAX_VALUE" in NUMBER));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: "MAX_VALUE" in NUMBER throw ReferenceError. Actual: ' + (e));  
  }
}

