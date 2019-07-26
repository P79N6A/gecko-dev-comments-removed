










try {
   eval("/\u000D/").source;
   $ERROR('#1.1: RegularExpressionFirstChar :: Carriage Return is incorrect. Actual: ' + (eval("/\u000D/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionFirstChar :: Carriage Return is incorrect. Actual: ' + (e));
  }
}     

