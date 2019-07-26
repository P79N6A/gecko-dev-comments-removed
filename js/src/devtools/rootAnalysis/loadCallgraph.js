

"use strict";

var calleeGraph = {};
var callerGraph = {};
var gcFunctions = {};
var gcEdges = {};
var suppressedFunctions = {};

function addGCFunction(caller, reason)
{
    if (caller in suppressedFunctions)
        return false;

    if (ignoreGCFunction(caller))
        return false;

    if (!(caller in gcFunctions)) {
        gcFunctions[caller] = reason;
        return true;
    }

    return false;
}

function addCallEdge(caller, callee, suppressed)
{
    if (!(caller in calleeGraph))
        calleeGraph[caller] = [];
    calleeGraph[caller].push({callee:callee, suppressed:suppressed});

    if (!(callee in callerGraph))
        callerGraph[callee] = [];
    callerGraph[callee].push({caller:caller, suppressed:suppressed});
}

var functionNames = [""];

function loadCallgraph(file)
{
    var suppressedFieldCalls = {};
    var resolvedFunctions = {};

    var textLines = snarf(file).split('\n');
    for (var line of textLines) {
        var match;
        if (match = /^\#(\d+) (.*)/.exec(line)) {
            assert(functionNames.length == match[1]);
            functionNames.push(match[2]);
            continue;
        }
        var suppressed = false;
        if (/SUPPRESS_GC/.test(line)) {
            match = /^(..)SUPPRESS_GC (.*)/.exec(line);
            line = match[1] + match[2];
            suppressed = true;
        }
        if (match = /^I (\d+) VARIABLE ([^\,]*)/.exec(line)) {
            var caller = functionNames[match[1]];
            var name = match[2];
            if (!indirectCallCannotGC(caller, name) && !suppressed)
                addGCFunction(caller, "IndirectCall: " + name);
        } else if (match = /^F (\d+) CLASS (.*?) FIELD (.*)/.exec(line)) {
            var caller = functionNames[match[1]];
            var csu = match[2];
            var fullfield = csu + "." + match[3];
            if (suppressed)
                suppressedFieldCalls[fullfield] = true;
            else if (!fieldCallCannotGC(csu, fullfield))
                addGCFunction(caller, "FieldCall: " + fullfield);
        } else if (match = /^D (\d+) (\d+)/.exec(line)) {
            var caller = functionNames[match[1]];
            var callee = functionNames[match[2]];
            addCallEdge(caller, callee, suppressed);
        } else if (match = /^R (\d+) (\d+)/.exec(line)) {
            var callerField = functionNames[match[1]];
            var callee = functionNames[match[2]];
            addCallEdge(callerField, callee, false);
            resolvedFunctions[callerField] = true;
        }
    }

    
    
    var worklist = [];
    for (var callee in callerGraph)
        suppressedFunctions[callee] = true;
    for (var caller in calleeGraph) {
        if (!(caller in callerGraph)) {
            suppressedFunctions[caller] = true;
            worklist.push(caller);
        }
    }

    
    
    
    while (worklist.length) {
        name = worklist.pop();
        if (shouldSuppressGC(name))
            continue;
        if (!(name in suppressedFunctions))
            continue;
        delete suppressedFunctions[name];
        if (!(name in calleeGraph))
            continue;
        for (var entry of calleeGraph[name]) {
            if (!entry.suppressed)
                worklist.push(entry.callee);
        }
    }

    
    for (var name in gcFunctions) {
        if (name in suppressedFunctions)
            delete gcFunctions[name];
    }

    for (var name in suppressedFieldCalls) {
        suppressedFunctions[name] = true;
    }

    for (var gcName of [ 'jsgc.cpp:void Collect(JSRuntime*, uint8, int64, uint32, uint32)',
                         'void js::MinorGC(JSRuntime*, uint32)' ])
    {
        assert(gcName in callerGraph);
        addGCFunction(gcName, "GC");
    }

    
    var worklist = [];
    for (var name in gcFunctions)
        worklist.push(name);

    
    while (worklist.length) {
        name = worklist.pop();
        assert(name in gcFunctions);
        if (!(name in callerGraph))
            continue;
        for (var entry of callerGraph[name]) {
            if (!entry.suppressed && addGCFunction(entry.caller, name))
                worklist.push(entry.caller);
        }
    }

    
    
    for (var name in resolvedFunctions) {
        if (!(name in gcFunctions)) {
            suppressedFunctions[name] = true;
            printErr("Adding " + name);
        }
    }
}
