










try {
   eval("/a\u2028/").source;
   $ERROR('#1.1: RegularExpressionChar :: Line separator is incorrect. Actual: ' + (eval("/a\u2028/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionChar :: Line separator is incorrect. Actual: ' + (e));
  }
}     

