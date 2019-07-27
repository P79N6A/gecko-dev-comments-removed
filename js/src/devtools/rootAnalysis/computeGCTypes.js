

"use strict";

loadRelativeToScript('utility.js');
loadRelativeToScript('annotations.js');

var annotatedGCPointers = [];

function processCSU(csu, body)
{
    if (!("DataField" in body))
        return;
    for (var field of body.DataField) {
        var type = field.Field.Type;
        var fieldName = field.Field.Name[0];
        if (type.Kind == "Pointer") {
            var target = type.Type;
            if (target.Kind == "CSU")
                addNestedPointer(csu, target.Name, fieldName);
        }
        if (type.Kind == "CSU") {
            
            
            if (type.Name == "JS::AutoGCRooter" || type.Name == "JS::CustomAutoRooter")
                return;
            addNestedStructure(csu, type.Name, fieldName);
        }
    }
    if (isGCPointer(csu))
        annotatedGCPointers.push(csu);
}

var structureParents = {}; 
var pointerParents = {}; 

function addNestedStructure(csu, inner, field)
{
    if (!(inner in structureParents))
        structureParents[inner] = [];
    structureParents[inner].push([ csu, field ]);
}

function addNestedPointer(csu, inner, field)
{
    if (!(inner in pointerParents))
        pointerParents[inner] = [];
    pointerParents[inner].push([ csu, field ]);
}

var xdb = xdbLibrary();
xdb.open("src_comp.xdb");

var minStream = xdb.min_data_stream();
var maxStream = xdb.max_data_stream();

for (var csuIndex = minStream; csuIndex <= maxStream; csuIndex++) {
    var csu = xdb.read_key(csuIndex);
    var data = xdb.read_entry(csu);
    var json = JSON.parse(data.readString());
    assert(json.length == 1);
    processCSU(csu.readString(), json[0]);

    xdb.free_string(csu);
    xdb.free_string(data);
}

var gcTypes = {}; 
var gcPointers = {}; 
var nonGCTypes = {}; 
var nonGCPointers = {}; 
var gcFields = {};




function markGCType(typeName, child, why, depth, ptrdness)
{
    
    
    
    
    
    
    if (!ptrdness && isUnsafeStorage(typeName)) {
        
        
        
        
        
        ptrdness = -1;
    }

    depth += ptrdness;
    if (depth > 2)
        return;

    if (depth == 0 && isRootedTypeName(typeName))
        return;
    if (depth == 1 && isRootedPointerTypeName(typeName))
        return;

    if (depth == 0) {
        if (typeName in nonGCTypes)
            return;
        if (!(typeName in gcTypes))
            gcTypes[typeName] = new Set();
        gcTypes[typeName].add(why);
    } else if (depth == 1) {
        if (typeName in nonGCPointers)
            return;
        if (!(typeName in gcPointers))
            gcPointers[typeName] = new Set();
        gcPointers[typeName].add(why);
    }

    if (!(typeName in gcFields))
        gcFields[typeName] = new Map();
    gcFields[typeName].set(why, [ child, ptrdness ]);

    if (typeName in structureParents) {
        for (var field of structureParents[typeName]) {
            var [ holderType, fieldName ] = field;
            markGCType(holderType, typeName, fieldName, depth, 0);
        }
    }
    if (typeName in pointerParents) {
        for (var field of pointerParents[typeName]) {
            var [ holderType, fieldName ] = field;
            markGCType(holderType, typeName, fieldName, depth, 1);
        }
    }
}

function addGCType(typeName, child, why, depth, ptrdness)
{
    markGCType(typeName, 'annotation', '<annotation>', 0, 0);
}

function addGCPointer(typeName)
{
    markGCType(typeName, 'annotation', '<pointer-annotation>', 1, 0);
}

for (var type of listNonGCTypes())
    nonGCTypes[type] = true;
for (var type of listNonGCPointers())
    nonGCPointers[type] = true;
for (var type of listGCTypes())
    addGCType(type);
for (var type of listGCPointers())
    addGCPointer(type);

for (var typeName of annotatedGCPointers)
    addGCPointer(typeName);

function explain(csu, indent, seen) {
    if (!seen)
        seen = new Set();
    seen.add(csu);
    if (!(csu in gcFields))
        return;
    if (gcFields[csu].has('<annotation>')) {
        print(indent + "which is a GCThing because I said so");
        return;
    }
    if (gcFields[csu].has('<pointer-annotation>')) {
        print(indent + "which is a GCPointer because I said so");
        return;
    }
    for (var [ field, [ child, ptrdness ] ] of gcFields[csu]) {
        var inherit = "";
        if (field == "field:0")
            inherit = " (probably via inheritance)";
        var msg = indent + "contains field '" + field + "' ";
        if (ptrdness == -1)
            msg += "(with a pointer to unsafe storage) holding a ";
        else if (ptrdness == 0)
            msg += "of type ";
        else
            msg += "pointing to type ";
        msg += child + inherit;
        print(msg);
        if (!seen.has(child))
            explain(child, indent + "  ", seen);
    }
}

for (var csu in gcTypes) {
    print("GCThing: " + csu);
    explain(csu, "  ");
}
for (var csu in gcPointers) {
    print("GCPointer: " + csu);
    explain(csu, "  ");
}
