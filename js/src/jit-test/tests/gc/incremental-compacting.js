


if (!("gcstate" in this && "gczeal" in this))
    quit();

gczeal(0);

function testCompacting(zoneCount, objectCount, sliceCount)
{
    
    
    

    var zones = [];
    for (var i = 0; i < zoneCount; i++) {
        var zone = newGlobal();
        evaluate("var objects; " +
                 "function makeObjectGraph(objectCount) { " +
                 "    objects = []; " +
                 "    for (var i = 0; i < objectCount; i++) " +
                 "        objects.push({ serial: i }); " +
                "}",
                 { global: zone });
        zone.makeObjectGraph(objectCount);
        zones.push(zone);
    }

    
    if (gcstate() !== "none")
        gc();

    startgc(sliceCount, "shrinking");
    while (gcstate() !== "none") {
        gcslice(sliceCount);
    }

    return zones;
}

testCompacting(1, 100000, 100000);
testCompacting(2, 100000, 100000);
testCompacting(4, 50000,  100000);
testCompacting(2, 100000, 50000);
