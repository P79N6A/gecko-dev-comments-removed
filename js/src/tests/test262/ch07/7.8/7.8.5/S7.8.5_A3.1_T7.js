










var regexp;
eval("regexp = /(?:)/\u0067"); 
if (regexp.global !== true) {
  $ERROR('#1: var regexp = /(?:)/\\u0067; regexp.global === true. Actual: ' + (regexp.global));
}                        

