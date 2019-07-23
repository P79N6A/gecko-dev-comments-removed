



































function onLoad()
{
    protectedAuthThread = window.arguments[0].QueryInterface(Components.interfaces.nsIProtectedAuthThread);

    if (!protectedAuthThread) 
    {
        window.close();
        return;
    }

    try
    {
        var tokenName = protectedAuthThread.getTokenName();

        var tag = document.getElementById("tokenName");
        tag.setAttribute("value",tokenName);

        setCursor("wait");
  
        protectedAuthThread.login(window);

    } catch (exception)
    {
        window.close();
        return;
    }
}

function onClose()
{
    setCursor("default");
}
