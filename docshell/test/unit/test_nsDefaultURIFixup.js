let urifixup = Cc["@mozilla.org/docshell/urifixup;1"].
               getService(Ci.nsIURIFixup);
let prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);

let pref = "browser.fixup.typo.scheme";

let data = [
  {
    
    wrong: 'ttp://www.example.com/',
    fixed: 'http://www.example.com/',
  },
  {
    
    wrong: 'ttps://www.example.com/',
    fixed: 'https://www.example.com/',
  },
  {
    
    wrong: 'tps://www.example.com/',
    fixed: 'https://www.example.com/',
  },
  {
    
    wrong: 'ps://www.example.com/',
    fixed: 'https://www.example.com/',
  },
  {
    
    wrong: 'ile:///this/is/a/test.html',
    fixed: 'file:///this/is/a/test.html',
  },
  {
    
    wrong: 'le:///this/is/a/test.html',
    fixed: 'file:///this/is/a/test.html',
  },
  {
    
    wrong: 'https://example.com/this/is/a/test.html',
    fixed: 'https://example.com/this/is/a/test.html',
  },
  {
    
    wrong: 'whatever://this/is/a/test.html',
    fixed: 'whatever://this/is/a/test.html',
  },
];

let len = data.length;

function run_test() {
  run_next_test();
}


add_task(function test_unset_pref_fixes_typos() {
  prefs.clearUserPref(pref);
  for (let i = 0; i < len; ++i) {
    let item = data[i];
    let result =
      urifixup.createFixupURI(item.wrong,
                              urifixup.FIXUP_FLAG_FIX_SCHEME_TYPOS).spec;
    do_check_eq(result, item.fixed);
  }
});
  


add_task(function test_false_pref_keeps_typos() {
  prefs.setBoolPref(pref, false);
  for (let i = 0; i < len; ++i) {
    let item = data[i];
    let result =
      urifixup.createFixupURI(item.wrong,
                              urifixup.FIXUP_FLAG_FIX_SCHEME_TYPOS).spec;
    do_check_eq(result, item.wrong);
  }
});



add_task(function test_true_pref_fixes_typos() {
  prefs.setBoolPref(pref, true);
  for (let i = 0; i < len; ++i) {
    let item = data[i];
    let result =
        urifixup.createFixupURI(item.wrong,
                                urifixup.FIXUP_FLAG_FIX_SCHEME_TYPOS).spec;
    do_check_eq(result, item.fixed);
  }
});
