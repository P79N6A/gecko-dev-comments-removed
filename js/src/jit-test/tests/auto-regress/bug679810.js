



function WorkerBee(name, dept, projs) {
    this.projects &= projs || new Array();
}
function Engineer(name, projs, machine) {
    this.base = WorkerBee;
    this.base(name, "engineering", projs)
    this.machine = machine || "";
}
Engineer.prototype = {};
var les = new Engineer("Morris, Les", new Array("JavaScript"), "indy");
