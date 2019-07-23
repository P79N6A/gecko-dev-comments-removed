





































gTestfile = 'proto_1.js';


















var SECTION = "proto_1";
var VERSION = "JS1_3";
var TITLE   = "new PrototypeObject";

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

new TestCase( SECTION,
	      "jim = new Employee(); jim.name",
	      "",
	      jim.name );


new TestCase( SECTION,
	      "jim = new Employee(); jim.dept",
	      "general",
	      jim.dept );

var sally = new Manager();

new TestCase( SECTION,
	      "sally = new Manager(); sally.name",
	      "",
	      sally.name );
new TestCase( SECTION,
	      "sally = new Manager(); sally.dept",
	      "general",
	      sally.dept );

new TestCase( SECTION,
	      "sally = new Manager(); sally.reports.length",
	      0,
	      sally.reports.length );

new TestCase( SECTION,
	      "sally = new Manager(); typeof sally.reports",
	      "object",
	      typeof sally.reports );

var fred = new SalesPerson();

new TestCase( SECTION,
	      "fred = new SalesPerson(); fred.name",
	      "",
	      fred.name );

new TestCase( SECTION,
	      "fred = new SalesPerson(); fred.dept",
	      "sales",
	      fred.dept );

new TestCase( SECTION,
	      "fred = new SalesPerson(); fred.quota",
	      100,
	      fred.quota );

new TestCase( SECTION,
	      "fred = new SalesPerson(); fred.projects.length",
	      0,
	      fred.projects.length );

var jane = new Engineer();

new TestCase( SECTION,
	      "jane = new Engineer(); jane.name",
	      "",
	      jane.name );

new TestCase( SECTION,
	      "jane = new Engineer(); jane.dept",
	      "engineering",
	      jane.dept );

new TestCase( SECTION,
	      "jane = new Engineer(); jane.projects.length",
	      0,
	      jane.projects.length );

new TestCase( SECTION,
	      "jane = new Engineer(); jane.machine",
	      "",
	      jane.machine );


test();
