






































var xalan_field;

function onLoad()
{
    view.tree = document.getElementById('out');
    view.boxObject = view.tree.boxObject.QueryInterface(Components.interfaces.nsITreeBoxObject);
    {  
        view.mIframe = document.getElementById('hiddenHtml');
        view.mIframe.webNavigation.allowPlugins = false;
        view.mIframe.webNavigation.allowJavascript = false;
        view.mIframe.webNavigation.allowMetaRedirects = false;
        view.mIframe.webNavigation.allowImages = false;
    }
    view.database = view.tree.database;
    view.builder = view.tree.builder.QueryInterface(nsIXULTemplateBuilder);
    view.builder.QueryInterface(nsIXULTreeBuilder);
    runItem.prototype.kDatabase = view.database;
    xalan_field = document.getElementById("xalan_rdf");
    var persistedUrl = xalan_field.getAttribute('url');
    if (persistedUrl) {
        view.xalan_url = persistedUrl;
        xalan_field.value = persistedUrl;
    }
    view.setDataSource();
    return true;
}

function onUnload()
{
    if (xalan_field)
        xalan_field.setAttribute('url', xalan_field.value);
}
