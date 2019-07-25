




let EXPORTED_SYMBOLS = [ ];

Components.utils.import("resource:///modules/devtools/gcli.jsm");











gcli.addCommand({
  name: "cookie",
  description: gcli.lookup("cookieDesc"),
  manual: gcli.lookup("cookieManual")
});




var cookieListHtml = "" +
  "<table>" +
  "  <tr>" +
  "    <th>" + gcli.lookup("cookieListOutKey") + "</th>" +
  "    <th>" + gcli.lookup("cookieListOutValue") + "</th>" +
  "    <th>" + gcli.lookup("cookieListOutActions") + "</th>" +
  "  </tr>" +
  "  <tr foreach='cookie in ${cookies}'>" +
  "    <td>${cookie.key}</td>" +
  "    <td>${cookie.value}</td>" +
  "    <td>" +
  "      <span class='gcli-out-shortcut' onclick='${onclick}'" +
  "          data-command='cookie set ${cookie.key} '" +
  "          >" + gcli.lookup("cookieListOutEdit") + "</span>" +
  "      <span class='gcli-out-shortcut'" +
  "          onclick='${onclick}' ondblclick='${ondblclick}'" +
  "          data-command='cookie remove ${cookie.key}'" +
  "          >" + gcli.lookup("cookieListOutRemove") + "</span>" +
  "    </td>" +
  "  </tr>" +
  "</table>" +
  "";




gcli.addCommand({
  name: "cookie list",
  description: gcli.lookup("cookieListDesc"),
  manual: gcli.lookup("cookieListManual"),
  returnType: "string",
  exec: function Command_cookieList(args, context) {
    
    var doc = context.environment.contentDocument;
    var cookies = doc.cookie.split("; ").map(function(cookieStr) {
      var equalsPos = cookieStr.indexOf("=");
      return {
        key: cookieStr.substring(0, equalsPos),
        value: cookieStr.substring(equalsPos + 1)
      };
    });

    return context.createView({
      html: cookieListHtml,
      data: {
        cookies: cookies,
        onclick: createUpdateHandler(context),
        ondblclick: createExecuteHandler(context),
      }
    });
  }
});




gcli.addCommand({
  name: "cookie remove",
  description: gcli.lookup("cookieRemoveDesc"),
  manual: gcli.lookup("cookieRemoveManual"),
  params: [
    {
      name: "key",
      type: "string",
      description: gcli.lookup("cookieRemoveKeyDesc"),
    }
  ],
  exec: function Command_cookieRemove(args, context) {
    let document = context.environment.contentDocument;
    let expDate = new Date();
    expDate.setDate(expDate.getDate() - 1);
    document.cookie = escape(args.key) + "=; expires=" + expDate.toGMTString();
  }
});




gcli.addCommand({
  name: "cookie set",
  description: gcli.lookup("cookieSetDesc"),
  manual: gcli.lookup("cookieSetManual"),
  params: [
    {
      name: "key",
      type: "string",
      description: gcli.lookup("cookieSetKeyDesc")
    },
    {
      name: "value",
      type: "string",
      description: gcli.lookup("cookieSetValueDesc")
    },
    {
      group: gcli.lookup("cookieSetOptionsDesc"),
      params: [
        {
          name: "path",
          type: "string",
          defaultValue: "/",
          description: gcli.lookup("cookieSetPathDesc")
        },
        {
          name: "domain",
          type: "string",
          defaultValue: null,
          description: gcli.lookup("cookieSetDomainDesc")
        },
        {
          name: "secure",
          type: "boolean",
          description: gcli.lookup("cookieSetSecureDesc")
        }
      ]
    }
  ],
  exec: function Command_cookieSet(args, context) {
    let document = context.environment.contentDocument;

    document.cookie = escape(args.key) + "=" + escape(args.value) +
            (args.domain ? "; domain=" + args.domain : "") +
            (args.path ? "; path=" + args.path : "") +
            (args.secure ? "; secure" : ""); 
  }
});





function withCommand(element, action) {
  var command = element.getAttribute("data-command");
  if (!command) {
    command = element.querySelector("*[data-command]")
            .getAttribute("data-command");
  }

  if (command) {
    action(command);
  }
  else {
    console.warn("Missing data-command for " + util.findCssSelector(element));
  }
}







function createUpdateHandler(context) {
  return function(ev) {
    withCommand(ev.currentTarget, function(command) {
      context.update(command);
    });
  }
}







function createExecuteHandler(context) {
  return function(ev) {
    withCommand(ev.currentTarget, function(command) {
      context.exec({
        visible: true,
        typed: command
      });
    });
  }
}
