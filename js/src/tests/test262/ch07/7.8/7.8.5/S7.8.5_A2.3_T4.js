










try {
   eval("/a\u000D/").source;
   $ERROR('#1.1: RegularExpressionChar :: Carriage Retur is incorrect. Actual: ' + (eval("/a\u000D/").source)); 
}
catch (e) {
  if ((e instanceof SyntaxError) !== true) {
     $ERROR('#1.2: RegularExpressionChar :: Carriage Retur is incorrect. Actual: ' + (e));
  }
}     

