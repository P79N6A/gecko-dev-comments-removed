





































let EXPORTED_SYMBOLS = [ "GcliCommands" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource:///modules/gcli.jsm");
Components.utils.import("resource:///modules/HUDService.jsm");

let bundleName = "chrome://browser/locale/devtools/gclicommands.properties";
let stringBundle = Services.strings.createBundle(bundleName);

let gcli = gcli._internal.require("gcli/index");
let canon = gcli._internal.require("gcli/canon");


let document;




let GcliCommands = {
  


  setDocument: function GcliCommands_setDocument(aDocument) {
    document = aDocument;
  },

  


  unsetDocument: function GcliCommands_unsetDocument() {
    document = undefined;
  }
};







function lookup(aName)
{
  try {
    return stringBundle.GetStringFromName(aName);
  } catch (ex) {
    throw new Error("Failure in lookup('" + aName + "')");
  }
};







function lookupFormat(aName, aSwaps)
{
  try {
    return stringBundle.formatStringFromName(aName, aSwaps, aSwaps.length);
  } catch (ex) {
    Services.console.logStringMessage("Failure in lookupFormat('" + aName + "')");
    throw new Error("Failure in lookupFormat('" + aName + "')");
  }
}





gcli.addCommand({
  name: "echo",
  description: lookup("echoDesc"),
  params: [
    {
      name: "message",
      type: "string",
      description: lookup("echoMessageDesc")
    }
  ],
  returnType: "string",
  exec: function Command_echo(args, context) {
    return args.message;
  }
});





gcli.addCommand({
  name: "help",
  returnType: "html",
  description: lookup("helpDesc"),
  exec: function Command_help(args, context) {
    let output = [];

    output.push("<strong>" + lookup("helpAvailable") + ":</strong><br/>");

    let commandNames = canon.getCommandNames();
    commandNames.sort();

    output.push("<table>");
    for (let i = 0; i < commandNames.length; i++) {
      let command = canon.getCommand(commandNames[i]);
      if (!command.hidden && command.description) {
        output.push("<tr>");
        output.push('<th class="gcliCmdHelpRight">' + command.name + "</th>");
        output.push("<td>&#x2192; " + command.description + "</td>");
        output.push("</tr>");
      }
    }
    output.push("</table>");

    return output.join("");
  }
});





gcli.addCommand({
  name: "console",
  description: lookup("consoleDesc"),
  manual: lookup("consoleManual")
});




gcli.addCommand({
  name: "console clear",
  description: lookup("consoleclearDesc"),
  exec: function(args, context) {
    let hud = HUDService.getHudReferenceById(context.environment.hudId);
    hud.gcliterm.clearOutput();
  }
});

