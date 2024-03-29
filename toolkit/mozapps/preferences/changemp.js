





const nsPK11TokenDB = "@mozilla.org/security/pk11tokendb;1";
const nsIPK11TokenDB = Components.interfaces.nsIPK11TokenDB;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsPKCS11ModuleDB = "@mozilla.org/security/pkcs11moduledb;1";
const nsIPKCS11ModuleDB = Components.interfaces.nsIPKCS11ModuleDB;
const nsIPKCS11Slot = Components.interfaces.nsIPKCS11Slot;
const nsIPK11Token = Components.interfaces.nsIPK11Token;


var params;
var tokenName="";
var pw1;

function init()
{
  pw1 = document.getElementById("pw1");
	 	 
  process();
}


function process()
{
   var secmoddb = Components.classes[nsPKCS11ModuleDB].getService(nsIPKCS11ModuleDB);
   var bundle = document.getElementById("bundlePreferences");

   
   

   var slot = secmoddb.findSlotByName(tokenName);
   if (slot) {
     var oldpwbox = document.getElementById("oldpw");
     var msgBox = document.getElementById("message");
     var status = slot.status;
     if (status == nsIPKCS11Slot.SLOT_UNINITIALIZED
         || status == nsIPKCS11Slot.SLOT_READY) {
      
       oldpwbox.setAttribute("hidden", "true");
       msgBox.setAttribute("value", bundle.getString("password_not_set")); 
       msgBox.setAttribute("hidden", "false");

       if (status == nsIPKCS11Slot.SLOT_READY) {
         oldpwbox.setAttribute("inited", "empty");
       } else {
         oldpwbox.setAttribute("inited", "true");
       }
      
       
       document.getElementById('pw1').focus();
    
     } else {
       
       oldpwbox.setAttribute("hidden", "false");
       msgBox.setAttribute("hidden", "true");
       oldpwbox.setAttribute("inited", "false");
       oldpwbox.focus();
     }
   }

  if (params) {
    
    params.SetInt(1, 0);
  }
  
  checkPasswords();
}

function setPassword()
{
  var pk11db = Components.classes[nsPK11TokenDB].getService(nsIPK11TokenDB);
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                .getService(Components.interfaces.nsIPromptService);
  var token = pk11db.findTokenByName(tokenName);
  dump("*** TOKEN!!!! (name = |" + token + "|\n");

  var oldpwbox = document.getElementById("oldpw");
  var initpw = oldpwbox.getAttribute("inited");
  var bundle = document.getElementById("bundlePreferences");
  
  var success = false;
  
  if (initpw == "false" || initpw == "empty") {
    try {
      var oldpw = "";
      var passok = 0;
      
      if (initpw == "empty") {
        passok = 1;
      } else {
        oldpw = oldpwbox.value;
        passok = token.checkPassword(oldpw);
      }
      
      if (passok) {
        if (initpw == "empty" && pw1.value == "") {
          
          
        } else {
          if (pw1.value == "") {
            var secmoddb = Components.classes[nsPKCS11ModuleDB].getService(nsIPKCS11ModuleDB);
            if (secmoddb.isFIPSEnabled) {
              
              promptService.alert(window,
                                  bundle.getString("pw_change_failed_title"),
                                  bundle.getString("pw_change2empty_in_fips_mode"));
              passok = 0;
            }
          }
          if (passok) {
            token.changePassword(oldpw, pw1.value);
            if (pw1.value == "") {
              promptService.alert(window,
                                  bundle.getString("pw_change_success_title"),
                                  bundle.getString("pw_erased_ok") 
                                  + " " + bundle.getString("pw_empty_warning"));
            } else {
              promptService.alert(window,
                                  bundle.getString("pw_change_success_title"),
                                  bundle.getString("pw_change_ok"));
            }
            success = true;
          }
        }
      } else {
        oldpwbox.focus();
        oldpwbox.setAttribute("value", "");
        promptService.alert(window,
                            bundle.getString("pw_change_failed_title"),
                            bundle.getString("incorrect_pw"));
      }
    } catch (e) {
      promptService.alert(window,
                          bundle.getString("pw_change_failed_title"),
                          bundle.getString("failed_pw_change"));
    }
  } else {
    token.initPassword(pw1.value);
    if (pw1.value == "") {
      promptService.alert(window,
                          bundle.getString("pw_change_success_title"),
                          bundle.getString("pw_not_wanted")
                          + " " + bundle.getString("pw_empty_warning"));
    }
    success = true;
  }

  
  if (success)
    window.close();
}

function setPasswordStrength()
{






  var pw=document.getElementById('pw1').value;


  var pwlength=(pw.length);
  if (pwlength>5)
    pwlength=5;



  var numnumeric = pw.replace (/[0-9]/g, "");
  var numeric=(pw.length - numnumeric.length);
  if (numeric>3)
    numeric=3;


  var symbols = pw.replace (/\W/g, "");
  var numsymbols=(pw.length - symbols.length);
  if (numsymbols>3)
    numsymbols=3;


  var numupper = pw.replace (/[A-Z]/g, "");
  var upper=(pw.length - numupper.length);
  if (upper>3)
    upper=3;


  var pwstrength=((pwlength*10)-20) + (numeric*10) + (numsymbols*15) + (upper*10);

  
  if ( pwstrength < 0 ) {
    pwstrength = 0;
  }
  
  if ( pwstrength > 100 ) {
    pwstrength = 100;
  }

  var mymeter=document.getElementById('pwmeter');
  mymeter.value = pwstrength;

  return;
}

function checkPasswords()
{
  var pw1=document.getElementById('pw1').value;
  var pw2=document.getElementById('pw2').value;
  var ok=document.documentElement.getButton("accept");

  var oldpwbox = document.getElementById("oldpw");
  if (oldpwbox) {
    var initpw = oldpwbox.getAttribute("inited");

    if (initpw == "empty" && pw1 == "") {
      
      
      
      
      ok.setAttribute("disabled","true");
      return;
    }
  }

  if (pw1 == pw2){
    ok.setAttribute("disabled","false");
  } else
  {
    ok.setAttribute("disabled","true");
  }

}
