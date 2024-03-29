function makeURL(spec) {
  return Cc["@mozilla.org/network/io-service;1"].
           getService(Components.interfaces.nsIIOService).
           newURI(spec, null, null).
           QueryInterface(Components.interfaces.nsIURL);
}


function run_test() {

  
  let tests = [
    ["http://mozilla.org/",  "http://www.mozilla.org/", "http://www.mozilla.org/"],
    ["http://mozilla.org/",  "http://www.mozilla.org",  "http://www.mozilla.org/"],
    ["http://foo.com/bar/",  "http://foo.com:80/bar/",  ""                       ],
    ["http://foo.com/",      "http://foo.com/a.htm#b",  "a.htm#b"                ],
    ["http://foo.com/a/b/",  "http://foo.com/c",        "../../c"                ],
    ["http://foo.com/a?b/c/", "http://foo.com/c" ,      "c"                      ],
    ["http://foo.com/a#b/c/", "http://foo.com/c" ,      "c"                      ],
    ["http://foo.com/a;p?b/c/", "http://foo.com/c" ,    "c"                      ],
    ["http://foo.com/a/b?c/d/", "http://foo.com/c",     "../c"                   ],
    ["http://foo.com/a/b#c/d/", "http://foo.com/c",     "../c"                   ],
    ["http://foo.com/a/b;p?c/d/", "http://foo.com/c",   "../c"                   ],
    ["http://foo.com/a/b/c?d/e/", "http://foo.com/f",   "../../f"                ],
    ["http://foo.com/a/b/c#d/e/", "http://foo.com/f",   "../../f"                ],
    ["http://foo.com/a/b/c;p?d/e/", "http://foo.com/f", "../../f"                ],
    ["http://foo.com/a?b/c/", "http://foo.com/c/d" ,    "c/d"                    ],
    ["http://foo.com/a#b/c/", "http://foo.com/c/d" ,    "c/d"                    ],
    ["http://foo.com/a;p?b/c/", "http://foo.com/c/d" ,  "c/d"                    ],
    ["http://foo.com/a/b?c/d/", "http://foo.com/c/d",   "../c/d"                 ],
    ["http://foo.com/a/b#c/d/", "http://foo.com/c/d",   "../c/d"                 ],
    ["http://foo.com/a/b;p?c/d/", "http://foo.com/c/d", "../c/d"                 ],
    ["http://foo.com/a/b/c?d/e/", "http://foo.com/f/g/", "../../f/g/"            ],
    ["http://foo.com/a/b/c#d/e/", "http://foo.com/f/g/", "../../f/g/"            ],
    ["http://foo.com/a/b/c;p?d/e/", "http://foo.com/f/g/", "../../f/g/"          ],
  ];

  for (var i = 0; i < tests.length; i++) {
    let url1 = makeURL(tests[i][0]);
    let url2 = makeURL(tests[i][1]);
    let expected = tests[i][2];
    do_check_eq(expected, url1.getRelativeSpec(url2));
  }
}
