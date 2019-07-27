
var groupingElements = {
	p: {
		
		align: "string",
	},
	hr: {
		
		align: "string",
		color: "string",
		noShade: "boolean",
		size: "string",
		width: "string",
	},
	pre: {
		
		width: "long",
	},
	blockquote: {
		cite: "url",
	},
	ol: {
		
		reversed: "boolean",
		
		
		start: {type: "long", defaultVal: 1},
		type: "string",

		
		compact: "boolean",
	},
	ul: {
		
		compact: "boolean",
		type: "string",
	},
	li: {
		
		value: "long",

		
		type: "string",
	},
	dl: {
		
		compact: "boolean",
	},
	dt: {},
	dd: {},
	figure: {},
	figcaption: {},
	main: {},
	div: {
		
		align: "string",
	},
};

mergeElements(groupingElements);
