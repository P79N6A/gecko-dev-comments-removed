










try {
   eval("/s\u2029/").source;
   $ERROR('#1.1: RegularExpressionChar :: Paragraph separator is incorrect. Actual: ' + (eval("/s\u2029/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionChar :: Paragraph separator is incorrect. Actual: ' + (e));
  }
}     

