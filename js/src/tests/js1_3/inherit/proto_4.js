





































gTestfile = 'proto_4.js';























var SECTION = "proto_3";
var VERSION = "JS1_3";
var TITLE   = "Adding properties to the prototype";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

function Employee () {
  this.name = "";
  this.dept = "general";
}
function Manager () {
  this.reports = [];
}
Manager.prototype = new Employee();

function WorkerBee () {
  this.projects = new Array();
}

WorkerBee.prototype = new Employee();

function SalesPerson () {
  this.dept = "sales";
  this.quota = 100;
}
SalesPerson.prototype = new WorkerBee();

function Engineer () {
  this.dept = "engineering";
  this.machine = "";
}
Engineer.prototype = new WorkerBee();

var jim = new Employee();
var terry = new Engineer();
var sean = new SalesPerson();
var wally = new Manager();

Employee.prototype.specialty = "none";

var pat = new Employee();
var leslie = new Engineer();
var bubbles = new SalesPerson();
var furry = new Manager();

Engineer.prototype.specialty = "code";

var chris = new Engineer();


new TestCase( SECTION,
	      "jim = new Employee(); jim.specialty",
	      "none",
	      jim.specialty );

new TestCase( SECTION,
	      "terry = new Engineer(); terry.specialty",
	      "code",
	      terry.specialty );

new TestCase( SECTION,
	      "sean = new SalesPerson(); sean.specialty",
	      "none",
	      sean.specialty );

new TestCase( SECTION,
	      "wally = new Manager(); wally.specialty",
	      "none",
	      wally.specialty );

new TestCase( SECTION,
	      "furry = new Manager(); furry.specialty",
	      "none",
	      furry.specialty );

new TestCase( SECTION,
	      "pat = new Employee(); pat.specialty",
	      "none",
	      pat.specialty );

new TestCase( SECTION,
	      "leslie = new Engineer(); leslie.specialty",
	      "code",
	      leslie.specialty );

new TestCase( SECTION,
	      "bubbles = new SalesPerson(); bubbles.specialty",
	      "none",
	      bubbles.specialty );


new TestCase( SECTION,
	      "chris = new Employee(); chris.specialty",
	      "code",
	      chris.specialty );
test();
