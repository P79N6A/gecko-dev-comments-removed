
















































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
    
