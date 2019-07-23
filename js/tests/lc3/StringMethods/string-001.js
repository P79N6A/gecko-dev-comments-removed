





































gTestfile = 'string-001.js';






var SECTION = "java.lang.Strings using JavaScript String methods";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 " + SECTION;

startTest();

var jm = getMethods( "java.lang.String" );
var methods = new Array();

for ( var i = 0; i < jm.length; i++ ) {
  cm = jm[i].toString();
  methods[methods.length] = [ getMethodName(cm), getArguments(cm) ];
}

var a = new Array();






a[a.length] = new TestObject(
  "var s"+a.length+" = new java.lang.String(\"hello\"); s"+a.length+".valueOf("+a.length+") +''",
  "s"+a.length,
  "valueOf",
  1,
  false,
  "0.0" );






a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"boo\"); s"+a.length+".toString() +''",
  "s"+a.length,
  "toString",
  0,
  false,
  "boo" );

a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".charAt(0)",
  "s"+a.length,
  "charAt",
  1,
  false,
  "J".charCodeAt(0) );

a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".indexOf(\"L\")",
  "s"+a.length,
  "indexOf",
  1,
  false,
  11 );


a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".lastIndexOf(\"t\")",
  "s"+a.length,
  "lastIndexOf",
  1,
  false,
  21 );

a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".substring(\"11\") +''",
  "s"+a.length,
  "substring",
  1,
  false,
  "LiveConnect" );

a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".substring(\"15\") +''",
  "s"+a.length,
  "substring",
  1,
  false,
  "Connect" );

a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".substring(4,10) +''",
  "s"+a.length,
  "substring",
  2,
  false,
  "Script" );

a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".toLowerCase() +''",
  "s"+a.length,
  "substring",
  0,
  false,
  "javascript liveconnect" );

a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"JavaScript LiveConnect\"); s"+a.length+".toUpperCase() +''",
  "s"+a.length,
  "substring",
  0,
  false,
  "JAVASCRIPT LIVECONNECT" );

















a[a.length] = new TestObject(
  "var s" +a.length+" = new java.lang.String(\"0 1 2 3 4 5 6 7 8 9\"); s"+a.length+".constructor",
  "s"+a.length,
  "constructor",
  0,
  true,
  String.prototype.constructor);


test();








function TestObject( description, ob, method, argLength, override, expect ) {
  this.description = description;
  this.object = ob;
  this.method = method;
  this.override = override
    this.argLength = argLength;
  this.expect;

  this.result = eval(description);

  this.isJSMethod = eval( ob +"."+ method +" == String.prototype." + method );

  new TestCase(
    description,
    expect,
    this.result );

  if ( hasMethod( method, argLength )  ) {
    new TestCase(
      ob +"." + method +" == String.prototype." + method,
      override,
      this.isJSMethod );

  } else  {
    
    
    

    new TestCase(
      ob +"." + method +" == String.prototype." + method,
      override,
      this.isJSMethod );
  }
}

function getMethods( javaString ) {
  return java.lang.Class.forName( javaString ).getMethods();
}
function isStatic( m ) {
  if ( m.lastIndexOf("static") > 0 ) {
    
    return true;
  }
  return false;
}
function getArguments( m ) {
  var argIndex = m.lastIndexOf("(", m.length());
  var argString = m.substr(argIndex+1, m.length() - argIndex -2);
  return argString.split( "," );
}
function getMethodName( m ) {
  var argIndex = m.lastIndexOf( "(", m.length());
  var nameIndex = m.lastIndexOf( ".", argIndex);
  return m.substr( nameIndex +1, argIndex - nameIndex -1 );
}
function hasMethod( m, noArgs ) {
  for ( var i = 0; i < methods.length; i++ ) {
    if ( (m == methods[i][0]) && (noArgs == methods[i][1].length)) {
      return true;
    }
  }
  return false;
}
