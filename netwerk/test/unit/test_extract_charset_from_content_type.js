





































var charset = {};
var charsetStart = {};
var charsetEnd = {};
var hadCharset;

function reset() {
  delete charset.value;
  delete charsetStart.value;
  delete charsetEnd.value;
  hadCharset = undefined;
}

function check(aHadCharset, aCharset, aCharsetStart, aCharsetEnd) {
  do_check_eq(aHadCharset, hadCharset);
  do_check_eq(aCharset, charset.value);
  if (hadCharset) {
    do_check_eq(aCharsetStart, charsetStart.value);
    do_check_eq(aCharsetEnd, charsetEnd.value);
  }
}

function run_test() {
  var netutil = Components.classes["@mozilla.org/network/util;1"]
                          .getService(Components.interfaces.nsINetUtil);
  hadCharset =
    netutil.extractCharsetFromContentType("text/html", charset, charsetStart,
					  charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType("TEXT/HTML", charset, charsetStart,
					  charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType("text/html, text/html", charset,
					  charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType("text/html, text/plain",
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/html, ', charset, charsetStart,
					  charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/html, */*', charset,
					  charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/html, foo', charset,
					  charsetStart, charsetEnd);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html; charset=ISO-8859-1",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 9, 29);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html  ;    charset=ISO-8859-1",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 11, 34);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html  ;    charset=ISO-8859-1  ",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 11, 36);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html  ;    charset=ISO-8859-1 ; ",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 11, 35);

  hadCharset =
    netutil.extractCharsetFromContentType('text/html; charset="ISO-8859-1"',
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 9, 31);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html; charset='ISO-8859-1'",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 9, 31);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html; charset='ISO-8859-1', text/html",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 9, 31);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html; charset='ISO-8859-1', text/html; charset=UTF8",
					  charset, charsetStart, charsetEnd);
  check(true, "UTF8", 42, 56);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html; charset=ISO-8859-1, TEXT/HTML",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 9, 29);

  hadCharset =
    netutil.extractCharsetFromContentType("text/html; charset=ISO-8859-1, TEXT/plain",
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType("text/plain, TEXT/HTML; charset='ISO-8859-1', text/html, TEXT/HTML",
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 21, 43);

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain, TEXT/HTML; param="charset=UTF8"; charset=\'ISO-8859-1\'; param2="charset=UTF16", text/html, TEXT/HTML',
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 43, 65);

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain, TEXT/HTML; param=charset=UTF8; charset=\'ISO-8859-1\'; param2=charset=UTF16, text/html, TEXT/HTML',
					  charset, charsetStart, charsetEnd);
  check(true, "ISO-8859-1", 41, 63);

  hadCharset =
    netutil.extractCharsetFromContentType("text/plain; param= , text/html",
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain; param=", text/html"',
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain; param=", \\" , text/html"',
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain; param=", \\" , text/html , "',
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain param=", \\" , text/html , "',
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain charset=UTF8',
					  charset, charsetStart, charsetEnd);
  check(false, "");

  hadCharset =
    netutil.extractCharsetFromContentType('text/plain, TEXT/HTML; param="charset=UTF8"; ; param2="charset=UTF16", text/html, TEXT/HTML',
					  charset, charsetStart, charsetEnd);
  check(false, "");
}
