

"use strict";

loadRelativeToScript('utility.js');
loadRelativeToScript('annotations.js');

function processCSU(csu, body)
{
    if (!("DataField" in body))
        return;
    for (var field of body.DataField) {
        var type = field.Field.Type;
        if (type.Kind == "Pointer") {
            var target = type.Type;
            if (target.Kind == "CSU")
                addNestedPointer(csu, target.Name);
        }
        if (type.Kind == "CSU") {
            
            
            if (type.Name == "JS::AutoGCRooter" || type.Name == "JS::CustomAutoRooter")
                return;
            addNestedStructure(csu, type.Name);
        }
    }
}

function addNestedStructure(csu, inner)
{
    if (!(inner in structureParents))
        structureParents[inner] = [];
    structureParents[inner].push(csu);
}

function addNestedPointer(csu, inner)
{
    if (!(inner in pointerParents))
        pointerParents[inner] = [];
    pointerParents[inner].push(csu);
}

var structureParents = {};
var pointerParents = {};

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

function addGCType(name)
{
    if (isRootedTypeName(name))
        return;

    print("GCThing: " + name);
    if (name in structureParents) {
        for (var holder of structureParents[name])
            addGCType(holder);
    }
    if (name in pointerParents) {
        for (var holder of pointerParents[name])
            addGCPointer(holder);
    }
}

function addGCPointer(name)
{
    
    if (isRootedPointerTypeName(name))
        return;

    print("GCPointer: " + name);
    if (name in structureParents) {
        for (var holder of structureParents[name])
            addGCPointer(holder);
    }
}

addGCType('js::ObjectImpl');
addGCType('JSString');
addGCType('js::Shape');
addGCType('js::BaseShape');
addGCType('JSScript');
addGCType('js::LazyScript');
addGCType('js::ion::IonCode');
addGCPointer('JS::Value');
addGCPointer('jsid');
