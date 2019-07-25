









XPCOMUtils.defineLazyServiceGetter(this, "gHistory",
                                   "@mozilla.org/browser/history;1",
                                   "mozIAsyncHistory");

XPCOMUtils.defineLazyServiceGetter(this, "gGlobalHistory",
                                   "@mozilla.org/browser/nav-history-service;1",
                                   "nsIGlobalHistory2");

const TEST_DOMAIN = "http://mozilla.org/";
const TOPIC_UPDATEPLACES_COMPLETE = "places-updatePlaces-complete";
const URI_VISIT_SAVED = "uri-visit-saved";
const RECENT_EVENT_THRESHOLD = 15 * 60 * 1000000;













function VisitInfo(aTransitionType,
                   aVisitTime)
{
  this.transitionType =
    aTransitionType === undefined ? TRANSITION_LINK : aTransitionType;
  this.visitDate = aVisitTime || Date.now() * 1000;
}





function NavHistoryObserver()
{
}
NavHistoryObserver.prototype =
{
  onBeginUpdateBatch: function() { },
  onEndUpdateBatch: function() { },
  onVisit: function() { },
  onTitleChanged: function() { },
  onBeforeDeleteURI: function() { },
  onDeleteURI: function() { },
  onClearHistory: function() { },
  onPageChanged: function() { },
  onDeleteVisits: function() { },
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavHistoryObserver,
  ]),
};












function TitleChangedObserver(aURI,
                              aExpectedTitle,
                              aCallback)
{
  this.uri = aURI;
  this.expectedTitle = aExpectedTitle;
  this.callback = aCallback;
}
TitleChangedObserver.prototype = {
  __proto__: NavHistoryObserver.prototype,
  onTitleChanged: function(aURI,
                           aTitle)
  {
    do_log_info("onTitleChanged(" + aURI.spec + ", " + aTitle + ")");
    if (!this.uri.equals(aURI)) {
      return;
    }
    do_check_eq(aTitle, this.expectedTitle);
    this.callback();
  },
};










function VisitObserver(aURI,
                       aGUID,
                       aCallback)
{
  this.uri = aURI;
  this.guid = aGUID;
  this.callback = aCallback;
}
VisitObserver.prototype = {
  __proto__: NavHistoryObserver.prototype,
  onVisit: function(aURI,
                    aVisitId,
                    aTime,
                    aSessionId,
                    aReferringId,
                    aTransitionType,
                    aGUID)
  {
    do_log_info("onVisit(" + aURI.spec + ", " + aVisitId + ", " + aTime +
                ", " + aSessionId + ", " + aReferringId + ", " +
                aTransitionType + ", " + aGUID + ")"); 
    if (!this.uri.equals(aURI) || this.guid != aGUID) {
      return;
    }
    this.callback(aTime, aTransitionType);
  },
};









function do_check_title_for_uri(aURI,
                                aTitle)
{
  let stack = Components.stack.caller;
  let stmt = DBConn().createStatement(
    "SELECT title " +
    "FROM moz_places " +
    "WHERE url = :url "
  );
  stmt.params.url = aURI.spec;
  do_check_true(stmt.executeStep(), stack);
  do_check_eq(stmt.row.title, aTitle, stack);
  stmt.finalize();
}




function test_interface_exists()
{
  let history = Cc["@mozilla.org/browser/history;1"].getService(Ci.nsISupports);
  do_check_true(history instanceof Ci.mozIAsyncHistory);
  run_next_test();
}

function test_invalid_uri_throws()
{
  
  let place = {
    visits: [
      new VisitInfo(),
    ],
  };
  try {
    gHistory.updatePlaces(place);
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  
  const TEST_VALUES = [
    null,
    undefined,
    {},
    [],
    TEST_DOMAIN + "test_invalid_id_throws",
  ];
  for (let i = 0; i < TEST_VALUES.length; i++) {
    place.uri = TEST_VALUES[i];
    try {
      gHistory.updatePlaces(place);
      do_throw("Should have thrown!");
    }
    catch (e) {
      do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
    }
  }
  run_next_test();
}

function test_invalid_places_throws()
{
  
  try {
    gHistory.updatePlaces();
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_XPC_NOT_ENOUGH_ARGS);
  }

  
  const TEST_VALUES = [
    null,
    undefined,
    {},
    [],
    "",
  ];
  for (let i = 0; i < TEST_VALUES.length; i++) {
    let value = TEST_VALUES[i];
    try {
      gHistory.updatePlaces(value);
      do_throw("Should have thrown!");
    }
    catch (e) {
      do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
    }
  }

  run_next_test();
}

function test_invalid_guid_throws()
{
  
  let place = {
    guid: "BAD_GUID",
    uri: NetUtil.newURI(TEST_DOMAIN + "test_invalid_guid_throws"),
    visits: [
      new VisitInfo(),
    ],
  };
  try {
    gHistory.updatePlaces(place);
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  
  place.guid = "__BADGUID+__";
  do_check_eq(place.guid.length, 12);
  try {
    gHistory.updatePlaces(place);
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_no_visits_throws()
{
  const TEST_URI =
    NetUtil.newURI(TEST_DOMAIN + "test_no_id_or_guid_no_visits_throws");
  const TEST_GUID = "_RANDOMGUID_";
  const TEST_PLACEID = 2;

  let log_test_conditions = function(aPlace) {
    let str = "Testing place with " +
      (aPlace.uri ? "uri" : "no uri") + ", " +
      (aPlace.guid ? "guid" : "no guid") + ", " +
      (aPlace.visits ? "visits array" : "no visits array");
    do_log_info(str);
  };

  
  
  
  let place = { };
  for (let uri = 1; uri >= 0; uri--) {
    place.uri = uri ? TEST_URI : undefined;

    for (let guid = 1; guid >= 0; guid--) {
      place.guid = guid ? TEST_GUID : undefined;

      for (let visits = 1; visits >= 0; visits--) {
        place.visits = visits ? [] : undefined;

        log_test_conditions(place);
        try {
          gHistory.updatePlaces(place);
          do_throw("Should have thrown!");
        }
        catch (e) {
          do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
        }
      }
    }
  }

  run_next_test();
}

function test_add_visit_no_date_throws()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_add_visit_no_date_throws"),
    visits: [
      new VisitInfo(),
    ],
  };
  delete place.visits[0].visitDate;
  try {
    gHistory.updatePlaces(place);
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_add_visit_no_transitionType_throws()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_add_visit_no_transitionType_throws"),
    visits: [
      new VisitInfo(),
    ],
  };
  delete place.visits[0].transitionType;
  try {
    gHistory.updatePlaces(place);
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_add_visit_invalid_transitionType_throws()
{
  
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN +
                        "test_add_visit_invalid_transitionType_throws"),
    visits: [
      new VisitInfo(TRANSITION_LINK - 1),
    ],
  };
  try {
    gHistory.updatePlaces(place);
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  
  place.visits[0] = new VisitInfo(TRANSITION_FRAMED_LINK + 1);
  try {
    gHistory.updatePlaces(place);
    do_throw("Should have thrown!");
  }
  catch (e) {
    do_check_eq(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  run_next_test();
}

function test_non_addable_uri_errors()
{
  
  const URLS = [
    "about:config",
    "imap://cyrus.andrew.cmu.edu/archive.imap",
    "news://new.mozilla.org/mozilla.dev.apps.firefox",
    "mailbox:Inbox",
    "moz-anno:favicon:http://mozilla.org/made-up-favicon",
    "view-source:http://mozilla.org",
    "chrome://browser/content/browser.xul",
    "resource://gre-resources/hiddenWindow.html",
    "data:,Hello%2C%20World!",
    "wyciwyg:/0/http://mozilla.org",
    "javascript:alert('hello wolrd!');",
  ];
  let places = [];
  URLS.forEach(function(url) {
    try {
      let place = {
        uri: NetUtil.newURI(url),
        title: "test for " + url,
        visits: [
          new VisitInfo(),
        ],
      };
      places.push(place);
    }
    catch (e if e.result === Cr.NS_ERROR_FAILURE) {
      
      
      
      do_log_info("Could not construct URI for '" + url + "'; ignoring");
    }
  });

  let callbackCount = 0;
  gHistory.updatePlaces(places, function(aResultCode, aPlaceInfo) {
    do_log_info("Checking '" + aPlaceInfo.uri.spec + "'");
    do_check_eq(aResultCode, Cr.NS_ERROR_INVALID_ARG);
    do_check_false(gGlobalHistory.isVisited(aPlaceInfo.uri));

    
    if (++callbackCount == places.length) {
      waitForAsyncUpdates(run_next_test);
    }
  });
}

function test_duplicate_guid_errors()
{
  
  
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_duplicate_guid_fails_first"),
    visits: [
      new VisitInfo(),
    ],
  };

  do_check_false(gGlobalHistory.isVisited(place.uri));
  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    do_check_true(gGlobalHistory.isVisited(place.uri));

    let badPlace = {
      uri: NetUtil.newURI(TEST_DOMAIN + "test_duplicate_guid_fails_second"),
      visits: [
        new VisitInfo(),
      ],
      guid: aPlaceInfo.guid,
    };

    do_check_false(gGlobalHistory.isVisited(badPlace.uri));
    gHistory.updatePlaces(badPlace, function(aResultCode, aPlaceInfo) {
      do_check_eq(aResultCode, Cr.NS_ERROR_STORAGE_CONSTRAINT);
      do_check_false(gGlobalHistory.isVisited(badPlace.uri));

      waitForAsyncUpdates(run_next_test);
    });
  });
}

function test_invalid_referrerURI_ignored()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN +
                        "test_invalid_referrerURI_ignored"),
    visits: [
      new VisitInfo(),
    ],
  };
  place.visits[0].referrerURI = NetUtil.newURI(place.uri.spec + "_unvisistedURI");
  do_check_false(gGlobalHistory.isVisited(place.uri));
  do_check_false(gGlobalHistory.isVisited(place.visits[0].referrerURI));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));

    
    do_check_false(gGlobalHistory.isVisited(place.visits[0].referrerURI));

    
    let stmt = DBConn().createStatement(
      "SELECT from_visit " +
      "FROM moz_historyvisits " +
      "WHERE id = :visit_id"
    );
    stmt.params.visit_id = aPlaceInfo.visits[0].visitId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.from_visit, 0);
    stmt.finalize();

    waitForAsyncUpdates(run_next_test);
  });
}

function test_nonnsIURI_referrerURI_ignored()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN +
                        "test_nonnsIURI_referrerURI_ignored"),
    visits: [
      new VisitInfo(),
    ],
  };
  place.visits[0].referrerURI = place.uri.spec + "_nonnsIURI";
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));

    
    let stmt = DBConn().createStatement(
      "SELECT from_visit " +
      "FROM moz_historyvisits " +
      "WHERE id = :visit_id"
    );
    stmt.params.visit_id = aPlaceInfo.visits[0].visitId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.from_visit, 0);
    stmt.finalize();

    waitForAsyncUpdates(run_next_test);
  });
}

function test_invalid_sessionId_ignored()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN +
                        "test_invalid_sessionId_ignored"),
    visits: [
      new VisitInfo(),
    ],
  };
  place.visits[0].sessionId = -1;
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));

    
    let visit = aPlaceInfo.visits[0];
    do_check_neq(visit.sessionId, place.visits[0].sessionId);

    
    let stmt = DBConn().createStatement(
      "SELECT session " +
      "FROM moz_historyvisits " +
      "WHERE id = :visit_id"
    );
    stmt.params.visit_id = visit.visitId;
    do_check_true(stmt.executeStep());
    do_check_neq(stmt.row.session, place.visits[0].sessionId);
    stmt.finalize();

    waitForAsyncUpdates(run_next_test);
  });
}

function test_unstored_sessionId_ignored()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN +
                        "test_unstored_sessionId_ignored"),
    visits: [
      new VisitInfo(),
    ],
  };

  
  let stmt = DBConn().createStatement(
    "SELECT MAX(session) as max_session " +
    "FROM moz_historyvisits"
  );
  do_check_true(stmt.executeStep());
  let maxSessionId = stmt.row.max_session;
  stmt.finalize();

  
  place.visits[0].sessionId = maxSessionId + 10;
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));

    
    let visit = aPlaceInfo.visits[0];
    do_check_neq(visit.sessionId, place.visits[0].sessionId);

    
    let stmt = DBConn().createStatement(
      "SELECT MAX(session) as max_session " +
      "FROM moz_historyvisits"
    );
    do_check_true(stmt.executeStep());

    
    
    let newMaxSessionId = stmt.row.max_session;
    do_check_eq(maxSessionId + 1, newMaxSessionId);
    stmt.finalize();

    waitForAsyncUpdates(run_next_test);
  });
}


function test_old_referrer_ignored()
{
  
  
  
  let oldTime = (Date.now() * 1000) - (RECENT_EVENT_THRESHOLD + 1);
  let referrerPlace = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_old_referrer_ignored_referrer"),
    visits: [
      new VisitInfo(TRANSITION_LINK, oldTime),
    ],
  };

  
  
  do_check_false(gGlobalHistory.isVisited(referrerPlace.uri));
  gHistory.updatePlaces(referrerPlace, function(aResultCode, aPlaceInfo) {
    
    
    
    do_check_true(Components.isSuccessCode(aResultCode));
    do_check_true(gGlobalHistory.isVisited(referrerPlace.uri));

    let visitInfo = new VisitInfo();
    visitInfo.referrerURI = referrerPlace.uri;
    let place = {
      uri: NetUtil.newURI(TEST_DOMAIN + "test_old_referrer_ignored_page"),
      visits: [
        visitInfo,
      ],
    };

    do_check_false(gGlobalHistory.isVisited(place.uri));
    gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
      do_check_true(Components.isSuccessCode(aResultCode));
      do_check_true(gGlobalHistory.isVisited(place.uri));

      
      
      do_check_eq(aPlaceInfo.visits[0].referrerURI, null);
      let stmt = DBConn().createStatement(
        "SELECT COUNT(1) AS count " +
        "FROM moz_historyvisits " +
        "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) " +
        "AND from_visit = 0 "
      );
      stmt.params.page_url = place.uri.spec;
      do_check_true(stmt.executeStep());
      do_check_eq(stmt.row.count, 1);
      stmt.finalize();

      waitForAsyncUpdates(run_next_test);
    });
  });
}

function test_place_id_ignored()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_place_id_ignored_first"),
    visits: [
      new VisitInfo(),
    ],
  };

  do_check_false(gGlobalHistory.isVisited(place.uri));
  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    do_check_true(gGlobalHistory.isVisited(place.uri));

    let placeId = aPlaceInfo.placeId;
    do_check_neq(placeId, 0);

    let badPlace = {
      uri: NetUtil.newURI(TEST_DOMAIN + "test_place_id_ignored_second"),
      visits: [
        new VisitInfo(),
      ],
      placeId: placeId,
    };

    do_check_false(gGlobalHistory.isVisited(badPlace.uri));
    gHistory.updatePlaces(badPlace, function(aResultCode, aPlaceInfo) {
      do_check_true(Components.isSuccessCode(aResultCode));

      do_check_neq(aPlaceInfo.placeId, placeId);
      do_check_true(gGlobalHistory.isVisited(badPlace.uri));

      waitForAsyncUpdates(run_next_test);
    });
  });
}

function test_observer_topic_dispatched_when_complete()
{
  
  
  
  let places = [
    { uri: NetUtil.newURI(TEST_DOMAIN +
                          "test_observer_topic_dispatched_when_complete"),
      visits: [
        new VisitInfo(),
        new VisitInfo(TRANSITION_EMBED),
      ],
    },
    { uri: NetUtil.newURI("data:,Hello%2C%20World!"),
      visits: [
        new VisitInfo(),
      ],
    },
  ];
  do_check_false(gGlobalHistory.isVisited(places[0].uri));
  do_check_false(gGlobalHistory.isVisited(places[1].uri));

  const EXPECTED_COUNT = 3;
  let callbackCount = 0;

  gHistory.updatePlaces(places, function(aResultCode, aPlaceInfo) {
    let checker = PlacesUtils.history.canAddURI(aPlaceInfo.uri) ?
      do_check_true : do_check_false;
    checker(Components.isSuccessCode(aResultCode));
    callbackCount++;
  });

  let observer = {
    observe: function(aSubject, aTopic, aData)
    {
      do_check_eq(aTopic, TOPIC_UPDATEPLACES_COMPLETE);
      do_check_eq(callbackCount, EXPECTED_COUNT);
      Services.obs.removeObserver(observer, TOPIC_UPDATEPLACES_COMPLETE);
      waitForAsyncUpdates(run_next_test);
    },
  };
  Services.obs.addObserver(observer, TOPIC_UPDATEPLACES_COMPLETE, false);
}

function test_add_visit()
{
  const VISIT_TIME = Date.now() * 1000;
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_add_visit"),
    title: "test_add_visit title",
    visits: [],
  };
  for (let transitionType = TRANSITION_LINK;
       transitionType <= TRANSITION_FRAMED_LINK;
       transitionType++) {
    place.visits.push(new VisitInfo(transitionType, VISIT_TIME));
  }
  do_check_false(gGlobalHistory.isVisited(place.uri));

  let callbackCount = 0;
  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    do_check_true(gGlobalHistory.isVisited(place.uri));

    
    do_check_true(place.uri.equals(aPlaceInfo.uri));
    do_check_eq(aPlaceInfo.frecency, -1); 
    do_check_eq(aPlaceInfo.title, place.title);

    
    let visits = aPlaceInfo.visits;
    do_check_eq(visits.length, 1);
    let visit = visits[0];
    do_check_eq(visit.visitDate, VISIT_TIME);
    do_check_true(visit.transitionType >= TRANSITION_LINK &&
                  visit.transitionType <= TRANSITION_FRAMED_LINK);
    do_check_true(visit.referrerURI === null);

    
    
    if (visit.transitionType == TRANSITION_EMBED) {
      
      do_check_eq(aPlaceInfo.placeId, 0);
      do_check_eq(aPlaceInfo.guid, null);

      
      do_check_eq(visit.visitId, 0);
      do_check_eq(visit.sessionId, 0);
    }
    
    else {
      
      do_check_true(aPlaceInfo.placeId > 0);
      do_check_valid_places_guid(aPlaceInfo.guid);

      
      do_check_true(visit.visitId > 0);
      do_check_true(visit.sessionId > 0);
    }

    
    if (++callbackCount == place.visits.length) {
      waitForAsyncUpdates(run_next_test);
    }
  });
}

function test_properties_saved()
{
  
  let places = [];
  for (let transitionType = TRANSITION_LINK;
       transitionType <= TRANSITION_FRAMED_LINK;
       transitionType++) {
    let place = {
      uri: NetUtil.newURI(TEST_DOMAIN + "test_properties_saved/" +
                          transitionType),
      title: "test_properties_saved test",
      visits: [
        new VisitInfo(transitionType),
      ],
    };
    do_check_false(gGlobalHistory.isVisited(place.uri));
    places.push(place);
  }

  let callbackCount = 0;
  gHistory.updatePlaces(places, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));
    let visit = aPlaceInfo.visits[0];
    print("TEST-INFO | test_properties_saved | updatePlaces callback for " +
          "transition type " + visit.transitionType);

    
    const EXPECTED_COUNT = visit.transitionType == TRANSITION_EMBED ? 0 : 1;

    
    let stmt = DBConn().createStatement(
      "SELECT COUNT(1) AS count " +
      "FROM moz_places h " +
      "JOIN moz_historyvisits v " +
      "ON h.id = v.place_id " +
      "WHERE h.url = :page_url " +
      "AND v.visit_date = :visit_date "
    );
    stmt.params.page_url = uri.spec;
    stmt.params.visit_date = visit.visitDate;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.count, EXPECTED_COUNT);
    stmt.finalize();

    
    stmt = DBConn().createStatement(
      "SELECT COUNT(1) AS count " +
      "FROM moz_places h " +
      "JOIN moz_historyvisits v " +
      "ON h.id = v.place_id " +
      "WHERE h.url = :page_url " +
      "AND v.visit_type = :transition_type "
    );
    stmt.params.page_url = uri.spec;
    stmt.params.transition_type = visit.transitionType;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.count, EXPECTED_COUNT);
    stmt.finalize();

    
    stmt = DBConn().createStatement(
      "SELECT COUNT(1) AS count " +
      "FROM moz_places h " +
      "JOIN moz_historyvisits v " +
      "ON h.id = v.place_id " +
      "WHERE h.url = :page_url " +
      "AND v.session = :session_id "
    );
    stmt.params.page_url = uri.spec;
    stmt.params.session_id = visit.sessionId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.count, EXPECTED_COUNT);
    stmt.finalize();

    
    stmt = DBConn().createStatement(
      "SELECT COUNT(1) AS count " +
      "FROM moz_places h " +
      "WHERE h.url = :page_url " +
      "AND h.title = :title "
    );
    stmt.params.page_url = uri.spec;
    stmt.params.title = aPlaceInfo.title;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.count, EXPECTED_COUNT);
    stmt.finalize();

    
    if (++callbackCount == places.length) {
      waitForAsyncUpdates(run_next_test);
    }
  });
}

function test_guid_saved()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_guid_saved"),
    guid: "__TESTGUID__",
    visits: [
      new VisitInfo(),
    ],
  };
  do_check_valid_places_guid(place.guid);
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));
    do_check_eq(aPlaceInfo.guid, place.guid);
    do_check_guid_for_uri(uri, place.guid);

    waitForAsyncUpdates(run_next_test);
  });
}

function test_referrer_saved()
{
  let places = [
    { uri: NetUtil.newURI(TEST_DOMAIN + "test_referrer_saved/referrer"),
      visits: [
        new VisitInfo(),
      ],
    },
    { uri: NetUtil.newURI(TEST_DOMAIN + "test_referrer_saved/test"),
      visits: [
        new VisitInfo(),
      ],
    },
  ];
  places[1].visits[0].referrerURI = places[0].uri;
  do_check_false(gGlobalHistory.isVisited(places[0].uri));
  do_check_false(gGlobalHistory.isVisited(places[1].uri));

  let callbackCount = 0;
  let referrerSessionId;
  gHistory.updatePlaces(places, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));
    let visit = aPlaceInfo.visits[0];

    
    if (++callbackCount != places.length) {
      referrerSessionId = visit.sessionId;
      return;
    }

    do_check_true(places[0].uri.equals(visit.referrerURI));
    do_check_eq(visit.sessionId, referrerSessionId);

    let stmt = DBConn().createStatement(
      "SELECT COUNT(1) AS count " +
      "FROM moz_historyvisits " +
      "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) " +
      "AND from_visit = ( " +
        "SELECT id " +
        "FROM moz_historyvisits " +
        "WHERE place_id = (SELECT id FROM moz_places WHERE url = :referrer) " +
      ") "
    );
    stmt.params.page_url = uri.spec;
    stmt.params.referrer = visit.referrerURI.spec;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.count, 1);
    stmt.finalize();

    waitForAsyncUpdates(run_next_test);
  });
}

function test_sessionId_saved()
{
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_sessionId_saved"),
    visits: [
      new VisitInfo(),
    ],
  };
  place.visits[0].sessionId = 3;
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    let uri = aPlaceInfo.uri;
    do_check_true(gGlobalHistory.isVisited(uri));

    let visit = aPlaceInfo.visits[0];
    do_check_eq(visit.sessionId, place.visits[0].sessionId);

    let stmt = DBConn().createStatement(
      "SELECT COUNT(1) AS count " +
      "FROM moz_historyvisits " +
      "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) " +
      "AND session = :session_id "
    );
    stmt.params.page_url = uri.spec;
    stmt.params.session_id = visit.sessionId;
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.row.count, 1);
    stmt.finalize();

    waitForAsyncUpdates(run_next_test);
  });
}

function test_guid_change_saved()
{
  
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_guid_change_saved"),
    visits: [
      new VisitInfo(),
    ],
  };
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));

    
    place.guid = "_GUIDCHANGE_";
    place.visits = [new VisitInfo()];
    gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
      do_check_true(Components.isSuccessCode(aResultCode));
      do_check_guid_for_uri(place.uri, place.guid);

      waitForAsyncUpdates(run_next_test);
    });
  });
}

function test_title_change_saved()
{
  
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_title_change_saved"),
    title: "original title",
    visits: [
      new VisitInfo(),
    ],
  };
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));

    
    place.title = "";
    place.visits = [new VisitInfo()];
    gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
      do_check_true(Components.isSuccessCode(aResultCode));
      do_check_title_for_uri(place.uri, null);

      
      place.title = "title change";
      place.visits = [new VisitInfo()];
      gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
        do_check_true(Components.isSuccessCode(aResultCode));
        do_check_title_for_uri(place.uri, place.title);

        
        place.title = null;
        place.visits = [new VisitInfo()];
        gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
          do_check_true(Components.isSuccessCode(aResultCode));
          do_check_title_for_uri(place.uri, place.title);

          waitForAsyncUpdates(run_next_test);
        });
      });
    });
  });
}

function test_no_title_does_not_clear_title()
{
  const TITLE = "test title";
  
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_no_title_does_not_clear_title"),
    title: TITLE,
    visits: [
      new VisitInfo(),
    ],
  };
  do_check_false(gGlobalHistory.isVisited(place.uri));

  gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));

    
    delete place.title;
    place.visits = [new VisitInfo()];
    gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
      do_check_true(Components.isSuccessCode(aResultCode));
      do_check_title_for_uri(place.uri, TITLE);

      waitForAsyncUpdates(run_next_test);
    });
  });
}

function test_title_change_notifies()
{
  
  
  let place = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_title_change_notifies"),
    visits: [
      new VisitInfo(),
    ],
  };
  do_check_false(gGlobalHistory.isVisited(place.uri));

  let silentObserver =
    new TitleChangedObserver(place.uri, "DO NOT WANT", function() {
      do_throw("unexpected callback!");
    });

  PlacesUtils.history.addObserver(silentObserver, false);
  gHistory.updatePlaces(place);

  
  
  
  place.uri = NetUtil.newURI(place.uri.spec + "/new-visit-with-title");
  place.title = "title 1";
  let callbackCount = 0;
  let observer = new TitleChangedObserver(place.uri, place.title, function() {
    switch (++callbackCount) {
      case 1:
        
        
        observer.expectedTitle = place.title = "title 2";
        place.visits = [new VisitInfo()];
        gHistory.updatePlaces(place);
        break;
      case 2:
        PlacesUtils.history.removeObserver(silentObserver);
        PlacesUtils.history.removeObserver(observer);
        waitForAsyncUpdates(run_next_test);
    };
  });
  PlacesUtils.history.addObserver(observer, false);
  gHistory.updatePlaces(place);
}

function test_visit_notifies()
{
  
  
  let place = {
    guid: "abcdefghijkl",
    uri: NetUtil.newURI(TEST_DOMAIN + "test_visit_notifies"),
    visits: [
      new VisitInfo(),
    ],
  };
  do_check_false(gGlobalHistory.isVisited(place.uri));

  let callbackCount = 0;
  let finisher = function() {
    if (++callbackCount == 2) {
      waitForAsyncUpdates(run_next_test);
    }
  }
  let visitObserver = new VisitObserver(place.uri, place.guid,
                                        function(aVisitDate,
                                                 aTransitionType) {
    let visit = place.visits[0];
    do_check_eq(visit.visitDate, aVisitDate);
    do_check_eq(visit.transitionType, aTransitionType);

    PlacesUtils.history.removeObserver(visitObserver);
    finisher();
  });
  PlacesUtils.history.addObserver(visitObserver, false);
  let observer = function(aSubject, aTopic, aData) {
    do_log_info("observe(" + aSubject + ", " + aTopic + ", " + aData + ")");
    do_check_true(aSubject instanceof Ci.nsIURI);
    do_check_true(aSubject.equals(place.uri));

    Services.obs.removeObserver(observer, URI_VISIT_SAVED);
    finisher();
  };
  Services.obs.addObserver(observer, URI_VISIT_SAVED, false);
  gHistory.updatePlaces(place);
}

function test_referrer_sessionId_persists()
{
  
  
  let referrerPlace = {
    uri: NetUtil.newURI(TEST_DOMAIN + "test_referrer_sessionId_persists_ref"),
    visits: [
      new VisitInfo(),
    ],
  };

  
  
  do_check_false(gGlobalHistory.isVisited(referrerPlace.uri));
  gHistory.updatePlaces(referrerPlace, function(aResultCode, aPlaceInfo) {
    do_check_true(Components.isSuccessCode(aResultCode));
    do_check_true(gGlobalHistory.isVisited(referrerPlace.uri));

    let sessionId = aPlaceInfo.visits[0].sessionId;
    do_check_neq(sessionId, null);

    let place = {
      uri: NetUtil.newURI(TEST_DOMAIN + "test_referrer_sessionId_persists"),
      visits: [
        new VisitInfo(),
      ],
    };
    place.visits[0].referrerURI = referrerPlace.uri;

    do_check_false(gGlobalHistory.isVisited(place.uri));
    gHistory.updatePlaces(place, function(aResultCode, aPlaceInfo) {
      do_check_true(Components.isSuccessCode(aResultCode));
      do_check_true(gGlobalHistory.isVisited(place.uri));

      do_check_eq(aPlaceInfo.visits[0].sessionId, sessionId);

      waitForAsyncUpdates(run_next_test);
    });
  });
}




[
  test_interface_exists,
  test_invalid_uri_throws,
  test_invalid_places_throws,
  test_invalid_guid_throws,
  test_no_visits_throws,
  test_add_visit_no_date_throws,
  test_add_visit_no_transitionType_throws,
  test_add_visit_invalid_transitionType_throws,
  
  
  test_non_addable_uri_errors,
  test_duplicate_guid_errors,
  test_invalid_referrerURI_ignored,
  test_nonnsIURI_referrerURI_ignored,
  test_invalid_sessionId_ignored,
  test_unstored_sessionId_ignored,
  test_old_referrer_ignored,
  test_place_id_ignored,
  test_observer_topic_dispatched_when_complete,
  test_add_visit,
  test_properties_saved,
  test_guid_saved,
  test_referrer_saved,
  test_sessionId_saved,
  test_guid_change_saved,
  test_title_change_saved,
  test_no_title_does_not_clear_title,
  test_title_change_notifies,
  test_visit_notifies,
  test_referrer_sessionId_persists,
].forEach(add_test);

function run_test()
{
  run_next_test();
}
