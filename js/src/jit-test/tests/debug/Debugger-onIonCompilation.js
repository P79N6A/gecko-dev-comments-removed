
function test() {
    
    for (var res = false; !res; res = inIon()) {};
    if (typeof res == "string")
        throw "Skipping test: Ion compilation is disabled/prevented.";
};


try {
    test();
} catch (x) {
    if (typeof x == "string")
        quit();
}



function assertInstruction(ins) {
    assertEq(typeof ins.id, "number");
    assertEq(ins.id | 0, ins.id);
    assertEq(typeof ins.opcode, "string");
}

function assertBlock(block) {
    assertEq(typeof block, "object");
    assertEq(typeof block.number, "number");
    assertEq(block.number | 0, block.number);
    assertEq(typeof block.instructions, "object");
    for (var ins of block.instructions)
        assertInstruction(ins);
}

function assertGraph(graph, scripts) {
    assertEq(typeof graph, "object");
    assertEq(typeof graph.blocks, "object");
    assertEq(graph.blocks.length, scripts.length);
    for (var block of graph.blocks)
        assertBlock(block);
}

function assertJSON(str, scripts) {
    assertEq(typeof str, "string");

    var json = JSON.parse(str);
    assertGraph(json.mir, scripts);
    assertGraph(json.lir, scripts);
}

function assertOnIonCompilationArgument(obj) {
    assertEq(typeof obj, "object");
    assertEq(typeof obj.scripts, "object");
    assertJSON(obj.json, obj.scripts);
}


var hits = 0;
var g = newGlobal();
g.parent = this;
g.eval(`
  var dbg = new Debugger();
  var parentw = dbg.addDebuggee(parent);
  var testw = parentw.makeDebuggeeValue(parent.test);
  var scriptw = testw.script;
`);


function check() {
  
  with ({}) {       
    gc();           
    hits = 0;       
    test();         
  }
}


g.eval(`
  dbg.onIonCompilation = function (graph) {
    // print('Compiled ' + graph.scripts[0].displayName + ':' + graph.scripts[0].startLine);
    if (graph.scripts[0] !== scriptw)
      return;
    parent.assertOnIonCompilationArgument(graph);
    parent.hits++;
  };
`);
check();

assertEq(hits >= 1, true);



g.dbg.onIonCompilation = function (graph) {
  
  if (graph.scripts[0] !== g.scriptw)
    return;
  assertOnIonCompilationArgument(graph);
  hits++;
};
check();
assertEq(hits >= 1, true);


g.eval(`
  dbg.enabled = false;
  dbg.onIonCompilation = function (graph) {
    parent.hits++;
  };
`);
check();
assertEq(hits, 0);

g.dbg.enabled = false;
g.dbg.onIonCompilation = function (graph) {
  hits++;
};
check();
assertEq(hits, 0);
