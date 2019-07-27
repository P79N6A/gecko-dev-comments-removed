
var embeddedElements = {
	img: {
		
		alt: "string",
		src: "url",
		srcset: "string",
		crossOrigin: {type: "enum", keywords: ["", "anonymous", "use-credentials"]},
		useMap: "string",
		isMap: "boolean",
		

		
		name: "string",
		lowsrc: {type: "url"},
		align: "string",
		hspace: "unsigned long",
		vspace: "unsigned long",
		longDesc: "url",
		border: {type: "string", treatNullAsEmptyString: true},
	},
	iframe: {
		
		src: "url",
		srcdoc: "string",
		name: "string",
		sandbox: "settable tokenlist",
		seamless: "boolean",
		allowFullscreen: "boolean",
		width: "string",
		height: "string",

		
		align: "string",
		scrolling: "string",
		frameBorder: "string",
		longDesc: "url",
		marginHeight: {type: "string", treatNullAsEmptyString: true},
		marginWidth: {type: "string", treatNullAsEmptyString: true}
	},
	embed: {
		
		src: "url",
		type: "string",
		width: "string",
		height: "string",

		
		align: "string",
		name: "string"
	},
	object: {
		
		data: "url",
		type: "string",
		typeMustMatch: "boolean",
		name: "string",
		useMap: "string",
		width: "string",
		height: "string",

		
		align: "string",
		archive: "string",
		code: "string",
		declare: "boolean",
		hspace: "unsigned long",
		standby: "string",
		vspace: "unsigned long",
		codeBase: "url",
		codeType: "string",
		border: {type: "string", treatNullAsEmptyString: true}
	},
	param: {
		
		name: "string",
		value: "string",

		
		type: "string",
		valueType: "string"
	},
	video: {
		
		src: "url",
		crossOrigin: {type: "enum", keywords: ["anonymous", "use-credentials"], nonCanon:{"": "anonymous"}},
		
		preload: {type: "enum", keywords: ["none", "metadata", "auto"], nonCanon: {"": "auto"}, defaultVal: null},
		autoplay: "boolean",
		loop: "boolean",
		mediaGroup: "string",
		controls: "boolean",
		defaultMuted: {type: "boolean", domAttrName: "muted"},

		width: "unsigned long",
		height: "unsigned long",
		poster: "url"
	},
	audio: {
		
		src: "url",
		crossOrigin: {type: "enum", keywords: ["anonymous", "use-credentials"], nonCanon:{"": "anonymous"}},
		
		preload: {type: "enum", keywords: ["none", "metadata", "auto"], nonCanon: {"": "auto"}, defaultVal: null},
		autoplay: "boolean",
		loop: "boolean",
		mediaGroup: "string",
		controls: "boolean",
		defaultMuted: {type: "boolean", domAttrName: "muted"}
	},
	source: {
		src: "url",
		type: "string",
		media: "string"
	},
	track: {
		kind: {type: "enum", keywords: ["subtitles", "captions", "descriptions", "chapters", "metadata"], defaultVal: "captions"},
		src: "url",
		srclang: "string",
		label: "string",
		"default": "boolean"
	},
	canvas: {
		width: {type: "unsigned long", defaultVal: 300},
		height: {type: "unsigned long", defaultVal: 150}
	},
	map: {
		name: "string"
	},
	area: {
		
		alt: "string",
		coords: "string",
		shape: "string",
		target: "string",
		download: "string",
		ping: "urls",
		rel: "string",
		relList: {type: "tokenlist", domAttrName: "rel"},
		hreflang: "string",
		type: "string",

	        
		href: "url",

		
		noHref: "boolean"
	},
};

mergeElements(embeddedElements);
