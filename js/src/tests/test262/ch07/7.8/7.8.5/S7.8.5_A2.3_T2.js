










try {
   eval("/a\u000A/").source;
   $ERROR('#1.1: RegularExpressionChar :: Line Feedis incorrect. Actual: ' + (eval("/a\u000A/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionChar :: Line Feed is incorrect. Actual: ' + (e));
  }
}     

