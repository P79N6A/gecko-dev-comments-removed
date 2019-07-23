












































function WizardHandlerSet( sMgr, wMgr )
{
  
  
  this.backButton         = document.getElementById("wiz-back-button");
  this.nextButton         = document.getElementById("wiz-next-button");
  this.finishButton       = document.getElementById("wiz-finish-button");
  this.cancelButton       = document.getElementById("wiz-cancel-button");
  
  this.nextButtonFunc     = null;
  this.backButtonFunc     = null;
  this.cancelButtonFunc   = null;
  this.finishButtonFunc   = null;
  this.pageLoadFunc       = null;
  this.enablingFunc       = null;
    
  this.SM                 = sMgr;
  this.WM                 = wMgr;

  
  this.SetHandlers        = WHS_SetHandlers;

  
  this.SetHandlers( this.nextButtonFunc, this.backButtonFunc, this.finishButtonFunc, 
                    this.cancelButtonFunc, this.pageLoadFunc, this.enablingFunc );
}
  

function DEF_onNext()
{
  var oParent = this.WHANDLER;
  if( oParent.nextButton.getAttribute("disabled") == "true" )
    return;

  
  if (!oParent.SM.PageIsValid())
      return;
  
	oParent.SM.SavePageData( this.currentPageTag, null, null, null );      
  if (this.wizardMap[this.currentPageTag]) {
    var nextPageTag = this.wizardMap[this.currentPageTag].next;
    this.LoadPage( nextPageTag, false );     
    this.ProgressUpdate( ++this.currentPageNumber );
  } else {
    dump("Error: Missing an entry in the wizard map for " +
         this.currentPageTag + "\n");
  }
}

function DEF_onBack()
{
  var oParent = this.WHANDLER;
	if( oParent.backButton.getAttribute("disabled") == "true" )
 	  return;
	oParent.SM.SavePageData( this.currentPageTag, null, null, null );      
	previousPageTag = this.wizardMap[this.currentPageTag].previous;
  this.LoadPage( previousPageTag, false ); 
  this.ProgressUpdate( --this.currentPageNumber );
}

function DEF_onCancel()
{
  if( top.window.opener )
    window.close();
}

function DEF_onFinish()
{
  var oParent = this.WHANDLER;
  if( !this.wizardMap[this.currentPageTag].finish )
    return;
  oParent.SM.SavePageData( this.currentPageTag, null, null, null );
  dump("WizardButtonHandlerSet Warning:\n");
  dump("===============================\n");
  dump("You must provide implementation for onFinish, or else your data will be lost!\n");
}


function DEF_DoEnabling( nextButton, backButton, finishButton )
{
  var oParent = this.WHANDLER;
  
	if( !this.currentPageTag ) 
    return;
  
  var nextTag = this.wizardMap[this.currentPageTag].next;
  if( nextTag && oParent.nextButton.getAttribute("disabled") ) {
    oParent.nextButton.removeAttribute( "disabled" );
  }
  else if( !nextTag && !oParent.nextButton.getAttribute("disabled") ) {
    oParent.nextButton.setAttribute( "disabled", "true" );
  }
  
  var finishTag = this.wizardMap[this.currentPageTag].finish;
  if( finishTag && oParent.finishButton.getAttribute("disabled") ) {
    oParent.finishButton.removeAttribute( "disabled" );
  }
  else if( !finishTag && !oParent.finishButton.getAttribute("disabled") ) {
    oParent.finishButton.setAttribute( "disabled", "true" );
  }
  
  var prevTag = this.wizardMap[this.currentPageTag].previous;
	if( prevTag && oParent.backButton.getAttribute("disabled") ) {
 	  oParent.backButton.removeAttribute("disabled");
  }
	else if( !prevTag && !oParent.backButton.getAttribute("disabled") ) {
   	oParent.backButton.setAttribute("disabled", "true"); 
  }
}








function DEF_onPageLoad( tag ) 
{
  var oParent = this.WHANDLER;
	this.currentPageTag = tag;
  if( this.DoButtonEnabling )              
    this.DoButtonEnabling();               
  if( this.content_frame ) {
    oParent.SM.SetPageData( tag, true );  

    
    var doc = window.frames[0].document;
    if ("controls" in doc && doc.controls.length > 0) {
      var controls = doc.controls;
      for (var i=0; i< controls.length; i++) {
        if (controls[i].focus) {
          controls[i].focus();
          break;                  
        }
      }
    }
  }
  else {
    dump("Widget Data Manager Error:\n"); 
    dump("==========================\n");
    dump("content_frame variable not defined. Please specify one as an argument to Startup();\n");
    return;
  }
}







function WHS_SetHandlers( nextFunc, backFunc, finishFunc, cancelFunc, pageLoadFunc, enablingFunc )
{
  
  this.nextButtonFunc   = nextFunc      ;
  this.backButtonFunc   = backFunc      ;
  this.cancelButtonFunc = cancelFunc    ;
  this.finishButtonFunc = finishFunc    ;
  this.pageLoadFunc     = pageLoadFunc  ;
  this.enablingFunc     = enablingFunc  ;
  
  
  
  this.WM.onNext             = ( !this.nextButtonFunc   ) ? DEF_onNext     : nextFunc ;
  this.WM.onBack             = ( !this.backButtonFunc   ) ? DEF_onBack     : backFunc ;
  this.WM.onCancel           = ( !this.cancelButtonFunc ) ? DEF_onCancel   : cancelFunc ;
  this.WM.onFinish           = ( !this.finishButtonFunc ) ? DEF_onFinish   : finishFunc ;
  this.WM.onPageLoad         = ( !this.pageLoadFunc     ) ? DEF_onPageLoad : pageLoadFunc ;
  this.WM.DoButtonEnabling   = ( !this.enablingFunc     ) ? DEF_DoEnabling : enablingFunc ;
}
