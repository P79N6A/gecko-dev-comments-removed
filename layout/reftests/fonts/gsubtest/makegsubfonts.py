
import os
import textwrap
from xml.etree import ElementTree
from fontTools.ttLib import TTFont, newTable
from fontTools.misc.psCharStrings import T2CharString
from fontTools.ttLib.tables.otTables import GSUB,\
    ScriptList, ScriptRecord, Script, DefaultLangSys,\
    FeatureList, FeatureRecord, Feature,\
    LookupList, Lookup, AlternateSubst, SingleSubst


directory = os.path.dirname(__file__)
shellSourcePath = os.path.join(directory, "gsubtest-shell.ttx")
shellTempPath = os.path.join(directory, "gsubtest-shell.otf")
featureList = os.path.join(directory, "gsubtest-features.txt")
javascriptData = os.path.join(directory, "gsubtest-features.js")
outputPath = os.path.join(os.path.dirname(directory), "gsubtest-lookup%d")

baseCodepoint = 0xe000





f = open(featureList, "rb")
text = f.read()
f.close()
mapping = []
for line in text.splitlines():
    line = line.strip()
    if not line:
        continue
    if line.startswith("#"):
        continue
    
    values = line.split("\t")
    tag = values.pop(0)
    mapping.append(tag);





def addGlyphToCFF(glyphName=None, program=None, private=None, globalSubrs=None, charStringsIndex=None, topDict=None, charStrings=None):
    charString = T2CharString(program=program, private=private, globalSubrs=globalSubrs)
    charStringsIndex.append(charString)
    glyphID = len(topDict.charset)
    charStrings.charStrings[glyphName] = glyphID
    topDict.charset.append(glyphName)

def makeLookup1():
    
    f = open(shellSourcePath)
    ttxData = f.read()
    f.close()
    ttxData = ttxData.replace("__familyName__", "gsubtest-lookup1")
    tempShellSourcePath = shellSourcePath + ".temp"
    f = open(tempShellSourcePath, "wb")
    f.write(ttxData)
    f.close()
    
    
    shell = TTFont(sfntVersion="OTTO")
    shell.importXML(tempShellSourcePath)
    shell.save(shellTempPath)
    os.remove(tempShellSourcePath)
    
    
    shell = TTFont(shellTempPath)
    
    
    hmtx = shell["hmtx"]
    glyphSet = shell.getGlyphSet()
    
    failGlyph = glyphSet["F"]
    failGlyph.decompile()
    failGlyphProgram = list(failGlyph.program)
    failGlyphMetrics = hmtx["F"]
    
    passGlyph = glyphSet["P"]
    passGlyph.decompile()
    passGlyphProgram = list(passGlyph.program)
    passGlyphMetrics = hmtx["P"]
    
    
    hmtx = shell["hmtx"]
    cmap = shell["cmap"]
    
    
    existingGlyphs = [".notdef", "space", "F", "P"]
    glyphOrder = list(existingGlyphs)
    
    
    cff = shell["CFF "].cff
    globalSubrs = cff.GlobalSubrs
    topDict = cff.topDictIndex[0]
    topDict.charset = existingGlyphs
    private = topDict.Private
    charStrings = topDict.CharStrings
    charStringsIndex = charStrings.charStringsIndex
    
    features = sorted(mapping)

    
    cp = baseCodepoint
    for index, tag in enumerate(features):
    
    	
    	glyphName = "%s.pass" % tag
    	glyphOrder.append(glyphName)
    	addGlyphToCFF(
    		glyphName=glyphName,
    		program=passGlyphProgram,
    		private=private,
    		globalSubrs=globalSubrs,
    		charStringsIndex=charStringsIndex,
    		topDict=topDict,
            charStrings=charStrings
    	)
    	hmtx[glyphName] = passGlyphMetrics
     
    	for table in cmap.tables:
    		if table.format == 4:
    			table.cmap[cp] = glyphName
    		else:
    			raise NotImplementedError, "Unsupported cmap table format: %d" % table.format
    	cp += 1
    
    	
    	glyphName = "%s.fail" % tag
    	glyphOrder.append(glyphName)
    	addGlyphToCFF(
    		glyphName=glyphName,
    		program=failGlyphProgram,
    		private=private,
    		globalSubrs=globalSubrs,
    		charStringsIndex=charStringsIndex,
    		topDict=topDict,
            charStrings=charStrings
    	)
    	hmtx[glyphName] = failGlyphMetrics
     
    	for table in cmap.tables:
    		if table.format == 4:
    			table.cmap[cp] = glyphName
    		else:
    			raise NotImplementedError, "Unsupported cmap table format: %d" % table.format

        
    	cp += 3

    
    shell.setGlyphOrder(glyphOrder)
    
    
    shell["GSUB"] = newTable("GSUB")
    gsub = shell["GSUB"].table = GSUB()
    gsub.Version = 1.0
    
    
    featureCount = len(features)
    
    
    scriptList = gsub.ScriptList = ScriptList()
    scriptList.ScriptCount = 1
    scriptList.ScriptRecord = []
    scriptRecord = ScriptRecord()
    scriptList.ScriptRecord.append(scriptRecord)
    scriptRecord.ScriptTag = "DFLT"
    script = scriptRecord.Script = Script()
    defaultLangSys = script.DefaultLangSys = DefaultLangSys()
    defaultLangSys.FeatureCount = featureCount
    defaultLangSys.FeatureIndex = range(defaultLangSys.FeatureCount)
    defaultLangSys.ReqFeatureIndex = 65535
    defaultLangSys.LookupOrder = None
    script.LangSysCount = 0
    script.LangSysRecord = []
    
    
    featureList = gsub.FeatureList = FeatureList()
    featureList.FeatureCount = featureCount
    featureList.FeatureRecord = []
    for index, tag in enumerate(features):
        
        featureRecord = FeatureRecord()
        featureRecord.FeatureTag = tag
        feature = featureRecord.Feature = Feature()
        featureList.FeatureRecord.append(featureRecord)
        
        feature.FeatureParams = None
        feature.LookupCount = 1
        feature.LookupListIndex = [index]
    
    
    lookupList = gsub.LookupList = LookupList()
    lookupList.LookupCount = featureCount
    lookupList.Lookup = []
    for tag in features:
        
        lookup = Lookup()
        lookup.LookupType = 1
        lookup.LookupFlag = 0
        lookup.SubTableCount = 1
        lookup.SubTable = []
        lookupList.Lookup.append(lookup)
        
        subtable = SingleSubst()
        subtable.Format = 2
        subtable.LookupType = 1
        subtable.mapping = {
            "%s.pass" % tag : "%s.fail" % tag,
            "%s.fail" % tag : "%s.pass" % tag,
        }
        lookup.SubTable.append(subtable)
    
    path = outputPath % 1 + ".otf"
    if os.path.exists(path):
    	os.remove(path)
    shell.save(path)
    
    
    if os.path.exists(shellTempPath):
        os.remove(shellTempPath)
    
def makeLookup3():
    
    f = open(shellSourcePath)
    ttxData = f.read()
    f.close()
    ttxData = ttxData.replace("__familyName__", "gsubtest-lookup3")
    tempShellSourcePath = shellSourcePath + ".temp"
    f = open(tempShellSourcePath, "wb")
    f.write(ttxData)
    f.close()
    
    
    shell = TTFont(sfntVersion="OTTO")
    shell.importXML(tempShellSourcePath)
    shell.save(shellTempPath)
    os.remove(tempShellSourcePath)
    
    
    shell = TTFont(shellTempPath)
    
    
    hmtx = shell["hmtx"]
    glyphSet = shell.getGlyphSet()
    
    failGlyph = glyphSet["F"]
    failGlyph.decompile()
    failGlyphProgram = list(failGlyph.program)
    failGlyphMetrics = hmtx["F"]
    
    passGlyph = glyphSet["P"]
    passGlyph.decompile()
    passGlyphProgram = list(passGlyph.program)
    passGlyphMetrics = hmtx["P"]
    
    
    hmtx = shell["hmtx"]
    cmap = shell["cmap"]
    
    
    existingGlyphs = [".notdef", "space", "F", "P"]
    glyphOrder = list(existingGlyphs)
    
    
    cff = shell["CFF "].cff
    globalSubrs = cff.GlobalSubrs
    topDict = cff.topDictIndex[0]
    topDict.charset = existingGlyphs
    private = topDict.Private
    charStrings = topDict.CharStrings
    charStringsIndex = charStrings.charStringsIndex
    
    features = sorted(mapping)

    
    cp = baseCodepoint
    for index, tag in enumerate(features):
    
    	
    	glyphName = "%s.pass" % tag
    	glyphOrder.append(glyphName)
    	addGlyphToCFF(
    		glyphName=glyphName,
    		program=passGlyphProgram,
    		private=private,
    		globalSubrs=globalSubrs,
    		charStringsIndex=charStringsIndex,
    		topDict=topDict,
            charStrings=charStrings
    	)
    	hmtx[glyphName] = passGlyphMetrics
     
    	
    	glyphName = "%s.fail" % tag
    	glyphOrder.append(glyphName)
    	addGlyphToCFF(
    		glyphName=glyphName,
    		program=failGlyphProgram,
    		private=private,
    		globalSubrs=globalSubrs,
    		charStringsIndex=charStringsIndex,
    		topDict=topDict,
            charStrings=charStrings
    	)
    	hmtx[glyphName] = failGlyphMetrics
     
    	
    	glyphName = "%s.default" % tag
    	glyphOrder.append(glyphName)
    	addGlyphToCFF(
    		glyphName=glyphName,
    		program=passGlyphProgram,
    		private=private,
    		globalSubrs=globalSubrs,
    		charStringsIndex=charStringsIndex,
    		topDict=topDict,
            charStrings=charStrings
    	)
    	hmtx[glyphName] = passGlyphMetrics
    
    	for table in cmap.tables:
    		if table.format == 4:
    			table.cmap[cp] = glyphName
    		else:
    			raise NotImplementedError, "Unsupported cmap table format: %d" % table.format
    	cp += 1
    
    	
    	for i in range(1,4):
    		glyphName = "%s.alt%d" % (tag, i)
    		glyphOrder.append(glyphName)
    		addGlyphToCFF(
    			glyphName=glyphName,
    			program=failGlyphProgram,
    			private=private,
    			globalSubrs=globalSubrs,
    			charStringsIndex=charStringsIndex,
    			topDict=topDict,
                charStrings=charStrings
    		)
    		hmtx[glyphName] = failGlyphMetrics
    		for table in cmap.tables:
    			if table.format == 4:
    				table.cmap[cp] = glyphName
    			else:
    				raise NotImplementedError, "Unsupported cmap table format: %d" % table.format
    		cp += 1
    	
    
    shell.setGlyphOrder(glyphOrder)
    
    
    shell["GSUB"] = newTable("GSUB")
    gsub = shell["GSUB"].table = GSUB()
    gsub.Version = 1.0
    
    
    featureCount = len(features)
    
    
    scriptList = gsub.ScriptList = ScriptList()
    scriptList.ScriptCount = 1
    scriptList.ScriptRecord = []
    scriptRecord = ScriptRecord()
    scriptList.ScriptRecord.append(scriptRecord)
    scriptRecord.ScriptTag = "DFLT"
    script = scriptRecord.Script = Script()
    defaultLangSys = script.DefaultLangSys = DefaultLangSys()
    defaultLangSys.FeatureCount = featureCount
    defaultLangSys.FeatureIndex = range(defaultLangSys.FeatureCount)
    defaultLangSys.ReqFeatureIndex = 65535
    defaultLangSys.LookupOrder = None
    script.LangSysCount = 0
    script.LangSysRecord = []
    
    
    featureList = gsub.FeatureList = FeatureList()
    featureList.FeatureCount = featureCount
    featureList.FeatureRecord = []
    for index, tag in enumerate(features):
    	
    	featureRecord = FeatureRecord()
    	featureRecord.FeatureTag = tag
    	feature = featureRecord.Feature = Feature()
    	featureList.FeatureRecord.append(featureRecord)
    	
    	feature.FeatureParams = None
    	feature.LookupCount = 1
    	feature.LookupListIndex = [index]
    
    
    lookupList = gsub.LookupList = LookupList()
    lookupList.LookupCount = featureCount
    lookupList.Lookup = []
    for tag in features:
    	
    	lookup = Lookup()
    	lookup.LookupType = 3
    	lookup.LookupFlag = 0
    	lookup.SubTableCount = 1
    	lookup.SubTable = []
    	lookupList.Lookup.append(lookup)
    	
    	subtable = AlternateSubst()
    	subtable.Format = 1
    	subtable.LookupType = 3
    	subtable.alternates = {
    		"%s.default" % tag : ["%s.fail" % tag, "%s.fail" % tag, "%s.fail" % tag],
    		"%s.alt1" % tag    : ["%s.pass" % tag, "%s.fail" % tag, "%s.fail" % tag],
    		"%s.alt2" % tag    : ["%s.fail" % tag, "%s.pass" % tag, "%s.fail" % tag],
    		"%s.alt3" % tag    : ["%s.fail" % tag, "%s.fail" % tag, "%s.pass" % tag]
    	}
    	lookup.SubTable.append(subtable)
    
    path = outputPath % 3 + ".otf"
    if os.path.exists(path):
    	os.remove(path)
    shell.save(path)
    
    
    if os.path.exists(shellTempPath):
        os.remove(shellTempPath)
    
def makeJavascriptData():
    features = sorted(mapping)
    outStr = []

    outStr.append("")
    outStr.append("/* This file is autogenerated by makegsubfonts.py */")
    outStr.append("")
    outStr.append("/* ")
    outStr.append("  Features defined in gsubtest fonts with associated base")
    outStr.append("  codepoints for each feature:")
    outStr.append("")
    outStr.append("    cp = codepoint for feature featX")
    outStr.append("")
    outStr.append("    cp   default   PASS")
    outStr.append("    cp   featX=1   FAIL")
    outStr.append("    cp   featX=2   FAIL")
    outStr.append("")
    outStr.append("    cp+1 default   FAIL")
    outStr.append("    cp+1 featX=1   PASS")
    outStr.append("    cp+1 featX=2   FAIL")
    outStr.append("")
    outStr.append("    cp+2 default   FAIL")
    outStr.append("    cp+2 featX=1   FAIL")
    outStr.append("    cp+2 featX=2   PASS")
    outStr.append("")
    outStr.append("*/")
    outStr.append("")
    outStr.append("var gFeatures = {");
    cp = baseCodepoint

    taglist = []
    for tag in features:
        taglist.append("\"%s\": 0x%x" % (tag, cp))
        cp += 4
    
    outStr.append(textwrap.fill(", ".join(taglist), initial_indent="  ", subsequent_indent="  "))
    outStr.append("};");
    outStr.append("");

    if os.path.exists(javascriptData):
    	os.remove(javascriptData)

    f = open(javascriptData, "wb")
    f.write("\n".join(outStr))
    f.close()




print "Making lookup type 1 font..."
makeLookup1()

print "Making lookup type 3 font..."
makeLookup3()



print "Making javascript data file..."
makeJavascriptData()