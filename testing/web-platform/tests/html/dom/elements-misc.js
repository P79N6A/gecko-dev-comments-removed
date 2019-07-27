
var miscElements = {
	
	html: {
		
		version: "string",
	},

	
	script: {
		src: "url",
		type: "string",
		charset: "string",
		
		defer: "boolean",
		crossOrigin: {type: "enum", keywords: ["anonymous", "use-credentials"], nonCanon:{"": "anonymous"}},
	},
	noscript: {},

	
	ins: {
		cite: "url",
		dateTime: "string",
	},
	del: {
		cite: "url",
		dateTime: "string",
	},

	
	details: {
		open: "boolean",
	},
	summary: {},
	menu: {
		
		
		
		label: "string",

		
		compact: "boolean",
	},
	menuitem: {
		type: {type: "enum", keywords: ["command", "checkbox", "radio"], defaultVal: "command"},
		label: "string",
		icon: "url",
		disabled: "boolean",
		checked: "boolean",
		radiogroup: "string",
		"default": "boolean",
	},

	
	undefinedelement: {},
};

mergeElements(miscElements);
