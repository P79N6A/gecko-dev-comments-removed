










































var SECTION = "java array object inheritance JavaScript Array methods";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 " + SECTION;

startTest();

var a = new Array();

a[a.length] = new TestObject(
    "var b"+a.length+" = new java.lang.String(\"hello\").getBytes(); b"+a.length+".join() +''",
    "b"+a.length,
    "join",
    true,
    "104,101,108,108,111" );

a[a.length] = new TestObject(
    "var b"+a.length+" = new java.lang.String(\"JavaScript\").getBytes(); b"+a.length+".reverse().join() +''",
    "b"+a.length,
    "reverse",
    true,
    getCharValues("tpircSavaJ") );

a[a.length] = new TestObject(
    "var b"+a.length+" = new java.lang.String(\"JavaScript\").getBytes(); b"+a.length+".sort().join() +''",
    "b"+a.length,
    "sort",
    true,
    "105,112,114,116,118,74,83,97,97,99" );

a[a.length] = new TestObject(
    "var b"+a.length+" = new java.lang.String(\"JavaScript\").getBytes(); b"+a.length+".sort().join() +''",
    "b"+a.length,
    "sort",
    true,
    "105,112,114,116,118,74,83,97,97,99" );

test();




function getCharValues(string) {
    for ( var c = 0, cString = ""; c < string.length; c++ ) {
	cString += string.charCodeAt(c) + ((c+1 < string.length) ? "," : "");
    }
    return cString;
}








function TestObject( description, ob, method, override, expect ) {
    this.description = description;
    this.object = ob;
    this.method = method;
    this.override = override
        this.expect;

    this.result = eval(description);

    this.isJSMethod = eval( ob +"."+ method +" == Array.prototype." + method );

    

    new TestCase(
	description,
	expect,
	this.result );

    

    new TestCase(
	ob +"." + method +" == Array.prototype." + method,
	override,
	this.isJSMethod );

    

    new TestCase(
	ob + ".getClass().getName() +''",
	"[B",
	eval( ob+".getClass().getName() +''") );

}
