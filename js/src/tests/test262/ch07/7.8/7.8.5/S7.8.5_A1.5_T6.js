










try {
   eval("/\\\u2029/").source;
   $ERROR('#1.1: RegularExpressionFirstChar :: BackslashSequence :: \\Paragraph separator is incorrect. Actual: ' + (eval("/\\\u2029/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionFirstChar :: BackslashSequence :: \\Paragraph separator is incorrect. Actual: ' + (e));
  }
}     

