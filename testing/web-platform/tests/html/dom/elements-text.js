
var textElements = {
	a: {
		
		target: "string",
		download: "string",
		ping: "urls",
		rel: "string",
		relList: {type: "tokenlist", domAttrName: "rel"},
		hreflang: "string",
		type: "string",

		
		href: "url",

		
		coords: "string",
		charset: "string",
		name: "string",
		rev: "string",
		shape: "string",
	},
	em: {},
	strong: {},
	small: {},
	s: {},
	cite: {},
	q: {
		cite: "url",
	},
	dfn: {},
	abbr: {},
	data: {
		value: "string",
	},
	time: {
		dateTime: "string",
	},
	code: {},
	
	
	"var": {},
	samp: {},
	kbd: {},
	sub: {},
	sup: {},
	i: {},
	b: {},
	u: {},
	mark: {},
	ruby: {},
	rt: {},
	rp: {},
	bdi: {},
	bdo: {},
	span: {},
	br: {
		
		clear: "string",
	},
	wbr: {},
};

mergeElements(textElements);
