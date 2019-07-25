
















































import fontforge



for codepoint in range(ord("A"), ord("D") + 1):
    for (mark, width) in [("", 1500), ("2", 1800)]:
        charname = chr(codepoint)
        f = fontforge.font()
        n = "Mark" + mark + charname
        f.fontname = n
        f.familyname = n
        f.fullname = n
        f.copyright = "Copyright (c) 2008 Mozilla Corporation"

        g = f.createChar(codepoint, charname)
        g.importOutlines("mark" + mark + "-glyph.svg")
        g.width = width

        f.generate("mark" + mark + charname + ".ttf")
        f.generate("mark" + mark + charname + ".otf")


for codepoint in range(ord("A"), ord("A") + 1):
    for (mark, width) in [("", 1500), ("2", 1800)]:
        for (uposname, upos) in [("low", -350), ("high", -50)]:
            charname = chr(codepoint)
            f = fontforge.font()
            n = "Mark" + mark + charname
            f.fontname = n
            f.familyname = n
            f.fullname = n
            f.descent = 400
            f.upos = upos
            f.uwidth = 100
            f.copyright = "Copyright (c) 2008 Mozilla Corporation"
    
            g = f.createChar(codepoint, charname)
            g.importOutlines("mark" + mark + "-glyph.svg")
            g.width = width
    
            f.generate("mark" + mark + charname + "-" + uposname +
                       "underline.ttf")
    
f = fontforge.font()
n = "MarkAB-spaceliga"
f.fontname = n
f.familyname = n
f.fullname = n
f.copyright = "Copyright (c) 2008-2011 Mozilla Corporation"

g = f.createChar(ord(" "), "space")
g.width = 1000
for charname in ["A", "B"]:
    g = f.createChar(ord(charname), charname)
    g.importOutlines("mark-glyph.svg")
    g.width = 1500

f.addLookup("liga-table", "gsub_ligature", (), (("liga",(("latn",("dflt")),)),))
f.addLookupSubtable("liga-table", "liga-subtable")
g = f.createChar(-1, "spaceA")
g.glyphclass = "baseligature";
g.addPosSub("liga-subtable", ("space", "A"))
g.importOutlines("mark2-glyph.svg")
g.width = 1800

f.generate("markAB-spaceliga.otf")
