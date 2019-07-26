


onmessage = function(e) {
    
    postMessage(JSON.stringify({'OS_Constants': OS.Constants, 'OS_Constants_Win': OS.Constants.Win,
        'OS_Constants_Path': OS.Constants.Path, 'OS_Constants_Sys': OS.Constants.Sys,
        'OS_Constants_libc': OS.Constants.libc, 'OS_Constants_Sys_DEBUG': OS.Constants.Sys.DEBUG,
        'OS_Constants_Sys_Name': OS.Constants.Sys.Name}));
};