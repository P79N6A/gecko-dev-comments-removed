










try {
   eval("/\\\u000D/").source;
   $ERROR('#1.1: RegularExpressionFirstChar :: BackslashSequence :: \\Carriage Return is incorrect. Actual: ' + (eval("/\\\u000D/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionFirstChar :: BackslashSequence :: \\Carriage Return is incorrect. Actual: ' + (e));
  }
}     

