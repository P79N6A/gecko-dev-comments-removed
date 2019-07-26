


function runTests() {
  yield setLinks("0");
  yield addNewTabPageTab();

  
  
  yield whenSearchInitDone();

  let site = getCell(0).node.querySelector(".newtab-site");
  site.setAttribute("type", "sponsored");

  let sponsoredPanel = getContentDocument().getElementById("sponsored-panel");
  is(sponsoredPanel.state, "closed", "Sponsored panel must be closed");

  function continueOnceOn(event) {
    sponsoredPanel.addEventListener(event, function listener() {
      sponsoredPanel.removeEventListener(event, listener);
      executeSoon(TestRunner.next);
    });
  }

  
  continueOnceOn("popupshown");
  let sponsoredButton = site.querySelector(".newtab-control-sponsored");
  yield EventUtils.synthesizeMouseAtCenter(sponsoredButton, {}, getContentWindow());
  is(sponsoredPanel.state, "open", "Sponsored panel opens on click");
  ok(sponsoredButton.hasAttribute("panelShown"), "Sponsored button has panelShown attribute");

  
  continueOnceOn("popuphidden");
  yield sponsoredPanel.hidePopup();
  is(sponsoredPanel.state, "closed", "Sponsored panel correctly closed/hidden");
  ok(!sponsoredButton.hasAttribute("panelShown"), "Sponsored button does not have panelShown attribute");
}
