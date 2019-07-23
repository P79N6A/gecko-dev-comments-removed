





































gTestfile = '12.6.3-4.js';





































var SECTION = "12.6.3-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";
var BUGNUMBER="http://scopus.mcom.com/bugsplat/show_bug.cgi?id=344855";

writeHeaderToLog( SECTION + " "+ TITLE);




var o = new MyObject();
var result = 0;

for ( MyObject in o ) {
  result += o[MyObject];
}

new TestCase( SECTION,
	      "for ( MyObject in o ) { result += o[MyObject] }",
	      6,
	      result );

var result = 0;

for ( value in o ) {
  result += o[value];
}

new TestCase( SECTION,
	      "for ( value in o ) { result += o[value]",
	      6,
	      result );

var value = "value";
var result = 0;
for ( value in o ) {
  result += o[value];
}

new TestCase( SECTION,
	      "value = \"value\"; for ( value in o ) { result += o[value]",
	      6,
	      result );

var value = 0;
var result = 0;
for ( value in o ) {
  result += o[value];
}

new TestCase( SECTION,
	      "value = 0; for ( value in o ) { result += o[value]",
	      6,
	      result );



var ob = { 0:"hello" };
var result = 0;
for ( ob[0] in o ) {
  result += o[ob[0]];
}

new TestCase( SECTION,
	      "ob = { 0:\"hello\" }; for ( ob[0] in o ) { result += o[ob[0]]",
	      6,
	      result );

var result = 0;
for ( ob["0"] in o ) {
  result += o[ob["0"]];
}

new TestCase( SECTION,
	      "value = 0; for ( ob[\"0\"] in o ) { result += o[o[\"0\"]]",
	      6,
	      result );

var result = 0;
var ob = { value:"hello" };
for ( ob[value] in o ) {
  result += o[ob[value]];
}

new TestCase( SECTION,
	      "ob = { 0:\"hello\" }; for ( ob[value] in o ) { result += o[ob[value]]",
	      6,
	      result );

var result = 0;
for ( ob["value"] in o ) {
  result += o[ob["value"]];
}

new TestCase( SECTION,
	      "value = 0; for ( ob[\"value\"] in o ) { result += o[ob[\"value\"]]",
	      6,
	      result );

var result = 0;
for ( ob.value in o ) {
  result += o[ob.value];
}

new TestCase( SECTION,
	      "value = 0; for ( ob.value in o ) { result += o[ob.value]",
	      6,
	      result );










test();

function MyObject() {
  this.value = 2;
  this[0] = 4;
  return this;
}
