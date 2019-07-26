










if (eval("'\u0020str\u0020ing\u0020'") !== "\u0020str\u0020ing\u0020") {
  $ERROR('#1: eval("\'\\u0020str\\u0020ing\\u0020\'") === "\\u0020str\\u0020ing\\u0020"');
}


if (eval("' str ing '") !== " str ing ") {
  $ERROR('#2: eval("\' str ing \'") === " str ing "');
}

