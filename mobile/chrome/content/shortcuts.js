







































const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

function ShortcutEditor()
{
    
    function getCommandNames()
    {
        return Array.map(document.getElementsByTagNameNS(XUL_NS, "command"), function(c) { return c.getAttribute("id"); });
    }

    function findKeyForCommand(command)
    {
        
        
        
        
        
        var keys = document.getElementsByTagNameNS(XUL_NS, "key");
        var l = keys.length;
        for (var i = 0; i < l; i++)
            if (keys[i].getAttribute("command") == command)
                return keys[i];
    }

    function findCommandForKey(modifiers, key, keycode)
    {
        
        
        
        
        
        var keys = document.getElementsByTagNameNS(XUL_NS, "key");
        var l = keys.length;
        for (var i = 0; i < l; i++)
            if (keys[i].getAttribute("modifiers") == modifiers &&
                keys[i].getAttribute("key") == key &&
                keys[i].getAttribute("keycode") == keycode)
                return keys[i];
    }

    function addKey(command, modifiers, key, keycode)
    {
        
        
        
        
        

        if (findCommandForKey(modifiers, key, keycode))
            return null;

        var k;
        if ((k = findKeyForCommand(command)))
        {
            k.modifiers = modifiers;
            k.key = key;
            k.keycode = keycode;
        }
        else
        {
            k = document.createElementNS(XUL_NS, "key");
            k.modifiers = modifiers;
            k.key = key;
            k.keycode = keycode;
            k.command = command;
            document.getElementById("mainKeyset").appendChild(k);
        }

        return k;
    }

    function getKeyName(key) {
        
        
        
        
        
        
        

        if (!key)
            return "";

        var m = key.getAttribute("modifiers").replace(" ", "-");
        var k = key.getAttribute("key") || key.getAttribute("keycode");

        return m +"-"+ k;
    }

    
    this.edit = function()
    {
        var nodes = document.getElementById("ui-stack").childNodes;
        Array.forEach(nodes, function(n) { if (n.getAttribute("id") != "browser-container") { n.hidden = true; }});
        document.getElementById("shortcuts-container").hidden = false;
        fillShortcutList();
    };

    this.dismiss = function()
    {
        
        document.getElementById("shortcuts-container").hidden = true;
    };

    
    function fillShortcutList()
    {
        var listbox = document.getElementById("shortcuts");
        var commands = getCommandNames();
        var sb = document.getElementById("bundle-shortcuts");

        function doAppend(name, key)
        {
            
            
            

            var cell1 = document.createElementNS(XUL_NS, "listcell");
            cell1.setAttribute("label", name);
            var cell2 = document.createElementNS(XUL_NS, "listcell");
            cell2.setAttribute("label", key);
            var item = document.createElementNS(XUL_NS, "listitem");
            item.appendChild(cell1);
            item.appendChild(cell2);
            listbox.appendChild(item);
        }

        function doGetString(name)
        {
            try
            {
                return sb.getString(name);
            }
            catch (e) { }
        }

        function clear()
        {
            
            var c;
            while ((c = listbox.getRowCount()))
                listbox.removeItemAt(c - 1);
        }

        clear();
        commands.forEach(function(c) { doAppend(doGetString(c +".name") || c, getKeyName(findKeyForCommand(c)) || "â€”"); });
    }
}

var Shortcuts = new ShortcutEditor();
