

"use strict";

loadRelativeToScript('utility.js');
























var readableNames = {}; 
var mangledName = {}; 
var calleeGraph = {}; 
var callerGraph = {}; 
var gcFunctions = {}; 
var suppressedFunctions = {}; 
var gcEdges = {};

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


var idToMangled = [""];

function loadCallgraph(file)
{
    var suppressedFieldCalls = {};
    var resolvedFunctions = {};

    var textLines = snarf(file).split('\n');
    for (var line of textLines) {
        var match;
        if (match = line.charAt(0) == "#" && /^\#(\d+) (.*)/.exec(line)) {
            assert(functionNames.length == match[1]);
            functionNames.push(match[2]);
            var [ mangled, readable ] = splitFunction(match[2]);
            if (mangled in readableNames)
                readableNames[mangled].push(readable);
            else
                readableNames[mangled] = [ readable ];
            mangledName[readable] = mangled;
            idToMangled.push(mangled);
            continue;
        }
        var suppressed = false;
        if (line.indexOf("SUPPRESS_GC") != -1) {
            match = /^(..)SUPPRESS_GC (.*)/.exec(line);
            line = match[1] + match[2];
            suppressed = true;
        }
        var tag = line.charAt(0);
        if (match = tag == 'I' && /^I (\d+) VARIABLE ([^\,]*)/.exec(line)) {
            var mangledCaller = idToMangled[match[1]];
            var name = match[2];
            if (!indirectCallCannotGC(functionNames[match[1]], name) && !suppressed)
                addGCFunction(mangledCaller, "IndirectCall: " + name);
        } else if (match = tag == 'F' && /^F (\d+) CLASS (.*?) FIELD (.*)/.exec(line)) {
            var caller = idToMangled[match[1]];
            var csu = match[2];
            var fullfield = csu + "." + match[3];
            if (suppressed)
                suppressedFieldCalls[fullfield] = true;
            else if (!fieldCallCannotGC(csu, fullfield))
                addGCFunction(caller, "FieldCall: " + fullfield);
        } else if (match = tag == 'D' && /^D (\d+) (\d+)/.exec(line)) {
            var caller = idToMangled[match[1]];
            var callee = idToMangled[match[2]];
            addCallEdge(caller, callee, suppressed);
        } else if (match = tag == 'R' && /^R (\d+) (\d+)/.exec(line)) {
            var callerField = idToMangled[match[1]];
            var callee = idToMangled[match[2]];
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

    
    
    
    var top = worklist.length;
    while (top > 0) {
        name = worklist[--top];
        if (!(name in suppressedFunctions))
            continue;
        delete suppressedFunctions[name];
        if (!(name in calleeGraph))
            continue;
        for (var entry of calleeGraph[name]) {
            if (!entry.suppressed)
                worklist[top++] = entry.callee;
        }
    }

    
    for (var name in gcFunctions) {
        if (name in suppressedFunctions)
            delete gcFunctions[name];
    }

    for (var name in suppressedFieldCalls) {
        suppressedFunctions[name] = true;
    }

    for (var gcName of [ 'void js::gc::GCRuntime::collect(uint8, int64, uint32, uint32)',
                         'void js::gc::GCRuntime::minorGC(uint32)',
                         'void js::gc::GCRuntime::minorGC(uint32)' ])
    {
        assert(gcName in mangledName, "GC function not found: " + gcName);
        addGCFunction(mangledName[gcName], "GC");
    }

    
    var worklist = [];
    for (var name in gcFunctions)
        worklist.push(name);

    
    while (worklist.length) {
        name = worklist.shift();
        assert(name in gcFunctions);
        if (!(name in callerGraph))
            continue;
        for (var entry of callerGraph[name]) {
            if (!entry.suppressed && addGCFunction(entry.caller, name))
                worklist.push(entry.caller);
        }
    }

    
    
    for (var name in resolvedFunctions) {
        if (!(name in gcFunctions))
            suppressedFunctions[name] = true;
    }
}
