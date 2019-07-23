











































var SECTION = "for...in";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0";
SECTION;
startTest();




var dt = new Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass;

var a = new Array();

a[a.length] = new TestObject(
    dt.PUB_STRING,
    "var dt = new Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass; dt.getString()",
    0,
    "java.lang.String");

a[a.length] = new TestObject(
    dt.getLongObject(),
    "dt.getLongObject()",
    0,
    "java.lang.Long");

a[a.length] = new TestObject(
    new java.awt.Rectangle(5,6,7,8),
    "new java.awt.Rectangle(5,6,7,8)",
    0,
    "java.awt.Rectangle");

for ( var i = 0; i < a.length; i++ ) {
    

    for ( var arrayItem = 0; arrayItem < a[i].items; arrayItem++ ) {
	new TestCase(
	    "["+arrayItem+"]",
	    a[i].javaObject[arrayItem],
	    a[i].enumedObject[arrayItem] );
    }

    


    var fieldArray = getFields( a[i].javaClass );

    for ( var fieldIndex = 0; fieldIndex < fieldArray.length; fieldIndex++ ) {
	var f = fieldArray[fieldIndex] +"";

	if ( ! isStatic( f ) ) {
	    var currentField = getFieldName( f );

	    new TestCase(
		a[i].javaClass+"." + currentField + " enumerated ",
		true,
		a[i].enumedObject[currentField]+"" == a[i].javaObject[currentField] +"");
	}
    }

    

    var methodArray = getMethods(a[i].javaClass);

    for ( var methodIndex = 0; methodIndex < methodArray.length; methodIndex++ ) {
	var m = methodArray[methodIndex] +"";

	if ( ! isStatic(m)  && inClass( a[i].javaClass, m)) {
	    var currentMethod = getMethodName(m);

	    new TestCase(
		a[i].javaClass+"."+currentMethod + " enumerated ",
		true,
		a[i].enumedObject[currentMethod] +"" == a[i].javaObject[currentMethod] +"");

	}

    }
}


test();

function TestObject( ob, descr, len, jc) {
    this.javaObject= ob;
    this.description = descr;
    this.items    = len;
    this.javaClass = jc;
    this.enumedObject = new enumObject(ob);
}

function enumObject( o ) {
    this.pCount = 0;
    for ( var p in o ) {
	this.pCount++;
	this[p] = o[p];
    }
}




function inClass( javaClass, m ) {
    var argIndex = m.lastIndexOf( "(", m.length );
    var methodIndex = m.lastIndexOf( ".", argIndex );
    var classIndex = m.lastIndexOf( " ", methodIndex );
    var className =   m.substr(classIndex +1, methodIndex - classIndex -1 );

    if ( className == javaClass ) {
	return true;
    }
    return false;
}
function isStatic( m ) {
    if ( m.lastIndexOf ( "static" ) > 0 ) {
	return true;
    }
    return false;
}
function getMethods( javaString ) {
    return java.lang.Class.forName( javaString ).getMethods();
}
function getArguments( m ) {
    var argIndex = m.lastIndexOf("(", m.length());
    var argString = m.substr(argIndex+1, m.length() - argIndex -2);
    return argString.split( "," );
}
function getMethodName( m ) {
    var argIndex = m.lastIndexOf( "(", m.length);
    var nameIndex = m.lastIndexOf( ".", argIndex);
    return m.substr( nameIndex +1, argIndex - nameIndex -1 );
}
function getFields( javaClassName ) {
    return java.lang.Class.forName( javaClassName ).getFields();
}
function getFieldName( f ) {
    return f.substr( f.lastIndexOf(".", f.length)+1, f.length );
}
function getFieldNames( javaClassName ) {
    var javaFieldArray = getFields( javaClassName );

    for ( var i = 0, jsFieldArray = new Array(); i < javaFieldArray.length; i++ ) {
	var f = javaFieldArray[i] +"";
	jsFieldArray[i] = f.substr( f.lastIndexOf(".", f.length)+1, f.length );
    }
    return jsFieldArray;
}
function hasMethod( m, noArgs ) {
    for ( var i = 0; i < methods.length; i++ ) {
	if ( (m == methods[i][0]) && (noArgs == methods[i][1].length)) {
	    return true;
	}
    }
    return false;
}
