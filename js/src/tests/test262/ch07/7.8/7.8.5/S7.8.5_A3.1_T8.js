










var regexp;
eval("regexp = /(?:)/\u0069"); 
if (regexp.ignoreCase !== true) {
  $ERROR('#1: var regexp = /(?:)/\\u0069; regexp.ignoreCase === true. Actual: ' + (regexp.ignoreCase));
}                         

