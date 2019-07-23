




































var s;
for (s = 0; s < document.styleSheets.length; s++) {
   var sheet = document.styleSheets[s];
   dump("Style sheet #" + (s+1) + ": " + sheet.title + "\n");
   var i, r;
   dump("\n");
   for (i = 0; i < sheet.imports.length; i++) {
      dump("@import url(" + sheet.imports[i].href + ");\n");
   }
   dump("\n");
   for (r = 0; r < sheet.rules.length; r++) {
     var rule = sheet.rules[r];
     dump(rule.selectorText + "  {" + "\n");
     var style = rule.style;
     var p;
     for (p = 0; p < style.length; p++) {
	dump("    " + style[p] + ":" + style.getPropertyValue(style[p]) + ";\n");
     }
     dump("    }\n");
   }
   dump("\n");
} 