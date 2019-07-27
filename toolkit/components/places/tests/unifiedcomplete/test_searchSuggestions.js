Cu.import("resource://gre/modules/FormHistory.jsm");

const ENGINE_NAME = "engine-suggestions.xml";
const SERVER_PORT = 9000;
const SUGGEST_PREF = "browser.urlbar.suggest.searches";
const SUGGEST_RESTRICT_TOKEN = "$";

let suggestionsFn;
let previousSuggestionsFn;

function setSuggestionsFn(fn) {
  previousSuggestionsFn = suggestionsFn;
  suggestionsFn = fn;
}

function* cleanUpSuggestions() {
  yield cleanup();
  if (previousSuggestionsFn) {
    suggestionsFn = previousSuggestionsFn;
    previousSuggestionsFn = null;
  }
}

add_task(function* setUp() {
  
  
  let server = makeTestServer(SERVER_PORT);
  server.registerPathHandler("/suggest", (req, resp) => {
    
    
    let searchStr = decodeURIComponent(req.queryString.replace(/\+/g, " "));
    let suggestions = suggestionsFn(searchStr);
    let data = [searchStr, suggestions];
    resp.setHeader("Content-Type", "application/json", false);
    resp.write(JSON.stringify(data));
  });
  setSuggestionsFn(searchStr => {
    let suffixes = ["foo", "bar"];
    return suffixes.map(s => searchStr + " " + s);
  });

  
  let oldCurrentEngine = Services.search.currentEngine;
  do_register_cleanup(() => Services.search.currentEngine = oldCurrentEngine);
  let engine = yield addTestEngine(ENGINE_NAME, server);
  Services.search.currentEngine = engine;
});

add_task(function* disabled() {
  Services.prefs.setBoolPref(SUGGEST_PREF, false);
  yield check_autocomplete({
    search: "hello",
    matches: [],
  });
  yield cleanUpSuggestions();
});

add_task(function* singleWordQuery() {
  Services.prefs.setBoolPref(SUGGEST_PREF, true);

  yield check_autocomplete({
    search: "hello",
    matches: [{
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "hello foo",
        searchQuery: "hello",
        searchSuggestion: "hello foo",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }, {
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "hello bar",
        searchQuery: "hello",
        searchSuggestion: "hello bar",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }],
  });

  yield cleanUpSuggestions();
});

add_task(function* multiWordQuery() {
  Services.prefs.setBoolPref(SUGGEST_PREF, true);

  yield check_autocomplete({
    search: "hello world",
    matches: [{
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "hello world foo",
        searchQuery: "hello world",
        searchSuggestion: "hello world foo",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }, {
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "hello world bar",
        searchQuery: "hello world",
        searchSuggestion: "hello world bar",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }],
  });

  yield cleanUpSuggestions();
});

add_task(function* suffixMatch() {
  Services.prefs.setBoolPref(SUGGEST_PREF, true);

  setSuggestionsFn(searchStr => {
    let prefixes = ["baz", "quux"];
    return prefixes.map(p => p + " " + searchStr);
  });

  yield check_autocomplete({
    search: "hello",
    matches: [{
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "baz hello",
        searchQuery: "hello",
        searchSuggestion: "baz hello",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }, {
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "quux hello",
        searchQuery: "hello",
        searchSuggestion: "quux hello",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }],
  });

  yield cleanUpSuggestions();
});

add_task(function* queryIsNotASubstring() {
  Services.prefs.setBoolPref(SUGGEST_PREF, true);

  setSuggestionsFn(searchStr => {
    return ["aaa", "bbb"];
  });

  yield check_autocomplete({
    search: "hello",
    matches: [{
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "aaa",
        searchQuery: "hello",
        searchSuggestion: "aaa",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }, {
      uri: makeActionURI(("searchengine"), {
        engineName: ENGINE_NAME,
        input: "bbb",
        searchQuery: "hello",
        searchSuggestion: "bbb",
      }),
      title: ENGINE_NAME,
      style: ["action", "searchengine"],
      icon: "",
    }],
  });

  yield cleanUpSuggestions();
});

add_task(function* restrictToken() {
  Services.prefs.setBoolPref(SUGGEST_PREF, true);

  
  
  
  yield PlacesTestUtils.addVisits([
    {
      uri: NetUtil.newURI("http://example.com/hello-visit"),
      title: "hello visit",
    },
    {
      uri: NetUtil.newURI("http://example.com/hello-bookmark"),
      title: "hello bookmark",
    },
  ]);

  yield addBookmark({
    uri: NetUtil.newURI("http://example.com/hello-bookmark"),
    title: "hello bookmark",
  });

  
  
  yield check_autocomplete({
    search: "hello",
    matches: [
      {
        uri: NetUtil.newURI("http://example.com/hello-visit"),
        title: "hello visit",
      },
      {
        uri: NetUtil.newURI("http://example.com/hello-bookmark"),
        title: "hello bookmark",
        style: ["bookmark"],
      },
      {
        uri: makeActionURI(("searchengine"), {
          engineName: ENGINE_NAME,
          input: "hello foo",
          searchQuery: "hello",
          searchSuggestion: "hello foo",
        }),
        title: ENGINE_NAME,
        style: ["action", "searchengine"],
        icon: "",
      },
      {
        uri: makeActionURI(("searchengine"), {
          engineName: ENGINE_NAME,
          input: "hello bar",
          searchQuery: "hello",
          searchSuggestion: "hello bar",
        }),
        title: ENGINE_NAME,
        style: ["action", "searchengine"],
        icon: "",
      },
    ],
  });

  
  yield check_autocomplete({
    search: SUGGEST_RESTRICT_TOKEN + " hello",
    matches: [
      {
        uri: makeActionURI(("searchengine"), {
          engineName: ENGINE_NAME,
          input: "hello foo",
          searchQuery: "hello",
          searchSuggestion: "hello foo",
        }),
        title: ENGINE_NAME,
        style: ["action", "searchengine"],
        icon: "",
      },
      {
        uri: makeActionURI(("searchengine"), {
          engineName: ENGINE_NAME,
          input: "hello bar",
          searchQuery: "hello",
          searchSuggestion: "hello bar",
        }),
        title: ENGINE_NAME,
        style: ["action", "searchengine"],
        icon: "",
      }
    ],
  });

  yield cleanUpSuggestions();
});
