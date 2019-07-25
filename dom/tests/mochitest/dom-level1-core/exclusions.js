







































var dtdTests = ["attrdefaultvalue","attrnotspecifiedvalue", "attrremovechild1",
                "attrreplacechild1", "attrsetvaluenomodificationallowederr",
                "attrsetvaluenomodificationallowederrEE", "attrspecifiedvalueremove",
                "characterdataappenddatanomodificationallowederr", "characterdataappenddatanomodificationallowederrEE",
                "characterdatadeletedatanomodificationallowederr", "characterdatadeletedatanomodificationallowederrEE",
                "characterdatainsertdatanomodificationallowederr", "characterdatainsertdatanomodificationallowederrEE",
                "characterdatareplacedatanomodificationallowederr", "characterdatareplacedatanomodificationallowederrEE",
                "characterdatasetdatanomodificationallowederr", "characterdatasetdatanomodificationallowederrEE",
                "documentcreateelementdefaultattr", "documentcreateentityreference", "documentcreateentityreferenceknown",
                "documenttypegetentities", "documenttypegetentitieslength", "documenttypegetentitiestype",
                "documenttypegetnotations", "documenttypegetnotationstype", "elementremoveattribute",
                "elementremoveattributenodenomodificationallowederr", "elementremoveattributenodenomodificationallowederrEE",
                "elementremoveattributenomodificationallowederr", "elementremoveattributenomodificationallowederrEE",
                "elementremoveattributerestoredefaultvalue", "elementretrieveallattributes",
                "elementsetattributenodenomodificationallowederr", "elementsetattributenodenomodificationallowederrEE",
                "elementsetattributenomodificationallowederr", "elementsetattributenomodificationallowederrEE",
                "entitygetentityname", "entitygetpublicid", "entitygetpublicidnull", "namednodemapremovenameditem",
                "namednodemapremovenameditemgetvalue", "nodeappendchildnomodificationallowederr", "nodeappendchildnomodificationallowederrEE",
                "nodeentitynodeattributes", "nodeentitynodename", "nodeentitynodetype", "nodeentitynodevalue",
                "nodeentityreferencenodeattributes", "nodeentityreferencenodename", "nodeentityreferencenodetype",
                "nodeentityreferencenodevalue", "nodeentitysetnodevalue", "nodeinsertbeforenomodificationallowederr",
                "nodeinsertbeforenomodificationallowederrEE", "nodenotationnodeattributes", "nodenotationnodename",
                "nodenotationnodetype", "nodenotationnodevalue", "noderemovechildnomodificationallowederr",
                "noderemovechildnomodificationallowederrEE", "nodereplacechildnomodificationallowederr",
                "nodereplacechildnomodificationallowederrEE", "nodesetnodevaluenomodificationallowederr",
                "nodesetnodevaluenomodificationallowederrEE", "nodevalue03","nodevalue07", "nodevalue08",
                "notationgetnotationname", "notationgetpublicid", "notationgetpublicidnull", "notationgetsystemid",
                "notationgetsystemidnull", "processinginstructionsetdatanomodificationallowederr",
                "processinginstructionsetdatanomodificationallowederrEE", "textsplittextnomodificationallowederr",
                "textsplittextnomodificationallowederrEE"];


var indexErrTests = ["characterdataindexsizeerrdeletedatacountnegative", "characterdataindexsizeerrreplacedatacountnegative",
                     "characterdataindexsizeerrsubstringcountnegative", "hc_characterdataindexsizeerrdeletedatacountnegative",
                     "hc_characterdataindexsizeerrreplacedatacountnegative", "hc_characterdataindexsizeerrsubstringcountnegative"];

var attributeModTests = ["hc_attrappendchild1", "hc_attrappendchild3", "hc_attrappendchild5",
                         "hc_attrappendchild6", "hc_attrchildnodes2", "hc_attrclonenode1", "hc_attrinsertbefore1",
                         "hc_attrinsertbefore2", "hc_attrinsertbefore3", "hc_attrinsertbefore4", "hc_attrinsertbefore6",
                         "hc_attrnormalize", "hc_attrremovechild2", "hc_attrreplacechild1", "hc_attrreplacechild2",
                         "hc_attrsetvalue2", "hc_elementnormalize2", "hc_elementnotfounderr", "hc_elementremoveattribute", "hc_elementnormalize2",
                         "hc_elementnotfounderr", "hc_elementremoveattribute", ];
var modTests = ["hc_elementwrongdocumenterr", "hc_namednodemapwrongdocumenterr", "hc_nodeappendchildnewchilddiffdocument", "hc_nodeinsertbeforenewchilddiffdocument",
                "hc_nodereplacechildnewchilddiffdocument", "hc_elementwrongdocumenterr", "hc_namednodemapwrongdocumenterr", "hc_nodeappendchildnewchilddiffdocument",
                "hc_nodeinsertbeforenewchilddiffdocument", "hc_nodereplacechildnewchilddiffdocument", "elementwrongdocumenterr", "namednodemapwrongdocumenterr",
                "nodeappendchildnewchilddiffdocument", "nodeinsertbeforenewchilddiffdocument", "nodereplacechildnewchilddiffdocument"];

var createEntityRef = ["documentinvalidcharacterexceptioncreateentref",
                       "documentinvalidcharacterexceptioncreateentref1",
                       "hc_attrgetvalue2", "hc_nodevalue03"];


var todoTests = {};
var exclusions = concat(dtdTests, indexErrTests, attributeModTests, modTests, createEntityRef);
for (var excludedTestName in exclusions) { todoTests[exclusions[excludedTestName]] = true; }
