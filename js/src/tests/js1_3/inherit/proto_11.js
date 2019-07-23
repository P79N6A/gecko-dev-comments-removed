





































gTestfile = 'proto_11.js';


















var SECTION = "proto_11";
var VERSION = "JS1_3";
var TITLE   = "Global Information in Constructors";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var idCounter = 1;


function Employee ( name, dept ) {
  this.name = name || "";
  this.dept = dept || "general";
  this.id = idCounter++;
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

var pat = new Employee( "Toonces, Pat", "Tech Pubs" )
  var terry = new Employee( "O'Sherry Terry", "Marketing" );

var les = new Engineer( "Morris, Les",  new Array("JavaScript"), "indy" );

new TestCase( SECTION,
	      "pat.id",
	      5,
	      pat.id );

new TestCase( SECTION,
	      "terry.id",
	      6,
	      terry.id );

new TestCase( SECTION,
	      "les.id",
	      7,
	      les.id );


test();

