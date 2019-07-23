var doOKFunction = 0;
var doCancelFunction = 0;
var doButton2Function = 0;
var doButton3Function = 0;



function doSetOKCancel(okFunc, cancelFunc, button2Func, button3Func )
{
	
	
	doOKFunction = okFunc;
	doCancelFunction = cancelFunc;
	doButton2Function = button2Func;
	doButton3Function = button3Func;
}

function doOKButton()
{
	var close = true;
	
	if ( doOKFunction )
		close = doOKFunction();
	
	if (close && top)
		top.window.close();
}

function doCancelButton()
{
	var close = true;
	
	if ( doCancelFunction )
		close = doCancelFunction();
	
	if (close && top)
		top.window.close();
}

function doButton2()
{
	var close = true;
	
	if ( doButton2Function )
		close = doButton2Function();
	
	if (close && top)
		top.window.close();
}

function doButton3()
{
	var close = true;
	
	if ( doButton3Function )
		close = doButton3Function();
	
	if (close && top)
		top.window.close();
}

function moveToAlertPosition()
{
    
    if (window.outerWidth == 1) {
        dump("Trying to position a sizeless window; caller should have called sizeToContent() or sizeTo(). See bug 75649.\n");
        sizeToContent();
    }

	var xOffset = (opener.outerWidth - window.outerWidth) / 2;
	var yOffset = opener.outerHeight / 5;
	
	var newX = opener.screenX + xOffset;
	var newY = opener.screenY + yOffset;
	
	
	if (newX < screen.availLeft)
		newX = screen.availLeft + 20;
	if ((newX + window.outerWidth) > (screen.availLeft + screen.availWidth))
		newX = (screen.availLeft + screen.availWidth) - window.outerWidth - 20;

	if (newY < screen.availTop)
		newY = screen.availTop + 20;
	if ((newY + window.outerHeight) > (screen.availTop + screen.availHeight))
		newY = (screen.availTop + screen.availHeight) - window.outerHeight - 60;

	window.moveTo( newX, newY );
}

function centerWindowOnScreen()
{
	var xOffset = screen.availWidth/2 - window.outerWidth/2;
	var yOffset = screen.availHeight/2 - window.outerHeight/2; 
	
	xOffset = ( xOffset > 0 ) ? xOffset : 0;
  yOffset = ( yOffset > 0 ) ? yOffset : 0;
	window.moveTo( xOffset, yOffset);
}
