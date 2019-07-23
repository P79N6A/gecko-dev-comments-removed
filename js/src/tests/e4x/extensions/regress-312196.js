




































gTestfile = 'regress-312196.js';

var summary = "Extending E4X XML objects with __noSuchMethod__";
var BUGNUMBER = 312196;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

XML.ignoreWhitespace = true;
XML.prettyPrint = false;

function ajaxObj(content){
    var ajax = new XML(content);
    ajax.function::__noSuchMethod__ = autoDispatch;
    return ajax;
}

function autoDispatch(id){
    if (! this.@uri)
        return;
    var request = <request method={id}/>;
    for(var i=0; i<arguments[1].length; i++){
        request += <parameter>{arguments[1][i]}</parameter>;
    }
    var xml = this;
    xml.request = request;
    return(xml.toString());
}

var ws = new ajaxObj('<object uri="http://localhost"/>');

try
{
    actual = ws.function::sample('this', 'is', 'a', 'test');
}
catch(e)
{
    actual = e + '';
}

expect =
(<object uri="http://localhost">
<request method="sample"/>
<parameter>this</parameter>
<parameter>is</parameter>
<parameter>a</parameter>
<parameter>test</parameter>
</object>).toString();

TEST(1, expect, actual);

try
{
    actual = ws.sample('this', 'is', 'a', 'test');
}
catch(e)
{
    actual = e + '';
}

expect =
(<object uri="http://localhost">
<request method="sample"/>
<parameter>this</parameter>
<parameter>is</parameter>
<parameter>a</parameter>
<parameter>test</parameter>
<parameter>this</parameter>
<parameter>is</parameter>
<parameter>a</parameter>
<parameter>test</parameter>
</object>).toString();

TEST(2, expect, actual);

END();
