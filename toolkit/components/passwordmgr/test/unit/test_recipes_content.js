






"use strict";

Cu.import("resource://testing-common/httpd.js");
Cu.importGlobalProperties(["URL"]);

Cu.import("resource://gre/modules/devtools/Console.jsm");

add_task(function* test_getFieldOverrides() {
  let recipes = new Set([
    { 
      hosts: ["example.com:8080"],
      passwordSelector: "#password",
      pathRegex: /^\/$/,
      usernameSelector: ".username",
    },
    { 
      hosts: ["example.com:8080"],
    },
    { 
      description: "best match",
      hosts: ["a.invalid", "example.com:8080", "other.invalid"],
      passwordSelector: "#password",
      pathRegex: /^\/first\/second\/$/,
      usernameSelector: ".username",
    },
  ]);

  let form = RecipeHelpers.createTestForm("http://localhost:8080/first/second/");
  let override = LoginRecipesContent.getFieldOverrides(recipes, form);
  Assert.strictEqual(override.description, "best match",
                     "Check the best field override recipe was returned");
  Assert.strictEqual(override.usernameSelector, ".username", "Check usernameSelector");
  Assert.strictEqual(override.passwordSelector, "#password", "Check passwordSelector");
});
