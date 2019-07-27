



function testCaseFn() {
































}

var str = testCaseFn.toString().replace("/*","").replace("*/","");
str = str.replace("function testCaseFn() {\n", "").replace("/*End func*/}","");
var hasTemplateStrings = false;
try { eval("``"); hasTemplateStrings = true; } catch (exc) { }
if (hasTemplateStrings)
    eval(str);
