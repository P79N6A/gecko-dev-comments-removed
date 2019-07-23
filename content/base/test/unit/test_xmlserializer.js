





































function xmlEncode(aFile, aFlags, aCharset) {
    if(aFlags == undefined) aFlags = 0;
    if(aCharset == undefined) aCharset = "UTF-8";

    var doc = do_parse_document(aFile, "text/xml");

    var encoder = Components.classes["@mozilla.org/layout/documentEncoder;1?type=text/xml"]
                   .createInstance(nsIDocumentEncoder);
    encoder.setCharset(aCharset);
    encoder.init(doc, "text/xml", aFlags);
    return encoder.encodeToString();
}

function run_test()
{
    var result, expected;
    const de = Components.interfaces.nsIDocumentEncoder;

    result = xmlEncode("1_original.xml", de.OutputLFLineBreak);
    expected =loadContentFile("1_result.xml");
    do_check_eq(expected, result);

    result =  xmlEncode("2_original.xml", de.OutputLFLineBreak);
    expected = loadContentFile("2_result_1.xml");
    do_check_eq(expected, result);

    result =  xmlEncode("2_original.xml", de.OutputLFLineBreak);
    do_check_eq(expected, result);

    result =  xmlEncode("2_original.xml", de.OutputCRLineBreak);
    expected = expected.replace(/\n/g, "\r");
    do_check_eq(expected, result);

    result = xmlEncode("2_original.xml", de.OutputLFLineBreak | de.OutputCRLineBreak);
    expected = expected.replace(/\r/mg, "\r\n");
    do_check_eq(expected, result);
}
