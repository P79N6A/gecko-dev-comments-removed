










label1: for (var i = 0; i <= 0; i++) {
  for (var j = 0; j <= 0; j++) {
    break label1;
  }
  $ERROR('#1: Check break statement for automatic semicolon insertion');
}


var result = false;
label2: for (var i = 0; i <= 0; i++) {
  for (var j = 0; j <= 0; j++) {
    break 
    label2;
  }  
  result = true;
}

if (result !== true) {
  $ERROR('#2: Check break statement for automatic semicolon insertion');
}

