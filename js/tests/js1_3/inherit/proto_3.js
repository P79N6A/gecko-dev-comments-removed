





































gTestfile = 'proto_3.js';


















var SECTION = "proto_3";
var VERSION = "JS1_3";
var TITLE   = "Adding properties to an Instance";

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
var pat = new Employee();

jim.bonus = 300;

new TestCase( SECTION,
	      "jim = new Employee(); jim.bonus = 300; jim.bonus",
	      300,
	      jim.bonus );


new TestCase( SECTION,
	      "pat = new Employee(); pat.bonus",
	      void 0,
	      pat.bonus );
test();
