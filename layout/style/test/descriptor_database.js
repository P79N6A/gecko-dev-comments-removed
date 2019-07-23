











































var gCSSFontFaceDescriptors = {
	"font-family": {
		domProp: "fontFamily",
		values: [ "\"serif\"", "\"cursive\"", "seriff", "Times New     Roman", "TimesRoman", "\"Times New Roman\"" ],
		
		invalid_values: [ "sans-serif", "Times New Roman, serif", "'Times New Roman', serif", "cursive", "fantasy" ]
	},
	"font-stretch": {
		domProp: "fontStretch",
		values: [ "normal", "ultra-condensed", "extra-condensed", "condensed", "semi-condensed", "semi-expanded", "expanded", "extra-expanded", "ultra-expanded" ],
		invalid_values: [ "wider", "narrower" ]
	},
	"font-style": {
		domProp: "fontStyle",
		values: [ "normal", "italic", "oblique" ],
		invalid_values: []
	},
	"font-weight": {
		domProp: "fontWeight",
		values: [ "normal", "400", "bold", "100", "200", "300", "500", "600", "700", "800", "900" ],
		invalid_values: [ "107", "399", "401", "699", "710", "bolder", "lighter" ]
	},
	"src": {
		domProp: null,
		values: [
			"url(404.ttf)",
			"url(\"404.eot\")",
			"url(\'404.otf\')",
			"url(404.ttf) format(\"truetype\")",
			"url(404.ttf) format(\"truetype\", \"opentype\")",
			"url(404.ttf) format(\"truetype\", \"opentype\"), url(\'404.eot\')",
			"local(Times New Roman)",
			"local(\'Times New Roman\')",
			"local(\"Times New Roman\")",
			"local(serif)", 
			"local(\"serif\")",
			"url(404.ttf) format(\"truetype\", \"unknown\"), local(Times New Roman), url(\'404.eot\')",
		],
		invalid_values: [
			"url(404.ttf) format(truetype)",
			"url(404.ttf) format(\"truetype\" \"opentype\")",
			"url(404.ttf) format(\"truetype\",)",
			"local(\"Times New\" Roman)",
		]
	},
	"unicode-range": {
		domProp: null,
		values: [ "U+0-10FFFF", "U+3-7B3", "U+3??", "U+6A", "U+3????", "U+???", "U+302-302", "U+0-7,U+A-C", "U+100-17F,U+200-17F", "U+3??, U+500-513 ,U+612 , U+4????", "U+1FFF,U+200-27F" ],
		invalid_values: [ "U+1????-2????", "U+0-7,A-C", "U+100-17F,200-17F" ]
	}
}
