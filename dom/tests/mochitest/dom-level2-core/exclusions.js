





































dtdTests = ["attrgetownerelement01", "documentimportnode03", 
            "documentimportnode04", "documentimportnode19",
            "documentimportnode20", "documentimportnode21",
            "documentimportnode22", "documenttypeinternalSubset01", 
            "elementgetattributenodens03", "elementgetattributens02",
            "elementhasattribute02", "getAttributeNS01", "getElementById01",
            "getNamedItemNS03", "getNamedItemNS04", "hasAttribute02",
            "hasAttributeNS04", "importNode07", "importNode09",
            "importNode10", "importNode11", "importNode12", "importNode13",
            "internalSubset01", "localName02", "namednodemapgetnameditemns01",
            "namednodemapremovenameditemns02",
            "namednodemapremovenameditemns05", "namednodemapsetnameditemns05",
            "namednodemapsetnameditemns09", "namednodemapsetnameditemns10",
            "namednodemapsetnameditemns11", "namespaceURI01", 
            "nodeissupported04", "nodenormalize01", "nodesetprefix04",
            "prefix08", "removeAttributeNS01", "removeAttributeNS02",
            "removeNamedItemNS03", "setAttributeNodeNS02", "setAttributeNS03",
            "setNamedItemNS04"];

bug371552 = ["elementhasattributens02"];
wrongDocError = ["elementsetattributenodens05", "namednodemapsetnameditemns03",
                 "setAttributeNodeNS05", "setNamedItemNS02"];
attrAppendChild = ["elementsetattributenodens06", "importNode01"];
removeNamedItemNS = ["namednodemapremovenameditemns06",
                     "namednodemapremovenameditemns07",
                     "namednodemapremovenameditemns08",
                     "removeNamedItemNS02"];
bogusPrefix = ["nodesetprefix05", "nodesetprefix09", "prefix06", "prefix07"];
prefixReplacement = ["setAttributeNodeNS04"];

var todoTests = {};
var exclusions = concat(dtdTests, bug371552, wrongDocError, attrAppendChild,
                        removeNamedItemNS, bogusPrefix, prefixReplacement);
for (var excludedTestName in exclusions) { 
  todoTests[exclusions[excludedTestName]] = true; 
}
