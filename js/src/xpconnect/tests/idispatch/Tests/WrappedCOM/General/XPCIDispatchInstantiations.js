









































test();

function testInstantiate(name, id, scriptable, methods)
{
    var IDISPATCH_TEXT = "[xpconnect wrapped IDispatch";
    var obj = COMObject(id);
    var value = obj.toString().substr(0, IDISPATCH_TEXT.length);
    reportCompare(
        IDISPATCH_TEXT,
        value,
        "var obj = COMObject(" + id + ");");
    try
    {
        obj = COMObject(id);
    }
    catch (e)
    {
        if (scriptable)
        {
            reportFailure("COMObject('" + id + "') generated an exception");
        }
        return;
    }
    value = obj.toString().substr(0, IDISPATCH_TEXT.length);
    reportCompare(
        scriptable ? IDISPATCH_TEXT : undefined,
        scriptable ? value : obj,
        "var obj = COMObject(" + id + ");");
}

function testEnumeration(compInfo)
{
    var obj = COMObject(compInfo.cid);
    compareObject(obj, compInfo, "Enumeration Test");
}

function test()
{
    try
    {

        printStatus("Instantiation Tests - " + objectsDesc.length + " objects to test");
        for (index = 0; index < objectsDesc.length; ++index)
        {
            compInfo = objectsDesc[index];
            printStatus("Testing " + compInfo.name);
            testInstantiate(compInfo.name, compInfo.cid, compInfo.scriptable);
            testInstantiate(compInfo.name, compInfo.progid, compInfo.scriptable);
        }
        
        var obj;
        try
        {
            obj = COMObject("dwbnonexistentobject");
            printFailure("var obj = COMObject('dwbnonexistentobject'); did not throw an exception");
        }
        catch (e)
        {
        }
        try
        {
            obj = COMObject("dwbnonexistentobject");
            printFailure("obj = COMObject('dwbnonexistentobject'); did not throw an exception");
        }
        catch (e)
        {
        }
        printStatus("Enumeration Tests - testing " + objectsDesc.length + " objects");
        for (var index = 0; index < objectsDesc.length; ++index)
        {
            printStatus("Enumerating " + objectsDesc[index].name);
            testEnumeration(objectsDesc[index]);
        }

    } 
    catch (e)
    {
        reportFailure("Unhandled exception occurred:" + e.toString());
    }
}
