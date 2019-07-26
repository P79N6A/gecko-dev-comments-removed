



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

let EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");




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
  hidden: true,
  exec: function Command_echo(args, context) {
    return args.message;
  }
});
