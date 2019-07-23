











































StartTest( "Get interfaces by ID" );
SetupTest();
AddTestData();
StopTest();

function SetupTest() {
}

function AddTestData() {
	

	for ( p in Components.interfaces ) {
		AddTestCase(
			"Components.interfaces[" +p +"].equals( Components.ID(" +
			Components.interfaces[p].id +"))",
			true,
			Components.interfaces[p].equals(
				Components.ID(Components.interfaces[p].id))
		);
		AddTestCase(
			"Components.ID(" +Components.interfaces[p].id 
			+").equals( Components.interfaces["+p+"])",
			true,
			Components.interfaces[p].equals(
				Components.ID(Components.interfaces[p].id))
		);
	}
if (0) {
	
	AddTestCase(
		"Components.interfaces[0].equals( Components.ID("+
			Components.interfaces[1].id +") )",
		false,
		Components.interfaces[0].equals( 
			Components.ID(Components.interfaces[1].id) )
	);
}
}