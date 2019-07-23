




































var gTestfile = 'regress-314887.js';

var BUGNUMBER = 314887;
var summary = 'Do not crash when morons embed script tags in external script files';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

<script language="JavaScript" type="text/JavaScript">
<!--

 </script>
 
reportCompare(expect, actual, summary);
