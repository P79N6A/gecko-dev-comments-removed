
var tabularElements = {
	table: {
		
		sortable: "boolean",

		
		align: "string",
		border: "string",
		frame: "string",
		rules: "string",
		summary: "string",
		width: "string",
		bgColor: {type: "string", treatNullAsEmptyString: true},
		cellPadding: {type: "string", treatNullAsEmptyString: true},
		cellSpacing: {type: "string", treatNullAsEmptyString: true},
	},
	caption: {
		
		align: "string",
	},
	colgroup: {
		span: "limited unsigned long",

		
		align: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		vAlign: "string",
		width: "string",
	},
	col: {
		
		span: "limited unsigned long",

		
		align: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		vAlign: "string",
		width: "string",
	},
	tbody: {
		
		align: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		vAlign: "string",
	},
	thead: {
		
		align: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		vAlign: "string",
	},
	tfoot: {
		
		align: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		vAlign: "string",
	},
	tr: {
		
		align: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		vAlign: "string",
		bgColor: {type: "string", treatNullAsEmptyString: true},
	},
	td: {
		
		colSpan: {type: "unsigned long", defaultVal: 1},
		rowSpan: {type: "unsigned long", defaultVal: 1},
		headers: "settable tokenlist",

		
		align: "string",
		axis: "string",
		height: "string",
		width: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		noWrap: "boolean",
		vAlign: "string",
		bgColor: {type: "string", treatNullAsEmptyString: true},

		
		abbr: "string",
	},
	th: {
		
		colSpan: {type: "unsigned long", defaultVal: 1},
		rowSpan: {type: "unsigned long", defaultVal: 1},
		headers: "settable tokenlist",

		
		align: "string",
		axis: "string",
		height: "string",
		width: "string",
		ch: {type: "string", domAttrName: "char"},
		chOff: {type: "string", domAttrName: "charoff"},
		noWrap: "boolean",
		vAlign: "string",
		bgColor: {type: "string", treatNullAsEmptyString: true},

		
		
		
		
		
		scope: {type: "enum", keywords: ["row", "col", "rowgroup", "colgroup"]},
		abbr: "string",
		sorted: "string",
	},
};

mergeElements(tabularElements);
