





































gTestfile = 'proto_9.js';




















var SECTION = "proto_9";
var VERSION = "JS1_3";
var TITLE   = "Local versus Inherited Values";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

function Employee ( name, dept ) {
  this.name = name || "";
  this.dept = dept || "general";
}
function WorkerBee ( name, dept, projs ) {
  this.projects = new Array();
}
WorkerBee.prototype = new Employee();

var pat = new WorkerBee()

  Employee.prototype.specialty = "none";
Employee.prototype.name = "Unknown";

Array.prototype.getClass = Object.prototype.toString;



new TestCase( SECTION,
	      "pat.name",
	      "",
	      pat.name );

new TestCase( SECTION,
	      "pat.dept",
	      "general",
	      pat.dept );

new TestCase( SECTION,
	      "pat.projects.getClass",
	      "[object Array]",
	      pat.projects.getClass() );

new TestCase( SECTION,
	      "pat.projects.length",
	      0,
	      pat.projects.length );

test();
