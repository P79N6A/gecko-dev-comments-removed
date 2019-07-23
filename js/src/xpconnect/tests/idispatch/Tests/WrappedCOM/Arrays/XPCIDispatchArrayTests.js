









































test();

function test()
{
    printStatus("Testing arrays");  
    var obj = COMObject(CLSID_nsXPCDispTestArrays);
    var anArray = [ 0, 1, 2, 3];
    obj.TakesSafeArray(anArray);
    var strArray = [ "0", "1", "2", "3"];
    obj.TakesSafeArrayBSTR(strArray);
	return 0;
}
