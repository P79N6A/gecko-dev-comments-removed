









































function userOnStart()
{
}

function userOnBeforePage()
{
}

function userOnAfterPage()
{
  gPageCompleted = true;
}

function userOnStop()
{
}


gConsoleListener.onConsoleMessage = 
function userOnConsoleMessage(s)
{
  dump(s);
};
