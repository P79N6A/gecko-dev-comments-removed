


function runTests() {
  yield setLinks("0");
  yield addNewTabPageTab();

  let site = getCell(0).node.querySelector(".newtab-site");
  site.setAttribute("type", "sponsored");

  
  let sponsoredButton = site.querySelector(".newtab-sponsored");
  EventUtils.synthesizeMouseAtCenter(sponsoredButton, {}, getContentWindow());
  let explain = site.querySelector(".sponsored-explain");
  isnot(explain, null, "Sponsored explanation shown");
  ok(explain.querySelector("input").classList.contains("newtab-control-block"), "sponsored tiles show blocked image");
  ok(sponsoredButton.hasAttribute("active"), "Sponsored button has active attribute");

  
  EventUtils.synthesizeMouseAtCenter(sponsoredButton, {}, getContentWindow());
  is(site.querySelector(".sponsored-explain"), null, "Sponsored explanation no longer shown");
  ok(!sponsoredButton.hasAttribute("active"), "Sponsored button does not have active attribute");

  
  site.setAttribute("type", "enhanced");
  EventUtils.synthesizeMouseAtCenter(sponsoredButton, {}, getContentWindow());
  explain = site.querySelector(".sponsored-explain");
  isnot(explain, null, "Sponsored explanation shown");
  ok(explain.querySelector("input").classList.contains("newtab-customize"), "enhanced tiles show customize image");
  ok(sponsoredButton.hasAttribute("active"), "Sponsored button has active attribute");

  
  EventUtils.synthesizeMouseAtCenter(sponsoredButton, {}, getContentWindow());
  is(site.querySelector(".sponsored-explain"), null, "Sponsored explanation no longer shown");
  ok(!sponsoredButton.hasAttribute("active"), "Sponsored button does not have active attribute");
}
