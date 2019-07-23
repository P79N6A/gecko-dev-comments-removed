





































gTestfile = 'proto_2.js';


















var SECTION = "proto_2";
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

WorkerBee.prototype = new Employee;

function SalesPerson () {
  this.dept = "sales";
  this.quota = 100;
}
SalesPerson.prototype = new WorkerBee;

function Engineer () {
  this.dept = "engineering";
  this.machine = "";
}
Engineer.prototype = new WorkerBee;


var employee    = new Employee();
var manager     = new Manager();
var workerbee   = new WorkerBee();
var salesperson = new SalesPerson();
var engineer    = new Engineer();

new TestCase( SECTION,
	      "employee.__proto__ == Employee.prototype",
	      true,
	      employee.__proto__ == Employee.prototype );

new TestCase( SECTION,
	      "manager.__proto__ == Manager.prototype",
	      true,
	      manager.__proto__ == Manager.prototype );

new TestCase( SECTION,
	      "workerbee.__proto__ == WorkerBee.prototype",
	      true,
	      workerbee.__proto__ == WorkerBee.prototype );

new TestCase( SECTION,
	      "salesperson.__proto__ == SalesPerson.prototype",
	      true,
	      salesperson.__proto__ == SalesPerson.prototype );

new TestCase( SECTION,
	      "engineer.__proto__ == Engineer.prototype",
	      true,
	      engineer.__proto__ == Engineer.prototype );

test();

