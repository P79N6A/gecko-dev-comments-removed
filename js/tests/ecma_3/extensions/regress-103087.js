


















































var gTestfile = 'regress-103087.js';
var UBound = 0;
var BUGNUMBER = 103087;
var summary = "Testing that we don't crash on any of these regexps -";
var re = '';
var lm = '';
var lc = '';
var rc = '';



var NameStrt = "[A-Za-z_:]|[^\\x00-\\x7F]";
var NameChar = "[A-Za-z0-9_:.-]|[^\\x00-\\x7F]";
var Name = "(" + NameStrt + ")(" + NameChar + ")*";
var TextSE = "[^<]+";
var UntilHyphen = "[^-]*-";
var Until2Hyphens = UntilHyphen + "([^-]" + UntilHyphen + ")*-";
var CommentCE = Until2Hyphens + ">?";
var UntilRSBs = "[^]]*]([^]]+])*]+";
var CDATA_CE = UntilRSBs + "([^]>]" + UntilRSBs + ")*>";
var S = "[ \\n\\t\\r]+";
var QuoteSE = '"[^"]' + "*" + '"' + "|'[^']*'";
var DT_IdentSE = S + Name + "(" + S + "(" + Name + "|" + QuoteSE + "))*";
var MarkupDeclCE = "([^]\"'><]+|" + QuoteSE + ")*>";
var S1 = "[\\n\\r\\t ]";
var UntilQMs = "[^?]*\\?+";
var PI_Tail = "\\?>|" + S1 + UntilQMs + "([^>?]" + UntilQMs + ")*>";
var DT_ItemSE = "<(!(--" + Until2Hyphens + ">|[^-]" + MarkupDeclCE + ")|\\?" + Name + "(" + PI_Tail + "))|%" + Name + ";|" + S;
var DocTypeCE = DT_IdentSE + "(" + S + ")?(\\[(" + DT_ItemSE + ")*](" + S + ")?)?>?";
var DeclCE = "--(" + CommentCE + ")?|\\[CDATA\\[(" + CDATA_CE + ")?|DOCTYPE(" + DocTypeCE + ")?";
var PI_CE = Name + "(" + PI_Tail + ")?";
var EndTagCE = Name + "(" + S + ")?>?";
var AttValSE = '"[^<"]' + "*" + '"' + "|'[^<']*'";
var ElemTagCE = Name + "(" + S + Name + "(" + S + ")?=(" + S + ")?(" + AttValSE + "))*(" + S + ")?/?>?";
var MarkupSPE = "<(!(" + DeclCE + ")?|\\?(" + PI_CE + ")?|/(" + EndTagCE + ")?|(" + ElemTagCE + ")?)";
var XML_SPE = TextSE + "|" + MarkupSPE;
var CommentRE = "<!--" + Until2Hyphens + ">";
var CommentSPE = "<!--(" + CommentCE + ")?";
var PI_RE = "<\\?" + Name + "(" + PI_Tail + ")";
var Erroneous_PI_SE = "<\\?[^?]*(\\?[^>]+)*\\?>";
var PI_SPE = "<\\?(" + PI_CE + ")?";
var CDATA_RE = "<!\\[CDATA\\[" + CDATA_CE;
var CDATA_SPE = "<!\\[CDATA\\[(" + CDATA_CE + ")?";
var ElemTagSE = "<(" + NameStrt + ")([^<>\"']+|" + AttValSE + ")*>";
var ElemTagRE = "<" + Name + "(" + S + Name + "(" + S + ")?=(" + S + ")?(" + AttValSE + "))*(" + S + ")?/?>";
var ElemTagSPE = "<" + ElemTagCE;
var EndTagRE = "</" + Name + "(" + S + ")?>";
var EndTagSPE = "</(" + EndTagCE + ")?";
var DocTypeSPE = "<!DOCTYPE(" + DocTypeCE + ")?";
var PERef_APE = "%(" + Name + ";?)?";
var HexPart = "x([0-9a-fA-F]+;?)?";
var NumPart = "#([0-9]+;?|" + HexPart + ")?";
var CGRef_APE = "&(" + Name + ";?|" + NumPart + ")?";
var Text_PE = CGRef_APE + "|[^&]+";
var EntityValue_PE = CGRef_APE + "|" + PERef_APE + "|[^%&]+";


var rePatterns = new Array(AttValSE, CDATA_CE, CDATA_RE, CDATA_SPE, CGRef_APE, CommentCE, CommentRE, CommentSPE, DT_IdentSE, DT_ItemSE, DeclCE, DocTypeCE, DocTypeSPE, ElemTagCE, ElemTagRE, ElemTagSE, ElemTagSPE, EndTagCE, EndTagRE, EndTagSPE, EntityValue_PE, Erroneous_PI_SE, HexPart, MarkupDeclCE, MarkupSPE, Name, NameChar, NameStrt, NumPart, PERef_APE, PI_CE, PI_RE, PI_SPE, PI_Tail, QuoteSE, S, S1, TextSE, Text_PE, Until2Hyphens, UntilHyphen, UntilQMs, UntilRSBs, XML_SPE);



var str = '';
str += '<html xmlns="http://www.w3.org/1999/xhtml"' + '\n';
str += '      xmlns:xlink="http://www.w3.org/XML/XLink/0.9">' + '\n';
str += '  <head><title>Three Namespaces</title></head>' + '\n';
str += '  <body>' + '\n';
str += '    <h1 align="center">An Ellipse and a Rectangle</h1>' + '\n';
str += '    <svg xmlns="http://www.w3.org/Graphics/SVG/SVG-19991203.dtd" ' + '\n';
str += '         width="12cm" height="10cm">' + '\n';
str += '      <ellipse rx="110" ry="130" />' + '\n';
str += '      <rect x="4cm" y="1cm" width="3cm" height="6cm" />' + '\n';
str += '    </svg>' + '\n';
str += '    <p xlink:type="simple" xlink:href="ellipses.html">' + '\n';
str += '      More about ellipses' + '\n';
str += '    </p>' + '\n';
str += '    <p xlink:type="simple" xlink:href="rectangles.html">' + '\n';
str += '      More about rectangles' + '\n';
str += '    </p>' + '\n';
str += '    <hr/>' + '\n';
str += '    <p>Last Modified February 13, 2000</p>    ' + '\n';
str += '  </body>' + '\n';
str += '</html>';




test();




function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i=0; i<rePatterns.length; i++)
  {
    status = inSection(i);
    re = new RegExp(rePatterns[i]);

    
    re.exec(str);
    getResults();

    
    re.exec(lc);
    getResults();

    
    re.exec(rc);
    getResults();
  }

  reportCompare('No Crash', 'No Crash', '');

  exitFunc ('test');
}


function getResults()
{
  lm = RegExp.lastMatch;
  lc = RegExp.leftContext;
  rc = RegExp.rightContext;
}
