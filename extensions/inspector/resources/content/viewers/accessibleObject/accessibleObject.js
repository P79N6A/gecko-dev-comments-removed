















































var viewer;
var bundle;
var accService;




const kAccessibleRetrievalCID = "@mozilla.org/accessibleRetrieval;1";

const nsIAccessibleRetrieval = Components.interfaces.nsIAccessibleRetrieval;
const nsIAccessible = Components.interfaces.nsIAccessible;




window.addEventListener("load", AccessibleObjectViewer_initialize, false);

function AccessibleObjectViewer_initialize()
{
  bundle = document.getElementById("inspector-bundle");
  accService = XPCU.getService(kAccessibleRetrievalCID, nsIAccessibleRetrieval);

  viewer = new JSObjectViewer();

  viewer.__defineGetter__(
    "uid",
    function uidGetter()
    {
      return "accessibleObject";
    }
  );

  viewer.__defineSetter__(
    "subject",
    function subjectSetter(aObject)
    {
      var accObject = null;
      try {
        accObject = aObject.getUserData("accessible");
        if (accObject)
          XPCU.QI(accObject, nsIAccessible);
        else
          accObject = accService.getAccessibleFor(aObject);
      } catch(e) {
        dump("Failed to get accessible object for node.");
      }

      this.mSubject = accObject;
      this.emptyTree(this.mTreeKids);
      var ti = this.addTreeItem(this.mTreeKids,
                                bundle.getString("root.title"),
                                accObject,
                                accObject);
      ti.setAttribute("open", "true");

      this.mObsMan.dispatchEvent("subjectChange", { subject: accObject });
    }
   );

  viewer.initialize(parent.FrameExchange.receiveData(window));
}
