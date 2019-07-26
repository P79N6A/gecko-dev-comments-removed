



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");


gcli.addCommand({
  name: 'resize',
  description: gcli.lookup('resizeModeDesc')
});

gcli.addCommand({
  name: 'resize on',
  description: gcli.lookup('resizeModeOnDesc'),
  manual: gcli.lookup('resizeModeManual'),
  exec: gcli_cmd_resize
});

gcli.addCommand({
  name: 'resize off',
  description: gcli.lookup('resizeModeOffDesc'),
  manual: gcli.lookup('resizeModeManual'),
  exec: gcli_cmd_resize
});

gcli.addCommand({
  name: 'resize toggle',
  description: gcli.lookup('resizeModeToggleDesc'),
  manual: gcli.lookup('resizeModeManual'),
  exec: gcli_cmd_resize
});

gcli.addCommand({
  name: 'resize to',
  description: gcli.lookup('resizeModeToDesc'),
  params: [
    {
      name: 'width',
      type: 'number',
      description: gcli.lookup("resizePageArgWidthDesc"),
    },
    {
      name: 'height',
      type: 'number',
      description: gcli.lookup("resizePageArgHeightDesc"),
    },
  ],
  exec: gcli_cmd_resize
});

function gcli_cmd_resize(args, context) {
  let browserDoc = context.environment.chromeDocument;
  let browserWindow = browserDoc.defaultView;
  let mgr = browserWindow.ResponsiveUI.ResponsiveUIManager;
  mgr.handleGcliCommand(browserWindow,
                        browserWindow.gBrowser.selectedTab,
                        this.name,
                        args);
}
