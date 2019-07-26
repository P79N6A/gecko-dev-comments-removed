


function test() {
  let contentTab, contentDoc, inspector;

  waitForExplicitFinish();

  
  contentTab = gBrowser.selectedTab = gBrowser.addTab();

  
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    inspector = toolbox.getCurrentPanel();

    inspector.selection.setNode(content.document.querySelector("body"));
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node.tagName, "BODY", "Inspector displayed");

      
      gBrowser.selectedBrowser.addEventListener("load", function onload() {
        gBrowser.selectedBrowser.removeEventListener("load", onload, true);
        contentDoc = content.document;

        
        
        

        var iframe = contentDoc.createElement("iframe");
        contentDoc.body.appendChild(iframe);
        iframe.parentNode.removeChild(iframe);

        is(contentDoc.querySelector("iframe"), null, "Iframe has been removed");

        inspector.once("markuploaded", () => {
          
          is(contentDoc.querySelector("iframe"), null, "Iframe has been removed");
          is(contentDoc.getElementById("yay").textContent, "load", "Load event fired");

          inspector.selection.setNode(contentDoc.getElementById("yay"));
          inspector.once("inspector-updated", () => {
            ok(inspector.selection.node, "Inspector still displayed");

            
            contentTab, contentDoc, inspector = null;
            gBrowser.removeTab(contentTab);
            finish();
          });
        });
      }, true);
      content.location = "http://mochi.test:8888/browser/browser/devtools/" +
       "inspector/test/browser_inspector_dead_node_exception.html";
    });
  });
}
