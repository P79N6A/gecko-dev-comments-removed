
var metadataElements = {
	head: {},
	title: {},
	base: {
		
		target: "string",
	},
	link: {
		
		href: "url",
		crossOrigin: {type: "enum", keywords: ["anonymous", "use-credentials"], nonCanon:{"": "anonymous"}},
		rel: "string",
		relList: {type: "tokenlist", domAttrName: "rel"},
		media: "string",
		hreflang: "string",
		type: "string",
		sizes: "settable tokenlist",

		
		charset: "string",
		rev: "string",
		target: "string",
	},
	meta: {
		
		name: "string",
		httpEquiv: {type: "string", domAttrName: "http-equiv"},
		content: "string",

		
		scheme: "string",
	},
	style: {
		media: "string",
		type: "string",
		scoped: "boolean",
	},
};

mergeElements(metadataElements);
