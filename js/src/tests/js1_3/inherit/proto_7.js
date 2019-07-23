





































gTestfile = 'proto_7.js';




















var SECTION = "proto_6";
var VERSION = "JS1_3";
var TITLE   = "Adding properties to the Prototype Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

function Employee ( name, dept ) {
  this.name = name || "";
  this.dept = dept || "general";
}
function WorkerBee ( name, dept, projs ) {
  this.base = Employee;
  this.base( name, dept)
    this.projects = projs || new Array();
}
WorkerBee.prototype = new Employee();

function Engineer ( name, projs, machine ) {
  this.base = WorkerBee;
  this.base( name, "engineering", projs )
    this.machine = machine || "";
}


var pat = new Engineer( "Toonces, Pat",
			["SpiderMonkey", "Rhino"],
			"indy" );

Employee.prototype.specialty = "none";




new TestCase( SECTION,
	      "pat.name",
	      "Toonces, Pat",
	      pat.name );

new TestCase( SECTION,
	      "pat.dept",
	      "engineering",
	      pat.dept );

new TestCase( SECTION,
	      "pat.projects.length",
	      2,
	      pat.projects.length );

new TestCase( SECTION,
	      "pat.projects[0]",
	      "SpiderMonkey",
	      pat.projects[0] );

new TestCase( SECTION,
	      "pat.projects[1]",
	      "Rhino",
	      pat.projects[1] );

new TestCase( SECTION,
	      "pat.machine",
	      "indy",
	      pat.machine );

new TestCase( SECTION,
	      "pat.specialty",
	      void 0,
	      pat.specialty );

test();
