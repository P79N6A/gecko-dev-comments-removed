





































let EXPORTED_SYMBOLS = [ "GcliCommands" ];

Components.utils.import("resource:///modules/gcli.jsm");
Components.utils.import("resource:///modules/HUDService.jsm");





gcli.addCommand({
  name: "echo",
  description: gcli.lookup("echoDesc"),
  params: [
    {
      name: "message",
      type: "string",
      description: gcli.lookup("echoMessageDesc")
    }
  ],
  returnType: "string",
  exec: function Command_echo(args, context) {
    return args.message;
  }
});


let canon = gcli._internal.require("gcli/canon");




gcli.addCommand({
  name: "help",
  returnType: "html",
  description: gcli.lookup("helpDesc"),
  exec: function Command_help(args, context) {
    let output = [];

    output.push("<strong>" + gcli.lookup("helpAvailable") + ":</strong><br/>");

    let commandNames = canon.getCommandNames();
    commandNames.sort();

    output.push("<table>");
    for (let i = 0; i < commandNames.length; i++) {
      let command = canon.getCommand(commandNames[i]);
      if (!command.hidden && command.description) {
        output.push("<tr>");
        output.push('<th class="gcli-help-right">' + command.name + "</th>");
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
  description: gcli.lookup("consoleDesc"),
  manual: gcli.lookup("consoleManual")
});




gcli.addCommand({
  name: "console clear",
  description: gcli.lookup("consoleclearDesc"),
  exec: function(args, context) {
    let hud = HUDService.getHudReferenceById(context.environment.hudId);
    hud.gcliterm.clearOutput();
  }
});





gcli.addCommand({
  name: "inspect",
  description: gcli.lookup("inspectDesc"),
  manual: gcli.lookup("inspectManual"),
  params: [
    {
      name: "node",
      type: "node",
      description: gcli.lookup("inspectNodeDesc"),
      manual: gcli.lookup("inspectNodeManual")
    }
  ],
  exec: function Command_inspect(args, context) {
    let hud = HUDService.getHudReferenceById(context.environment.hudId);
    let InspectorUI = hud.gcliterm.document.defaultView.InspectorUI;
    InspectorUI.openInspectorUI(args.node);
  }
});
