









function DumpDOM(node)
{
	dump("--------------------- DumpDOM ---------------------\n");
	
	DumpNodeAndChildren(node, "");
	
	dump("------------------- End DumpDOM -------------------\n");
}



function DumpNodeAndChildren(node, prefix)
{
	dump(prefix + "<" + node.nodeName);

	var attributes = node.attributes;
	
	if ( attributes && attributes.length )
	{
		var item, name, value;
		
		for ( var index = 0; index < attributes.length; index++ )
		{
			item = attributes.item(index);
			name = item.nodeName;
			value = item.nodeValue;
			
			if ( (name == 'lazycontent' && value == 'true') ||
				 (name == 'xulcontentsgenerated' && value == 'true') ||
				 (name == 'id') ||
				 (name == 'instanceOf') )
			{
				
			}
			else
			{
				dump(" " + name + "=\"" + value + "\"");
			}
		}
	}
	
	if ( node.nodeType == 1 )
	{
		
		var text = node.getAttribute('id');
		if ( text && text[0] != '$' )
			dump(" id=\"" + text + "\"");
	}
	
	if ( node.nodeType == Node.TEXT_NODE )
		dump(" = \"" + node.data + "\"");
	
	dump(">\n");
	
	
	if ( node.nodeName == "IFRAME" || node.nodeName == "FRAME" )
	{
		if ( node.name )
		{
			var wind = top.frames[node.name];
			if ( wind && wind.document && wind.document.documentElement )
			{
				dump(prefix + "----------- " + node.nodeName + " -----------\n");
				DumpNodeAndChildren(wind.document.documentElement, prefix + "  ");
				dump(prefix + "--------- End " + node.nodeName + " ---------\n");
			}
		}
	}
	
	else if ( node.childNodes )
	{
		for ( var child = 0; child < node.childNodes.length; child++ )
			DumpNodeAndChildren(node.childNodes[child], prefix + "  ");
	} 
}
