
























































































































 
function PROT_BrowserView(tabWatcher, doc) {
  this.debugZone = "browserview";
  this.tabWatcher_ = tabWatcher;
  this.doc_ = doc;
}










PROT_BrowserView.prototype.getFirstUnhandledDocWithURL_ = function(url) {
  var docs = this.tabWatcher_.getDocumentsFromURL(url);
  if (!docs.length)
    return null;

  for (var i = 0; i < docs.length; i++) {
    
    
    if (docs[i].defaultView.top != docs[i].defaultView)
      continue;

    var browser = this.tabWatcher_.getBrowserFromDocument(docs[i]);
    G_Assert(this, !!browser, "Found doc but can't find browser???");
    var alreadyHandled = this.getProblem_(docs[i], browser);

    if (!alreadyHandled)
      return docs[i];
  }
  return null;
}















PROT_BrowserView.prototype.tryToHandleProblemRequest = function(warden,
                                                                request) {

  var doc = this.getFirstUnhandledDocWithURL_(request.name);
  if (doc) {
    var browser = this.tabWatcher_.getBrowserFromDocument(doc);
    G_Assert(this, !!browser, "Couldn't get browser from problem doc???");
    G_Assert(this, !this.getProblem_(doc, browser),
             "Doc is supposedly unhandled, but has state?");
    
    this.isProblemDocument_(browser, doc, warden);
    return true;
  }
  return false;
}











PROT_BrowserView.prototype.isProblemDocument_ = function(browser, 
                                                         doc, 
                                                         warden) {

  G_Debug(this, "Document is problem: " + doc.location.href);
 
  var url = doc.location.href;

  
  var displayer = new warden.displayers_["afterload"]("Phishing afterload",
                                                      browser,
                                                      this.doc_,
                                                      url);

  
  

  var hideHandler = BindToObject(this.onNavAwayFromProblem_, 
                                 this, 
                                 doc, 
                                 browser);
  doc.defaultView.addEventListener("pagehide", hideHandler, true);

  
  var problem = {
    "browser_": browser,
    "doc_": doc,
    "displayer_": displayer,
    "url_": url,
    "hideHandler_": hideHandler,
  };
  var numInQueue = this.queueProblem_(browser, problem);

  
  if (numInQueue == 1)
    new G_Alarm(BindToObject(this.unqueueNextProblem_, this, browser), 0);
}









PROT_BrowserView.prototype.onNavAwayFromProblem_ = function(doc, browser) {

  var problem = this.getProblem_(doc, browser);
  
  
  var message = problem.displayer_.messageShowing_ ? "phishnavaway"
                                                   : "ignorenavaway";
  G_Debug(this, "User nav'd away from problem: " + message);
  (new PROT_Reporter).report(message, problem.url_);

  G_Assert(this, doc === problem.doc_, "State doc not equal to nav away doc?");
  G_Assert(this, browser === problem.browser_, 
           "State browser not equal to nav away browser?");
  
  this.problemResolved(browser, doc);
}







PROT_BrowserView.prototype.hasProblem = function(browser) {
  return this.hasNonemptyProblemQueue_(browser);
}







PROT_BrowserView.prototype.hasNonemptyProblemQueue_ = function(browser) {
  try {
    return !!browser.PROT_problemState__ && 
      !!browser.PROT_problemState__.length;
  } catch(e) {
    
    
    
    
    return false;
  }
}












PROT_BrowserView.prototype.problemResolved = function(browser, opt_doc) {
  var problem;
  var doc;
  if (!!opt_doc) {
    doc = opt_doc;
    problem = this.getProblem_(doc, browser);
  } else {
    problem = this.getCurrentProblem_(browser);
    doc = problem.doc_;
  }

  problem.displayer_.done();
  var wasHead = this.deleteProblemFromQueue_(doc, browser);

  
  var queueNotEmpty = this.getCurrentProblem_(browser);

  if (wasHead && queueNotEmpty) {
    G_Debug(this, "More problems pending. Scheduling unqueue.");
    new G_Alarm(BindToObject(this.unqueueNextProblem_, this, browser), 0);
  }
}








PROT_BrowserView.prototype.unqueueNextProblem_ = function(browser) {
  var problem = this.getCurrentProblem_(browser);
  if (!problem) {
    G_Debug(this, "No problem in queue; doc nav'd away from? (shrug)");
    return;
  }

  
  
  
  if (!problem.displayer_.isActive()) {

    
    
    
    
    
    
    
    
    var haveContent = false;
    try {
      
      var h = problem.doc_.defaultView.getComputedStyle(problem.doc_.body, "")
              .getPropertyValue("height");
      G_Debug(this, "body height: " + h);

      if (Number(h.substring(0, h.length - 2)))
        haveContent = true;

    } catch (e) {
      G_Debug(this, "Masked in unqueuenextproblem: " + e);
    }
    
    if (!haveContent) {

      G_Debug(this, "Didn't get computed style. Re-queueing.");

      
      
      
      
      var p = this.removeProblemFromQueue_(problem.doc_, browser);
      G_Assert(this, p === problem, "Unqueued wrong problem?");
      this.queueProblem_(browser, problem);

      
      
      
      
      
      new G_Alarm(BindToObject(this.unqueueNextProblem_, 
                               this, 
                               browser),
                  200 );
      return;
    }

    problem.displayer_.start();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (this.tabWatcher_.getCurrentBrowser() === browser)
      new G_Alarm(BindToObject(this.problemBrowserMaybeSelected, 
                               this, 
                               browser),
                  350 );
  }
}













PROT_BrowserView.prototype.queueProblem_ = function(browser, problem) {
  G_Debug(this, "Adding problem state for " + problem.url_);

  if (this.hasNonemptyProblemQueue_(browser))
    G_Debug(this, "Already has problem state. Queueing this problem...");

  
  if (browser.PROT_problemState__ == undefined)
    browser.PROT_problemState__ = [];

  browser.PROT_problemState__.push(problem);
  return browser.PROT_problemState__.length;
}













PROT_BrowserView.prototype.deleteProblemFromQueue_ = function(doc, browser) {
  G_Debug(this, "Deleting problem state for " + browser);
  G_Assert(this, !!this.hasNonemptyProblemQueue_(browser),
           "Browser has no problem state");

  var problem = this.getProblem_(doc, browser);
  G_Assert(this, !!problem, "Couldnt find state in removeproblemstate???");

  var wasHead = browser.PROT_problemState__[0] === problem;
  this.removeProblemFromQueue_(doc, browser);

  var hideHandler = problem.hideHandler_;
  G_Assert(this, !!hideHandler, "No hidehandler in state?");
  problem.doc_.defaultView.removeEventListener("pagehide",
                                               hideHandler,
                                               true);
  return wasHead;
}













PROT_BrowserView.prototype.removeProblemFromQueue_ = function(doc, browser) {
  G_Debug(this, "Removing problem state for " + browser);
  G_Assert(this, !!this.hasNonemptyProblemQueue_(browser),
           "Browser has no problem state");

  var problem = null;
  
  for (var i = 0; i < browser.PROT_problemState__.length; i++)
    if (browser.PROT_problemState__[i].doc_ === doc) {
      problem = browser.PROT_problemState__.splice(i, 1)[0];
      break;
    }
  return problem;
}











PROT_BrowserView.prototype.getProblem_ = function(doc, browser) {
  if (!this.hasNonemptyProblemQueue_(browser))
    return null;

  
  for (var i = 0; i < browser.PROT_problemState__.length; i++)
    if (browser.PROT_problemState__[i].doc_ === doc)
      return browser.PROT_problemState__[i];
  return null;
}









PROT_BrowserView.prototype.getCurrentProblem_ = function(browser) {
  return browser.PROT_problemState__[0];
}







PROT_BrowserView.prototype.problemBrowserUnselected = function(browser) {
  var problem = this.getCurrentProblem_(browser);
  G_Assert(this, !!problem, "Couldn't get state from browser");
  problem.displayer_.browserUnselected();
}







PROT_BrowserView.prototype.problemBrowserMaybeSelected = function(browser) {
  var problem = this.getCurrentProblem_(browser);

  if (this.tabWatcher_.getCurrentBrowser() === browser &&
      problem &&
      problem.displayer_.isActive()) 
    this.problemBrowserSelected(browser);
}






PROT_BrowserView.prototype.problemBrowserSelected = function(browser) {
  G_Debug(this, "Problem browser selected");
  var problem = this.getCurrentProblem_(browser);
  G_Assert(this, !!problem, "No state? But we're selected!");
  problem.displayer_.browserSelected();
}









PROT_BrowserView.prototype.acceptAction = function(browser) {
  var problem = this.getCurrentProblem_(browser);

  
  
  
  

  new G_Alarm(BindToObject(problem.displayer_.acceptAction, 
                           problem.displayer_), 
              0);
}









PROT_BrowserView.prototype.declineAction = function(browser) {
  var problem = this.getCurrentProblem_(browser);
  G_Assert(this, !!problem, "User declined but no state???");

  
  
  
  

  new G_Alarm(BindToObject(problem.displayer_.declineAction, 
                           problem.displayer_), 
              0);
}








PROT_BrowserView.prototype.explicitShow = function(browser) {
  var problem = this.getCurrentProblem_(browser);
  G_Assert(this, !!problem, "Explicit show on browser w/o problem state???");
  problem.displayer_.explicitShow();
}
