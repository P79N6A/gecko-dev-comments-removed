


function check_lcms_has_assembly()
{
    
    

    if (navigator.platform == "MacIntel") {
        return true;
    }

    if (navigator.platform == "Win32" || navigator.platform == "OS/2") {
        
        
        return true;
    }

    
    
    if (navigator.platform.match(/(i[3456]86|x86_64|amd64)/)) {
        return true;
    }

    return false;
}

var lcms_has_assembly = check_lcms_has_assembly();
