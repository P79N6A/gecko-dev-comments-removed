





































gTestfile = 'proto_10.js';


















var SECTION = "proto_10";
var VERSION = "JS1_3";
var TITLE   = "Determining Instance Relationships";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

function InstanceOf( object, constructor ) {
  return object instanceof constructor;
}
function Employee ( name, dept ) {
  this.name = name || "";
  this.dept = dept || "general";
}

function Manager () {
  this.reports = [];
}
Manager.prototype = new Employee();

function WorkerBee ( name, dept, projs ) {
  this.base = Employee;
  this.base( name, dept)
    this.projects = projs || new Array();
}
WorkerBee.prototype = new Employee();

function SalesPerson () {
  this.dept = "sales";
  this.quota = 100;
}
SalesPerson.prototype = new WorkerBee();

function Engineer ( name, projs, machine ) {
  this.base = WorkerBee;
  this.base( name, "engineering", projs )
    this.machine = machine || "";
}
Engineer.prototype = new WorkerBee();

var pat = new Engineer();

new TestCase( SECTION,
              "InstanceOf( pat, Engineer )",
              true,
              InstanceOf( pat, Engineer ) );

new TestCase( SECTION,
              "InstanceOf( pat, WorkerBee )",
              true,
              InstanceOf( pat, WorkerBee ) );

new TestCase( SECTION,
              "InstanceOf( pat, Employee )",
              true,
              InstanceOf( pat, Employee ) );

new TestCase( SECTION,
              "InstanceOf( pat, Object )",
              true,
              InstanceOf( pat, Object ) );

new TestCase( SECTION,
              "InstanceOf( pat, SalesPerson )",
              false,
              InstanceOf ( pat, SalesPerson ) );
test();
