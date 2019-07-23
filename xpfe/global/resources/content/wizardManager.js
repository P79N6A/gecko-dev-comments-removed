















































function WizardManager( frame_id, tagURLPrefix, tagURLPostfix, wizardMap )
{
  
  this.currentPageTag       = null;
  
  this.wizardMap            = ( wizardMap ) ? wizardMap : null;
  
  this.currentPageNumber    = 0;
  
  this.firstTime            = true;   
  
  this.content_frame        = document.getElementById( frame_id );
  
  this.WSM                  = new WidgetStateManager( frame_id );
  
  this.WHANDLER             = new WizardHandlerSet( this.WSM, this );
  
  
  this.URL_PagePrefix       = ( tagURLPrefix  ) ? tagURLPrefix : null;
  this.URL_PagePostfix      = ( tagURLPostfix ) ? tagURLPrefix : null; 

  
  this.bundle               = srGetStrBundle("chrome://global/locale/wizardManager.properties");

  this.LoadPage             = WM_LoadPage;
  this.GetURLFromTag        = WM_GetURLFromTag;
  this.GetTagFromURL        = WM_GetTagFromURL;
  this.SetHandlers          = WM_SetHandlers;
  this.SetPageData          = WM_SetPageData;
  this.SavePageData         = WM_SavePageData;
  this.ProgressUpdate       = WM_ProgressUpdate;
  this.GetMapLength         = WM_GetMapLength;

  
  
  doSetWizardButtons( this );
}








function WM_LoadPage( pageURL, absolute )
{
	if( pageURL != "" )
	{
    if ( this.firstTime && !absolute )
      this.ProgressUpdate( this.currentPageNumber );

    
    
    

    
    if( !absolute ) {
            var src = this.GetURLFromTag( pageURL );
    } else {
      src = pageURL;
    }
    if( this.content_frame )
			this.content_frame.setAttribute("src", src);
    else 
      return false;

  	this.firstTime = false;
    return true;
	}
  return false;
}




               
function WM_GetURLFromTag( tag ) 
{
  return this.URL_PagePrefix + tag + this.URL_PagePostfix;
}




               
function WM_GetTagFromURL( url )
{
  return url.substring(this.URL_PagePrefix.length, this.URL_PagePostfix.length);
}


function WM_SetHandlers( onNext, onBack, onFinish, onCancel, onPageLoad, enablingFunc )
{
  this.WHANDLER.SetHandlers( onNext, onBack, onFinish, onCancel, onPageLoad, enablingFunc );
}

function WM_SetPageData()
{
  this.WSM.SetPageData();
}

function WM_SavePageData()
{
  this.WSM.SavePageData();
}





               
function WM_GetMapLength()
{
  var count = 0;
  for ( i in this.wizardMap )
    count++;
  return count;
}





               
function WM_ProgressUpdate( currentPageNumber )
{
  var statusbar = document.getElementById ( "status" );
  if ( statusbar ) {
      var string;
      try {
          string = this.bundle.formatStringFromName("oflabel",
                                                    [currentPageNumber+1,
                                                    this.GetMapLength()], 2);
      } catch (e) {
          string = "";
      }
    statusbar.setAttribute( "progress", string );
  }
}

