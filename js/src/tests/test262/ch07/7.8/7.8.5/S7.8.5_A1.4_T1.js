











if (/\1/.source !== "\\1") {
  $ERROR('#1: /\\1/');
}   


if (/\a/.source !== "\\a") {
  $ERROR('#2: /\\a/');
}


if (/\;/.source !== "\\;") {
  $ERROR('#3: /\\;/');
}


if (/\ /.source !== "\\ ") {
  $ERROR('#4: /\\ /');
}  

