
















































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


f = fontforge.font()
n = "MarkXMark2Y"
f.fontname = n
f.familyname = n
f.fullname = n
f.copyright = "Copyright (c) 2008 Mozilla Corporation"

g = f.createChar(ord("X"), "X")
g.importOutlines("mark-glyph.svg")
g.width = 1500

g = f.createChar(ord("Y"), "Y")
g.importOutlines("mark2-glyph.svg")
g.width = 1800

f.generate("markXmark2Y.ttf")

