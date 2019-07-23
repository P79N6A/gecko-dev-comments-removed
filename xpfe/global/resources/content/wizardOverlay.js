


















var doNextFunction    = null;
var doBackFunction    = null;
var doFinishFunction  = null;
var doCancelFunction  = null;


function doSetWizardButtons( wizardManager, nextFunc, backFunc, finishFunc, cancelFunc )
{
  if(wizardManager) {
    doNextFunction    = wizardManager.onNext;
    doBackFunction    = wizardManager.onBack;
    doFinishFunction  = wizardManager.onFinish;
    doCancelFunction  = wizardManager.onCancel;
  } else {
  	doNextFunction    = nextFunc;
    doBackFunction    = backFunc;
    doFinishFunction  = finishFunc;
    doCancelFunction  = cancelFunc; 
  }
}


function doNextButton()
{
	if ( doNextFunction )
		doNextFunction();
}


function doBackButton()
{
	if ( doBackFunction )
		doBackFunction();
}


function doFinishButton()
{
	if ( doFinishFunction )
  	doFinishFunction();
}


function doCancelButton()
{
	if ( doCancelFunction )
		doCancelFunction();
}


