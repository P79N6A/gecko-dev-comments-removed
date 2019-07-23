




































var bug = 214761;
var summary = 'Crash Regression from bug 208030';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);

var code = "var bar1=new Array();\n" +
"bar1[0]='foo';\n" +
"var bar2=document.all&&navigator.userAgent.indexOf('Opera')==-1;\n" +
"var bar3=document.getElementById&&!document.all;\n" +
"var bar4=document.layers;\n" +
"function foo1(){\n" +
"return false;}\n" + 
"function foo2(){\n" +
"return false;}\n" + 
"function foo3(){\n" + 
"return false;}\n" + 
"function foo4(){\n" + 
"return false;}\n" + 
"function foo5(){\n" + 
"return false;}\n" + 
"function foo6(){\n" + 
"return false;}\n" + 
"function foo7(){\n" + 
"return false;}";

try
{
  eval(code);
}
catch(e)
{
}
  
jsOptions.reset();

reportCompare(expect, actual, summary);
