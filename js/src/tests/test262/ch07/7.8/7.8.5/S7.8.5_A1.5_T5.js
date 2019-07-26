










try {
   eval("/\\\u2028/").source;
   $ERROR('#1.1: RegularExpressionFirstChar :: BackslashSequence :: \\Line separator is incorrect. Actual: ' + (eval("/\\\u2028/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionFirstChar :: BackslashSequence :: \\Line separator is incorrect. Actual: ' + (e));
  }
}     

