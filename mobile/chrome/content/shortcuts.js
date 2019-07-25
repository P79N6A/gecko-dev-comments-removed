











































const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

function ShortcutEditor()
{
    var prefsvc = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefService);
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch2);
    var keyPrefs = prefsvc.getBranch("shortcut.");
    var keyCache;

    
    function getCommandNames()
    {
        return Array.map(document.getElementsByTagNameNS(XUL_NS, "command"), function(c) { return c.getAttribute("id"); });
    }

    function getKeys()
    {
        if (keys)
            return keys;

        keyCache = { };
        Array.map(document.getElementsByTagNameNS(XUL_NS, "key"), function(k) { keyCache[k.getAttribute("command")] = k; });
        return keyCache;
    }

    function findKeyForCommand(command)
    {
        
        return getKeys()[command];
    }

    function findCommandForKey(keySpec)
    {
        
        
        
        
        
        var keys = document.getElementsByTagNameNS(XUL_NS, "key");
        var l = keys.length;
        for (var i = 0; i < l; i++)
            if (keys[i].getAttribute("modifiers") == keySpec.modifiers &&
                keys[i].getAttribute("key") == keySpec.key &&
                keys[i].getAttribute("keycode") == keySpec.keycode)
                return keys[i];
    }

    function addKey(command, keySpec)
    {
        
        
        
        
        

        if (findCommandForKey(keySpec))
            return null;

        var key;
        if ((key = findKeyForCommand(command)))
        {
            key.setAttribute("modifiers") = keySpec.modifiers;
            key.setAttribute("key") = keySpec.key;
            key.setAttribute("keycode") = keySpec.keycode;
        }
        else
        {
            key = document.createElementNS(XUL_NS, "key");
            key.setAttribute("modifiers") = keySpec.modifiers;
            key.setAttribute("key") = keySpec.key;
            key.setAttribute("keycode") = keySpec.keycode;
            key.setAttribute("command") = command;
            document.getElementById("mainKeyset").appendChild(k);
            keys[command] = key;
        }

        return key;
    }

    function makeKeySpec(modifiers, key, keycode)
    {
        if (modifiers instanceof Components.interfaces.nsIDOMElement)
            return {
                modifiers: modifiers.getAttribute("modifiers"),
                key: modifiers.getAttribute("key"),
                keycode: modifiers.getAttribute("keycode")
            };
        return {
            modifiers: modifiers,
            key: key,
            keycode: keycode
        };
    }

    
    
    
    

    
    var platformBundle = document.getElementById("bundle-platformKeys");
    var platformKeys = {
        shift: platformBundle.getString("VK_SHIFT"),
        meta: platformBundle.getString("VK_META"),
        alt: platformBundle.getString("VK_ALT"),
        control: platformBundle.getString("VK_CONTROL")
    };
    var modifierSeparator = platformBundle.getString("MODIFIER_SEPARATOR");

#ifdef XP_MACOSX
    var accelKey = Components.interfaces.nsIDOMKeyEvent.DOM_VK_META;
#else
    var accelKey = Components.interfaces.nsIDOMKeyEvent.DOM_VK_CONTROL;
#endif

    try {
        accelKey = prefs.getCharPref("ui.key.accelKey");
    } catch (e) { }

    
    var platformAccel = { };
    platformAccel[Components.interfaces.nsIDOMKeyEvent.DOM_VK_META] = platformKeys.meta;
    platformAccel[Components.interfaces.nsIDOMKeyEvent.DOM_VK_ALT] = platformKeys.alt;
    platformAccel[Components.interfaces.nsIDOMKeyEvent.DOM_VK_CONTROL] = platformKeys.control;
    platformKeys.accel = platformAccel[accelKey] || platformKeys.control;

    function getKeyName(key) {
        
        
        if (!key)
            return "";
        var keySpec = makeKeySpec(key);

        var accel = [];
        var keybundle = document.getElementById("bundle-keys");
        var keyName = keySpec.keytext || keySpec.key || keybundle.getString(keySpec.keycode);
        var modifiers = keySpec.modifiers.split(" ");
        for each (m in modifiers)
            if (m in platformKeys)
                accel.push(platformKeys[m]);
        accel.push(keyName);

        return accel.join(modifierSeparator);
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
        var tree = document.getElementById("shortcuts");
        var commands = getCommandNames();
        var sb = document.getElementById("bundle-shortcuts");

        function doAppend(name, key)
        {
            
            
            
            var cell1 = document.createElementNS(XUL_NS, "treecell");
            cell1.setAttribute("label", name);
            var cell2 = document.createElementNS(XUL_NS, "treecell");
            cell2.setAttribute("label", key);
            var row = document.createElementNS(XUL_NS, "treerow");
            row.appendChild(cell1);
            row.appendChild(cell2);
            var item = document.createElementNS(XUL_NS, "treeitem");
            item.appendChild(row);
            document.getElementById("shortcuts-children").appendChild(item);
        }

        function doGetString(name)
        {
            try
            {
                return sb.getString(name);
            }
            catch (e) { }
        }

        var children = document.getElementById("shortcuts-children");
        tree.removeChild(children);
        children = document.createElementNS(XUL_NS, "treechildren");
        children.setAttribute("id", "shortcuts-children");
        tree.appendChild(children);

        commands.forEach(function(c) { doAppend(doGetString(c +".name") || c, getKeyName(findKeyForCommand(c)) || "â€”"); });
    }

    
    function save(command, keySpec)
    {
        keyPrefs.setCharPref(command, JSON.toString(keySpec));
    }

    function restore(command)
    {
        return JSON.fromString(keyPrefs.getCharPref(command));
    }
}

var Shortcuts = new ShortcutEditor();
