







var Cc = Components.classes;
var Ci = Components.interfaces;

function run_test() {
  let converter = Cc["@mozilla.org/txttohtmlconv;1"]
                     .getService(Ci.mozITXTToHTMLConv);

  const tests = [
    
    {
      input: "RFC1738: <URL:http://mozilla.org> then",
      url: "http://mozilla.org"
    },
    
    {
      input: "RFC2396E: <http://mozilla.org/> then",
      url: "http://mozilla.org/"
    },
    
    {
      input: "see www.mozilla.org maybe",
      url: "http://www.mozilla.org"
    },
    
    {
      input:"I mean http://www.mozilla.org/.",
      url: "http://www.mozilla.org/"
    },
    {
      input:"you mean http://mozilla.org:80, right?",
      url: "http://mozilla.org:80"
    },
    {
      input:"go to http://mozilla.org; then go home",
      url: "http://mozilla.org"
    },
    {
      input:"http://mozilla.org! yay!",
      url: "http://mozilla.org"
    },
    {
      input:"er, http://mozilla.com?",
      url: "http://mozilla.com"
    },
    {
      input:"http://example.org- where things happen",
      url: "http://example.org"
    },
    {
      input:"see http://mozilla.org: front page",
      url: "http://mozilla.org"
    },
    {
      input:"'http://mozilla.org/': that's the url",
      url: "http://mozilla.org/"
    },
    {
      input:"some special http://mozilla.org/?x=.,;!-:x",
      url: "http://mozilla.org/?x=.,;!-:x"
    },
    {
      
      input:"'http://example.org/?test=true&success=true': ok",
      url: "http://example.org/?test=true&amp;success=true"
    },
    {
      input: "bracket: http://localhost/[1] etc.",
      url: "http://localhost/"
    },
    {
      input: "parenthesis: (http://localhost/) etc.",
      url: "http://localhost/"
    },
    {
      input: "(thunderbird)http://mozilla.org/thunderbird",
      url: "http://mozilla.org/thunderbird"
    },
    {
      input: "()http://mozilla.org",
      url: "http://mozilla.org"
    },
    {
      input: "parenthesis included: http://kb.mozillazine.org/Performance_(Thunderbird) etc.",
      url: "http://kb.mozillazine.org/Performance_(Thunderbird)"
    },
    {
      input: "parenthesis slash bracket: (http://localhost/)[1] etc.",
      url: "http://localhost/"
    },
    {
      input: "parenthesis bracket: (http://example.org[1]) etc.",
      url: "http://example.org"
    },
    {
      input: "ipv6 1: https://[1080::8:800:200C:417A]/foo?bar=x test",
      url: "https://[1080::8:800:200C:417A]/foo?bar=x"
    },
    {
      input: "ipv6 2: http://[::ffff:127.0.0.1]/#yay test",
      url: "http://[::ffff:127.0.0.1]/#yay"
    },
    {
      input: "ipv6 parenthesis port: (http://[2001:db8::1]:80/) test",
      url: "http://[2001:db8::1]:80/"
    }
  ];

  function hrefLink(url) {
    return ' href="' + url + '"';
  }

  for (let i = 0; i < tests.length; i++) {
    let output = converter.scanTXT(tests[i].input, Ci.mozITXTToHTMLConv.kURLs);
    let link = hrefLink(tests[i].url);
    if (output.indexOf(link) == -1)
      do_throw("Unexpected conversion: input=" + tests[i].input +
               ", output=" + output + ", link=" + link);
  }
}
