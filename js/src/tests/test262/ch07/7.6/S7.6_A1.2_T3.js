










var identifier = String.fromCharCode(0x0024);
eval("var " + identifier + "=1");
if (eval(identifier + "===1") !== true) {
  $ERROR('#1: var identifier = String.fromCharCode(0x0024); eval("var " + identifier + "=1"); eval(identifier + "===1") === true');
}


if ("$" !== String.fromCharCode(0x0024)) {
  $ERROR('#2: "$" === String.fromCharCode(0x0024)');
}

