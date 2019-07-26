



let gTests = [
  {
    desc: "Customization reset should restore visibility to default-visible toolbars.",
    setup: null,
    run: function() {
      let navbar = document.getElementById("nav-bar");
      is(navbar.collapsed, false, "Test should start with navbar visible");
      navbar.collapsed = true;
      is(navbar.collapsed, true, "navbar should be hidden now");

      yield resetCustomization();

      is(navbar.collapsed, false, "Customization reset should restore visibility to the navbar");
    },
    teardown: null
  },
];

function test() {
  waitForExplicitFinish();
  runTests(gTests);
}
