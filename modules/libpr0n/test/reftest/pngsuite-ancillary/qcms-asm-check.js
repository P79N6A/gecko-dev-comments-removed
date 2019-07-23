


function check_qcms_has_assembly()
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

var qcms_has_assembly = check_qcms_has_assembly();
