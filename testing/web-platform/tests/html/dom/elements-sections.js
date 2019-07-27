
var sectionElements = {
	body: {
		
		text: {type: "string", treatNullAsEmptyString: true},
		link: {type: "string", treatNullAsEmptyString: true},
		vLink: {type: "string", treatNullAsEmptyString: true},
		aLink: {type: "string", treatNullAsEmptyString: true},
		bgColor: {type: "string", treatNullAsEmptyString: true},
		background: "string",
	},
	article: {},
	section: {},
	nav: {},
	aside: {},
	h1: {
		
		align: "string",
	},
	h2: {
		
		align: "string",
	},
	h3: {
		
		align: "string",
	},
	h4: {
		
		align: "string",
	},
	h5: {
		
		align: "string",
	},
	h6: {
		
		align: "string",
	},
	hgroup: {},
	header: {},
	footer: {},
	address: {},
};

mergeElements(sectionElements);

extraTests.push(function() {
	ReflectionTests.reflects({type: "enum", keywords: ["ltr", "rtl", "auto"]}, "dir", document, "dir", document.documentElement);
	
	
	ReflectionTests.reflects({type: "string", treatNullAsEmptyString: true}, "fgColor", document, "text", document.body);
	ReflectionTests.reflects({type: "string", treatNullAsEmptyString: true}, "linkColor", document, "link", document.body);
	ReflectionTests.reflects({type: "string", treatNullAsEmptyString: true}, "vlinkColor", document, "vlink", document.body);
	ReflectionTests.reflects({type: "string", treatNullAsEmptyString: true}, "alinkColor", document, "alink", document.body);
	ReflectionTests.reflects({type: "string", treatNullAsEmptyString: true}, "bgColor", document, "bgcolor", document.body);
	
	document.documentElement.removeAttribute("dir");
	var attrs = ["text", "bgcolor", "link", "alink", "vlink"];
	for (var i = 0; i < attrs.length; i++) {
		document.body.removeAttribute(attrs[i]);
	}
});
