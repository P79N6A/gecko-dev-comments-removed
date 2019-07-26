





var search = window.location.href.match(/^.*\?(.*)$/);
var searchParams = new URLSearchParams(search ? search[1] : '');
var storage = searchParams.get('storage');
var context = searchParams.get('context');


var [context, isAnon, isInBrowser, appId, isPrivate] = context.match(/(a,)?(b,)?(i\d+,)?(p,)?/);
if (appId)
  appId = appId.match(/i(\d+),/)[1];


function $(id) { return document.getElementById(id) || {}; }


addEventListener('DOMContentLoaded', function() {
  $('anon').checked = !!isAnon;
  $('inbrowser').checked = !!isInBrowser;
  $('appid').value = appId || '';
  $('priv').checked = !!isPrivate;
}, false);



function navigate()
{
  context = '';
  if ($('anon').checked)
    context += 'a,';
  if ($('inbrowser').checked)
    context += 'b,';
  if ($('appid').value)
    context += 'i' + $('appid').value + ',';
  if ($('priv').checked)
    context += 'p,';

  window.location.href = 'about:cache?storage=' + storage + '&context=' + context;
}
