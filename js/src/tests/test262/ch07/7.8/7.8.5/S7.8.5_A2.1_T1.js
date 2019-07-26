











if (/1a/.source !== "1a") {
  $ERROR('#1: /1a/');
}   


if (/aa/.source !== "aa") {
  $ERROR('#2: /aa/');
}


if (/,;/.source !== ",;") {
  $ERROR('#3: /,;/');
}


if (/  /.source !== "  ") {
  $ERROR('#4: /  /');
}      


if (/a\u0041/.source !== "a\\u0041") {
  $ERROR('#5: /a\\u0041/');
}  

