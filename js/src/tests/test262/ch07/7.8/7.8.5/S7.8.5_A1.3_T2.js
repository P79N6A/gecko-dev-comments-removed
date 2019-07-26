










try {
   eval("/\u000A/").source;
   $ERROR('#1.1: RegularExpressionFirstChar :: Line Feed is incorrect. Actual: ' + (eval("/\u000A/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionFirstChar :: Line Feed is incorrect. Actual: ' + (e));
  }
}     

