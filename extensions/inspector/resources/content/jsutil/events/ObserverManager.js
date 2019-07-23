














































function ObserverManager(aTarget)
{
  this.mTarget = aTarget;
  this.mObservers = {};
}

ObserverManager.prototype = 
{
  addObserver: function(aEventName, aObserver)
  {
    var list;
    if (!(aEventName in this.mObservers)) {
      list = [];
      this.mObservers[aEventName] = list;
    } else
      list = this.mObservers[aEventName];
      
    
    list.push(aObserver);
  },

  removeObserver: function(aEventName, aObserver)
  {
    if (aEventName in this.mObservers) {
      var list = this.mObservers[aEventName];
      for (var i = 0; i < list.length; ++i) {
        if (list[i] == aObserver) {
          list.splice(i, 1);
          return;
        }
      }
    }
  },

  dispatchEvent: function(aEventName, aEventData)
  {
    if (aEventName in this.mObservers) {
      if (!("target" in aEventData))
        aEventData.target = this.mTarget;
      aEventData.type = aEventName;
      
      var list = this.mObservers[aEventName];
      for (var i = 0; i < list.length; ++i)
        list[i].onEvent(aEventData);
    }
  }

};
