





































gTestfile = 'regress-350238.js';

var BUGNUMBER = 350238;
var summary = 'Do not assert <x/>.@*++';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

if (typeof document != 'undefined' && 'addEventListener' in document)
{
    document.addEventListener('load',
                              (function () {
                                  var iframe = document.createElement('iframe');
                                  document.body.appendChild(iframe);
                                  iframe.contentDocument.location.href='javascript:<x/>.@*++;';
                              }), true);
}
else
{
    <x/>.@*++;
}

TEST(1, expect, actual);

END();
