











if (/a\1/.source !== "a\\1") {
  $ERROR('#1: /a\\1/');
}   


if (/a\a/.source !== "a\\a") {
  $ERROR('#2: /a\\a/');
}


if (/,\;/.source !== ",\\;") {
  $ERROR('#3: /,\\;/');
}


if (/ \ /.source !== " \\ ") {
  $ERROR('#4: / \\ /');
}  

