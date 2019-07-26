










var regexp;
eval("regexp = /(?:)/\u006D");
if (regexp.multiline !== true) {
  $ERROR('#1: var regexp = /(?:)/\\u006D; regexp.multiline === true. Actual: ' + (regexp.multiline));
}                             

