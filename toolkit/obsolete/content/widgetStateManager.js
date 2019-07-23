













































function WidgetStateManager( frame_id, panelPrefix, panelSuffix )
{
  
  






  this.PageData          = [];
  this.content_frame     = window.frames[frame_id];

  this.panelPrefix       = ( panelPrefix ) ? panelPrefix : null;
  this.panelSuffix       = ( panelSuffix ) ? panelSuffix : null;
  
  
  this.SavePageData      = WSM_SavePageData;
  this.SetPageData       = WSM_SetPageData;
  this.PageIsValid       = WSM_PageIsValid;
  this.GetTagFromURL     = WSM_GetTagFromURL;
  this.GetURLFromTag     = WSM_GetURLFromTag;
  this.toString          = WSM_toString;
  this.AddAttributes     = WSM_AddAttributes;
  this.ElementIsIgnored  = WSM_ElementIsIgnored;
  this.HasValidElements  = WSM_HasValidElements;
  this.LookupElement     = WSM_LookupElement;
  this.GetDataForTag     = WSM_GetDataForTag;
  this.SetDataForTag     = WSM_SetDataForTag;
}









               
function WSM_SavePageData( currentPageTag, optAttributes, exclElements, inclElements )
{
  
  
  
  
  
  
  
  
  
  
  
  
  if( !currentPageTag )
    currentPageTag = this.GetTagFromURL( this.content_frame.location.href, this.panelPrefix, this.panelSuffix, true );
  
  var doc = this.content_frame.document;
  var thisTagData = this.GetDataForTag(currentPageTag);

  if( this.content_frame.GetFields ) {
    
    this.SetDataForTag(currentPageTag, this.content_frame.GetFields());
    var string = "";
    for( var i in thisTagData )
    {
      string += "element: " + i + "\n";
    }
  }
  else if (doc && doc.controls) {
    var fields = doc.controls;
    var data = [];
    for( i = 0; i < fields.length; i++ ) 
    { 
      data[i] = []; 
      var formElement = fields[i];
      var elementEntry = thisTagData[formElement.id] = [];

      
      if( !this.ElementIsIgnored( formElement, exclElements ) )
        elementEntry.excluded = false;
      else 
        elementEntry.excluded = true;

      if( formElement.localName.toLowerCase() == "select" ) { 
        

          if( formElement.getAttribute("multiple") ) {
            
            for( var j = 0, idx = 0; j < formElement.options.length; j++ )
            {
              if( formElement.options[j].selected ) {
                elementEntry.value[idx] = formElement.options[j].value;
                idx++;
              }
            }
          }
          else {
              
              if (formElement.options[formElement.selectedIndex]) {
                  var value = formElement.options[formElement.selectedIndex].value;
                  dump("*** VALUE=" + value + "\n");
                  formElement.arbitraryvalue = value;
                  this.AddAttributes( formElement, elementEntry, "arbitraryvalue", optAttributes );
                  this.AddAttributes( formElement, elementEntry, "value", optAttributes);
              }
          }
      }
      else if( formElement.getAttribute("type") &&
               ( formElement.type.toLowerCase() == "checkbox" ||
                 formElement.type.toLowerCase() == "radio" ) ) {
        
        this.AddAttributes( formElement, elementEntry, "checked", optAttributes );
      }
      else if( formElement.type == "text" &&
               formElement.getAttribute( "datatype" ) == "nsIFileSpec" &&
               formElement.value ) {
        try {
          var filespec = Components.classes["@mozilla.org/filespec;1"].createInstance();
          filespec = filespec.QueryInterface( Components.interfaces.nsIFileSpec );
        }
        catch(e) {
          dump("*** Failed to create filespec object\n");
        }
        filespec.nativePath = formElement.value;
        this.AddAttributes( formElement, elementEntry, "filespec", optAttributes )
      }
      else
        this.AddAttributes( formElement, elementEntry, "value", optAttributes );  

      elementEntry.id       = formElement.id;
      elementEntry.localName = formElement.localName;
      
      elementEntry.elType   = ( formElement.type ) ? formElement.type : null;
    }
  }
  if( !this.HasValidElements( thisTagData ) )
    thisTagData.noData = true; 
}









function WSM_SetPageData( currentPageTag, hasExtraAttributes )
{
  if( !currentPageTag )
    currentPageTag = this.GetTagFromURL( this.content_frame.location.href, this.panelPrefix, this.panelSuffix, true );
  
	var doc = this.content_frame.document;
  var thisTagData = this.GetDataForTag(currentPageTag);
  if ( thisTagData && !thisTagData.nodata) {
  	for( var i in thisTagData ) {
      if( thisTagData[i].excluded || !i )
        continue;     
     
      var id    = thisTagData[i].id;
      var value = thisTagData[i].value;

      dump("*** id & value: " + id + " : " + value + "\n");
      
      if( this.content_frame.SetFields && !hasExtraAttributes )
        this.content_frame.SetFields( id, value );  
      else if( this.content_frame.SetFields && hasExtraAttributes )
        this.content_frame.SetFields( id, value, thisTagData[i]); 
      else {                              
        var formElement = doc.getElementById( i );
        
        if( formElement && hasExtraAttributes ) {        
          for( var attName in thisTagData[i] ) 
          {
            
            if( attName == "value" || attName == "id" )
              continue;                   
            var attValue  = thisTagData[i][attName];
            formElement.setAttribute( attName, attValue );
          }
        }
        
        
        if( formElement && formElement.localName.toLowerCase() == "input" ) {
          if( formElement.type.toLowerCase() == "checkbox" ||
              formElement.type.toLowerCase() == "radio" ) {
            if( value == undefined )
              formElement.checked = formElement.defaultChecked;
            else 
              formElement.checked = value;






          }
          else if( formElement.type.toLowerCase() == "text" &&
               formElement.getAttribute( "datatype" ) == "nsIFileSpec" ) {
            
            if( value ) {
              var filespec = value.QueryInterface( Components.interfaces.nsIFileSpec );
              try {
                formElement.value = filespec.nativePath;
              } 
              catch( ex ) {
                dump("Still need to fix uninitialized filespec problem!\n");
              }
            } 
            else
              formElement.value = formElement.defaultValue;
          }
          else {                          
            if( value == undefined )
              formElement.value = formElement.defaultValue;
            else
              formElement.value = value;
          }
        } 
        else if( formElement && formElement.localName.toLowerCase() == "select" ) {
          
            if( formElement.getAttribute("multiple") &&
                typeof(value) == "object" ) {
              
              for( var j = 0; j < value.length; j++ )
              {
                for ( var k = 0; k < formElement.options.length; k++ )
                {
                  if( formElement.options[k].value == value[j] )
                    formElement.options[k].selected = true;
                }
              }
            }
            else {
              
              for ( k = 0; k < formElement.options.length; k++ )
              {
                dump("*** value=" + value + "; options[k].value=" + formElement.options[k].value + "\n");
                if( formElement.options[k].value == value )
                  formElement.options[k].selected = true;
              }
            }            
        }
        else if( formElement && formElement.localName.toLowerCase() == "textarea" )
          formElement.value = value;
      }
    }
  }
  
  if (this.content_frame.onInit) {
    dump("Calling custom onInit()\n");
    this.content_frame.onInit();
  }
    
}   






function WSM_PageIsValid()
{
  if( this.content_frame.validate )
    return this.content_frame.validate();

  
  return true;
}






               
function WSM_GetTagFromURL( url, prefix, suffix, mode )
{
  
  if (!prefix) return undefined;
  if( mode )
    return url.substring( prefix.length, url.lastIndexOf(suffix) );
  else
    return url.substring( url.lastIndexOf(prefix) + 1, url.lastIndexOf(suffix) );
}





               
function WSM_GetURLFromTag( tag, prefix, postfix ) 
{
  return prefix + tag + postfix;
}






function WSM_toString()
{
  var string = "";
  for( var i in this.PageData ) {
    for( var j in this.PageData[i] ) {
      for( var k in this.PageData[i][j] ) {
        string += "WSM.PageData[" + i + "][" + j + "][" + k + "] : " + this.PageData[i][j][k] + ";\n";
      }
    }
  }
  return string;
}











function WSM_AddAttributes( formElement, elementEntry, valueAttribute, optAttributes )
{
  
  if( formElement.getAttribute("reversed") )
    elementEntry.value = !formElement[valueAttribute]; 
  else
    elementEntry.value = formElement[valueAttribute]; 
  
  if( optAttributes ) {   
    for(var k = 0; k < optAttributes.length; k++ ) 
    {
      attValue = formElement.getAttribute( optAttributes[k] );
      if( attValue )
        elementEntry[optAttributes[k]] = attValue;
    }
  }
}







function WSM_ElementIsIgnored( element, exclElements )
{
  if (!exclElements) return false;
  for( var i = 0; i < exclElements.length; i++ )
  {
    if( element.localName.toLowerCase() == exclElements[i] )
      return true;
  }
  return false;
}







function WSM_HasValidElements( dataStore )
{
  for( var i in dataStore ) 
  {
    if( !dataStore[i].excluded )
      return true;
  }
  return false;
}


function WSM_LookupElement( element, lookby, property )
{
  for(var i in this.PageData )
  {
    for( var j in this.PageData[i] )
    {
      if(!lookby) 
        lookby = "id";    
      if( j[lookby] == element && !property )
        return j;
      else if( j[lookby] == element ) {
        var container = [];
        for( var k = 0; k < property.length; k++ )
        {
          container[k] = this.PageData[i][k][property[k]];
        }
        return container; 
      }
    }
  }
  return undefined;
}








function WSM_GetDataForTag(tag) {
  if (!this.PageData[tag])
    this.PageData[tag] = [];
  return this.PageData[tag];
}

function WSM_SetDataForTag(tag, data) {
  this.PageData[tag] = data;
}


