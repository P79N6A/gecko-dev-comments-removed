








































test();

function test()
{
	printStatus("Stress testing");	
	for (index = 0; index < 10000; ++index)
	{
		for (x = 0; x < objectsDesc.length; ++x)
		{
			var obj = COMObject(objectsDesc[x].cid);
			for (prop in obj)
			{
				print(index + ":" +objectsDesc[x].name + ":" + prop);
			}
		}
	}
}
