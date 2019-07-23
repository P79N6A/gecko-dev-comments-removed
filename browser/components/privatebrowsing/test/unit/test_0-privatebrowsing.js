










































function run_test_on_service() {
  
  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);

  
  do_check_true(PRIVATEBROWSING_CONTRACT_ID in Cc);

  
  do_check_true("nsIPrivateBrowsingService" in Ci);

  
  try {
    var pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
             getService(Ci.nsIPrivateBrowsingService);
  } catch (ex) {
    LOG("exception thrown when trying to get the service: " + ex);
    do_throw("private browsing service could not be initialized");
  }

  
  do_check_false(pb.privateBrowsingEnabled);
  
  do_check_false(pb.autoStarted);

  
  pb.privateBrowsingEnabled = true;
  do_check_true(pb.privateBrowsingEnabled);
  do_check_false(pb.autoStarted);
  pb.privateBrowsingEnabled = false;
  do_check_false(pb.privateBrowsingEnabled);
  do_check_false(pb.autoStarted);

  
  var observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == kPrivateBrowsingNotification)
        this.data = aData;
    },
    data: null
  };
  os.addObserver(observer, kPrivateBrowsingNotification, false);
  pb.privateBrowsingEnabled = true;
  do_check_eq(observer.data, kEnter);
  pb.privateBrowsingEnabled = false;
  do_check_eq(observer.data, kExit);
  os.removeObserver(observer, kPrivateBrowsingNotification);

  
  observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == kPrivateBrowsingNotification) {
        try {
          pb.privateBrowsingEnabled = (aData == kEnter);
          do_throw("Setting privateBrowsingEnabled inside the " + aData +
            " notification should throw");
        } catch (ex) {
          if (!("result" in ex && ex.result == Cr.NS_ERROR_FAILURE))
            do_throw("Unexpected exception caught: " + ex);
        }
      }
    }
  };
  os.addObserver(observer, kPrivateBrowsingNotification, false);
  pb.privateBrowsingEnabled = true;
  do_check_true(pb.privateBrowsingEnabled); 
  pb.privateBrowsingEnabled = false;
  do_check_false(pb.privateBrowsingEnabled); 
  os.removeObserver(observer, kPrivateBrowsingNotification);

  
  observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == kPrivateBrowsingNotification) {
        try {
          var dummy = pb.privateBrowsingEnabled;
          if (aData == kEnter)
            do_check_true(dummy);
          else if (aData == kExit)
            do_check_false(dummy);
        } catch (ex) {
          do_throw("Unexpected exception caught: " + ex);
        }
      }
    }
  };
  os.addObserver(observer, kPrivateBrowsingNotification, false);
  pb.privateBrowsingEnabled = true;
  do_check_true(pb.privateBrowsingEnabled); 
  pb.privateBrowsingEnabled = false;
  do_check_false(pb.privateBrowsingEnabled); 
  os.removeObserver(observer, kPrivateBrowsingNotification);

  
  
  observer = {
    observe: function(aSubject, aTopic, aData) {
      switch (aTopic) {
      case kPrivateBrowsingCancelVoteNotification:
      case kPrivateBrowsingNotification:
        this.notifications.push(aTopic + " " + aData);
      }
    },
    notifications: []
  };
  os.addObserver(observer, kPrivateBrowsingCancelVoteNotification, false);
  os.addObserver(observer, kPrivateBrowsingNotification, false);
  pb.privateBrowsingEnabled = true;
  do_check_true(pb.privateBrowsingEnabled); 
  pb.privateBrowsingEnabled = false;
  do_check_false(pb.privateBrowsingEnabled); 
  os.removeObserver(observer, kPrivateBrowsingNotification);
  os.removeObserver(observer, kPrivateBrowsingCancelVoteNotification);
  var reference_order = [
      kPrivateBrowsingCancelVoteNotification + " " + kEnter,
      kPrivateBrowsingNotification + " " + kEnter,
      kPrivateBrowsingCancelVoteNotification + " " + kExit,
      kPrivateBrowsingNotification + " " + kExit
    ];
  do_check_eq(observer.notifications.join(","), reference_order.join(","));

  
  
  observer = {
    observe: function(aSubject, aTopic, aData) {
      switch (aTopic) {
      case kPrivateBrowsingCancelVoteNotification:
        do_check_neq(aSubject, null);
        try {
          aSubject.QueryInterface(Ci.nsISupportsPRBool);
        } catch (ex) {
          do_throw("aSubject in " + kPrivateBrowsingCancelVoteNotification +
            " should implement nsISupportsPRBool");
        }
        do_check_false(aSubject.data);
        aSubject.data = true; 

        
      case kPrivateBrowsingNotification:
        this.notifications.push(aTopic + " " + aData);
      }
    },
    nextPhase: function() {
      this.notifications.push("enter phase " + (++this._phase));
    },
    notifications: [],
    _phase: 0
  };
  os.addObserver(observer, kPrivateBrowsingCancelVoteNotification, false);
  os.addObserver(observer, kPrivateBrowsingNotification, false);
  pb.privateBrowsingEnabled = true;
  do_check_false(pb.privateBrowsingEnabled); 
  
  os.removeObserver(observer, kPrivateBrowsingCancelVoteNotification);
  observer.nextPhase();
  pb.privateBrowsingEnabled = true; 
  do_check_true(pb.privateBrowsingEnabled); 
  
  os.addObserver(observer, kPrivateBrowsingCancelVoteNotification, false);
  pb.privateBrowsingEnabled = false;
  do_check_true(pb.privateBrowsingEnabled); 
  os.removeObserver(observer, kPrivateBrowsingCancelVoteNotification);
  observer.nextPhase();
  pb.privateBrowsingEnabled = false; 
  do_check_false(pb.privateBrowsingEnabled);
  os.removeObserver(observer, kPrivateBrowsingNotification);
  reference_order = [
      kPrivateBrowsingCancelVoteNotification + " " + kEnter,
      "enter phase 1",
      kPrivateBrowsingNotification + " " + kEnter,
      kPrivateBrowsingCancelVoteNotification + " " + kExit,
      "enter phase 2",
      kPrivateBrowsingNotification + " " + kExit,
    ];
  do_check_eq(observer.notifications.join(","), reference_order.join(","));

  
  
  observer = {
    observe: function(aSubject, aTopic, aData) {
      this.notifications.push(aTopic + " " + aData);
    },
    notifications: []
  };
  os.addObserver(observer, kPrivateBrowsingNotification, false);
  os.addObserver(observer, kPrivateBrowsingTransitionCompleteNotification, false);
  pb.privateBrowsingEnabled = true;
  pb.privateBrowsingEnabled = false;
  os.removeObserver(observer, kPrivateBrowsingNotification);
  os.removeObserver(observer, kPrivateBrowsingTransitionCompleteNotification);
  reference_order = [
    kPrivateBrowsingNotification + " " + kEnter,
    kPrivateBrowsingTransitionCompleteNotification + " ",
    kPrivateBrowsingNotification + " " + kExit,
    kPrivateBrowsingTransitionCompleteNotification + " ",
  ];
  do_check_eq(observer.notifications.join(","), reference_order.join(","));
}


function run_test() {
  run_test_on_all_services();
}
