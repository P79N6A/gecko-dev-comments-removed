

"use strict";

loadRelativeToScript('utility.js');
loadRelativeToScript('annotations.js');
loadRelativeToScript('CFG.js');

var subclasses = {};
var superclasses = {};
var classFunctions = {};

var fieldCallSeen = {};

function addClassEntry(index, name, other)
{
    if (!(name in index)) {
        index[name] = [other];
        return;
    }

    for (var entry of index[name]) {
        if (entry == other)
            return;
    }

    index[name].push(other);
}


function processCSU(csuName, csu)
{
    if (!("FunctionField" in csu))
        return;
    for (var field of csu.FunctionField) {
        if (1 in field.Field) {
            var superclass = field.Field[1].Type.Name;
            var subclass = field.Field[1].FieldCSU.Type.Name;
            assert(subclass == csuName);
            addClassEntry(subclasses, superclass, subclass);
            addClassEntry(superclasses, subclass, superclass);
        }
        if ("Variable" in field) {
            
            var name = field.Variable.Name[0];
            var key = csuName + ":" + field.Field[0].Name[0];
            if (!(key in classFunctions))
                classFunctions[key] = [];
            classFunctions[key].push(name);
        }
    }
}

function findVirtualFunctions(initialCSU, field, suppressed)
{
    var worklist = [initialCSU];

    
    
    
    
    while (worklist.length) {
        var csu = worklist.pop();
        if (csu == "nsISupports" && (field == "AddRef" || field == "Release")) {
            suppressed[0] = true;
            return [];
        }
        if (isOverridableField(initialCSU, csu, field))
            return null;

        if (csu in superclasses) {
            for (var superclass of superclasses[csu])
                worklist.push(superclass);
        }
    }

    var functions = [];
    var worklist = [csu];

    while (worklist.length) {
        var csu = worklist.pop();
        var key = csu + ":" + field;

        if (key in classFunctions) {
            for (var name of classFunctions[key])
                functions.push(name);
        }

        if (csu in subclasses) {
            for (var subclass of subclasses[csu])
                worklist.push(subclass);
        }
    }

    return functions;
}

var memoized = {};
var memoizedCount = 0;

function memo(name)
{
    if (!(name in memoized)) {
        memoizedCount++;
        memoized[name] = "" + memoizedCount;
        print("#" + memoizedCount + " " + name);
    }
    return memoized[name];
}

var seenCallees = null;
var seenSuppressedCallees = null;




function getCallees(edge)
{
    if (edge.Kind != "Call")
        return [];

    var callee = edge.Exp[0];
    var callees = [];
    if (callee.Kind == "Var") {
        assert(callee.Variable.Kind == "Func");
        callees.push({'kind': 'direct', 'name': callee.Variable.Name[0]});
    } else {
        assert(callee.Kind == "Drf");
        if (callee.Exp[0].Kind == "Fld") {
            var field = callee.Exp[0].Field;
            var fieldName = field.Name[0];
            var csuName = field.FieldCSU.Type.Name;
            var functions = null;
            if ("FieldInstanceFunction" in field) {
                var suppressed = [ false ];
                functions = findVirtualFunctions(csuName, fieldName, suppressed);
                if (suppressed[0]) {
                    
                    
                    callees.push({'kind': "field", 'csu': csuName, 'field': fieldName,
                                  'suppressed': true});
                }
            }
            if (functions) {
                
                
                
                
                
                
                var targets = [];
                for (var name of functions) {
                    callees.push({'kind': "direct", 'name': name});
                    targets.push({'kind': "direct", 'name': name});
                }
                callees.push({'kind': "resolved-field", 'csu': csuName, 'field': fieldName, 'callees': targets});
            } else {
                
                
                callees.push({'kind': "field", 'csu': csuName, 'field': fieldName});
            }
        } else if (callee.Exp[0].Kind == "Var") {
            
            callees.push({'kind': "indirect", 'variable': callee.Exp[0].Variable.Name[0]});
        } else {
            
            callees.push({'kind': "unknown"});
        }
    }

    return callees;
}

var lastline;
function printOnce(line)
{
    if (line != lastline) {
        print(line);
        lastline = line;
    }
}

function processBody(caller, body)
{
    if (!('PEdge' in body))
        return;

    lastline = null;
    for (var edge of body.PEdge) {
        if (edge.Kind != "Call")
            continue;
        var edgeSuppressed = false;
        var seen = seenCallees;
        if (edge.Index[0] in body.suppressed) {
            edgeSuppressed = true;
            seen = seenSuppressedCallees;
        }
        for (var callee of getCallees(edge)) {
            var prologue = (edgeSuppressed || callee.suppressed) ? "SUPPRESS_GC " : "";
            prologue += memo(caller) + " ";
            if (callee.kind == 'direct') {
                if (!(callee.name in seen)) {
                    seen[name] = true;
                    printOnce("D " + prologue + memo(callee.name));
                }
            } else if (callee.kind == 'field') {
                var { csu, field } = callee;
                printOnce("F " + prologue + "CLASS " + csu + " FIELD " + field);
            } else if (callee.kind == 'resolved-field') {
                
                
                
                
                
                
                
                var { csu, field, callees } = callee;
                var fullFieldName = csu + "." + field;
                if (!(fullFieldName in fieldCallSeen)) {
                    fieldCallSeen[fullFieldName] = true;
                    for (var target of callees)
                        printOnce("R " + memo(fullFieldName) + " " + memo(target.name));
                }
            } else if (callee.kind == 'indirect') {
                printOnce("I " + prologue + "VARIABLE " + callee.variable);
            } else if (callee.kind == 'unknown') {
                printOnce("I " + prologue + "VARIABLE UNKNOWN");
            } else {
                printErr("invalid " + callee.kind + " callee");
                debugger;
            }
        }
    }
}

var callgraph = {};

var xdb = xdbLibrary();
xdb.open("src_comp.xdb");

var minStream = xdb.min_data_stream();
var maxStream = xdb.max_data_stream();

for (var csuIndex = minStream; csuIndex <= maxStream; csuIndex++) {
    var csu = xdb.read_key(csuIndex);
    var data = xdb.read_entry(csu);
    var json = JSON.parse(data.readString());
    processCSU(csu.readString(), json[0]);

    xdb.free_string(csu);
    xdb.free_string(data);
}

xdb.open("src_body.xdb");

printErr("Finished loading data structures");

var minStream = xdb.min_data_stream();
var maxStream = xdb.max_data_stream();

for (var nameIndex = minStream; nameIndex <= maxStream; nameIndex++) {
    var name = xdb.read_key(nameIndex);
    var data = xdb.read_entry(name);
    functionBodies = JSON.parse(data.readString());
    for (var body of functionBodies)
        body.suppressed = [];
    for (var body of functionBodies) {
        for (var [pbody, id] of allRAIIGuardedCallPoints(body, isSuppressConstructor))
            pbody.suppressed[id] = true;
    }

    seenCallees = {};
    seenSuppressedCallees = {};

    var functionName = name.readString();
    for (var body of functionBodies)
        processBody(functionName, body);

    
    
    
    
    
    
    
    
    
    
    var markerPos = functionName.indexOf(internalMarker);
    if (markerPos > 0) {
        var inChargeXTor = functionName.replace(internalMarker, "");
        print("D " + memo(inChargeXTor) + " " + memo(functionName));

        
        
        
        
        
        
        if (functionName.indexOf("::~") > 0) {
            var calledDestructor = inChargeXTor.replace("(int32)", "()");
            print("D " + memo(calledDestructor) + " " + memo(inChargeXTor));
        }
    }

    xdb.free_string(name);
    xdb.free_string(data);
}
