


































function MutationEventChecker()
{
  this.expectedEvents = [];

  this.watchAttr = function(element, attr)
  {
    if (this.attr) {
      this.finish();
    }

    this.expectedEvents = [];
    this.element        = element;
    this.attr           = attr;
    this.oldValue       = element.getAttribute(attr);
    this.giveUp         = false;
    this.ignore         = false;

    this.element.addEventListener('DOMAttrModified', this._listener, false);
  }

  this.expect = function()
  {
    if (this.giveUp) {
      return;
    }

    ok(this.expectedEvents.length == 0,
       "Expecting new events for " + this.attr +
       " but the following previously expected events have still not been " +
       "received: " + this._stillExpecting());
    if (this.expectedEvents.length != 0) {
      this.giveUp = true;
      return;
    }

    this.ignore = false;

    if (arguments.length == 0 ||
        arguments.length == 1 && arguments[0] == "") {
      return;
    }

    
    var args = Array.prototype.slice.call(arguments);
    
    if (args.length == 1 && typeof args[0] === 'string' &&
        args[0].indexOf(' ') > 0) {
      args = args[0].split(' ');
    }
    
    this.expectedEvents = args.map(this._argToEventId);
  }

  
  this.ignoreEvents = function()
  {
    
    ok(this.giveUp || this.expectedEvents.length == 0,
      "Going to ignore subsequent events on " + this.attr +
      " attribute, but we're still expecting the following events: " +
      this._stillExpecting());

    this.ignore = true;
  }

  this.finish = function()
  {
    
    ok(this.giveUp || this.expectedEvents.length == 0,
      "Finishing listening to " + this.attr +
      " attribute, but we're still expecting the following events: " +
      this._stillExpecting());

    this.element.removeEventListener('DOMAttrModified', this._listener, false);
    this.attr = "";
  }

  this._receiveEvent = function(e)
  {
    if (this.giveUp || this.ignore) {
      this.oldValue = e.newValue;
      return;
    }

    
    if (this.expectedEvents.length == 0) {
      ok(false, 'Unexpected ' + this._eventToName(e.attrChange) +
         ' event when none expected on ' + this.attr + ' attribute.');
      return;
    }

    var expectedEvent = this.expectedEvents.shift();

    
    if (e.attrChange != expectedEvent) {
      ok(false, 'Unexpected ' + this._eventToName(e.attrChange) +
        ' on ' + this.attr + ' attribute. Expected ' +
        this._eventToName(expectedEvent) + ' (followed by: ' +
        this._stillExpecting() + ")");
      
      
      this.giveUp = true;
      return;
    }

    
    is(e.target, this.element,
       'Unexpected node for mutation event on ' + this.attr + ' attribute');
    is(e.attrName, this.attr, 'Unexpected attribute name for mutation event');

    
    

    
    if (e.attrChange == MutationEvent.MODIFICATION) {
      ok(this.element.hasAttribute(this.attr),
         'Attribute not set after modification');
      is(e.prevValue, this.oldValue,
         'Unexpected old value for modification to ' + this.attr +
         ' attribute');
      isnot(e.newValue, this.oldValue,
         'Unexpected new value for modification to ' + this.attr +
         ' attribute');
    } else if (e.attrChange == MutationEvent.REMOVAL) {
      ok(!this.element.hasAttribute(this.attr), 'Attribute set after removal');
      is(e.prevValue, this.oldValue,
         'Unexpected old value for removal of ' + this.attr +
         ' attribute');
      
      
      
      ok(e.newValue === "",
         'Unexpected new value for removal of ' + this.attr +
         ' attribute');
    } else if (e.attrChange == MutationEvent.ADDITION) {
      ok(this.element.hasAttribute(this.attr),
         'Attribute not set after addition');
      
      
      
      ok(e.prevValue === "",
         'Unexpected old value for addition of ' + this.attr +
         ' attribute');
      ok(typeof(e.newValue) == 'string' && e.newValue !== "",
         'Unexpected new value for addition of ' + this.attr +
         ' attribute');
    } else {
      ok(false, 'Unexpected mutation event type: ' + e.attrChange);
      this.giveUp = true;
    }
    this.oldValue = e.newValue;
  }
  this._listener = this._receiveEvent.bind(this);

  this._stillExpecting = function()
  {
    if (this.expectedEvents.length == 0) {
      return "(nothing)";
    }
    var eventNames = [];
    for (var i=0; i < this.expectedEvents.length; i++) {
      eventNames.push(this._eventToName(this.expectedEvents[i]));
    }
    return eventNames.join(", ");
  }

  this._eventToName = function(evtId)
  {
    switch (evtId)
    {
    case MutationEvent.MODIFICATION:
      return "modification";
    case MutationEvent.ADDITION:
      return "addition";
    case MutationEvent.REMOVAL:
      return "removal";
    }
  }

  this._argToEventId = function(arg)
  {
    if (typeof arg === 'number')
      return arg;

    if (typeof arg !== 'string') {
      ok(false, "Unexpected event type: " + arg);
      return 0;
    }

    switch (arg.toLowerCase())
    {
    case "mod":
    case "modify":
    case "modification":
      return MutationEvent.MODIFICATION;

    case "add":
    case "addition":
      return MutationEvent.ADDITION;

    case "removal":
    case "remove":
      return MutationEvent.REMOVAL;

    default:
      ok(false, "Unexpected event name: " + arg);
      return 0;
    }
  }
}
