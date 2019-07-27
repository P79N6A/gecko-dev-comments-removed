






















$(function() {
  
  
  
  
  
  
  

  
  
  
  var bodywrapper = $('.bodywrapper');
  var sidebar = $('.sphinxsidebar');
  var sidebarwrapper = $('.sphinxsidebarwrapper');

  
  if (!sidebar.length) return;

  
  
  var bw_margin_expanded = bodywrapper.css('margin-left');
  var ssb_width_expanded = sidebar.width();

  
  
  var bw_margin_collapsed = '.8em';
  var ssb_width_collapsed = '.8em';

  
  var dark_color = $('.related').css('background-color');
  var light_color = $('.document').css('background-color');

  function sidebar_is_collapsed() {
    return sidebarwrapper.is(':not(:visible)');
  }

  function toggle_sidebar() {
    if (sidebar_is_collapsed())
      expand_sidebar();
    else
      collapse_sidebar();
  }

  function collapse_sidebar() {
    sidebarwrapper.hide();
    sidebar.css('width', ssb_width_collapsed);
    bodywrapper.css('margin-left', bw_margin_collapsed);
    sidebarbutton.css({
        'margin-left': '0',
        'height': bodywrapper.height()
    });
    sidebarbutton.find('span').text('»');
    sidebarbutton.attr('title', _('Expand sidebar'));
    document.cookie = 'sidebar=collapsed';
  }

  function expand_sidebar() {
    bodywrapper.css('margin-left', bw_margin_expanded);
    sidebar.css('width', ssb_width_expanded);
    sidebarwrapper.show();
    sidebarbutton.css({
        'margin-left': ssb_width_expanded-12,
        'height': bodywrapper.height()
    });
    sidebarbutton.find('span').text('«');
    sidebarbutton.attr('title', _('Collapse sidebar'));
    document.cookie = 'sidebar=expanded';
  }

  function add_sidebar_button() {
    sidebarwrapper.css({
        'float': 'left',
        'margin-right': '0',
        'width': ssb_width_expanded - 28
    });
    
    sidebar.append(
        '<div id="sidebarbutton"><span>&laquo;</span></div>'
    );
    var sidebarbutton = $('#sidebarbutton');
    light_color = sidebarbutton.css('background-color');
    
    var viewport_height;
    if (window.innerHeight)
 	  viewport_height = window.innerHeight;
    else
	  viewport_height = $(window).height();
    sidebarbutton.find('span').css({
        'display': 'block',
        'margin-top': (viewport_height - sidebar.position().top - 20) / 2
    });

    sidebarbutton.click(toggle_sidebar);
    sidebarbutton.attr('title', _('Collapse sidebar'));
    sidebarbutton.css({
        'color': '#FFFFFF',
        'border-left': '1px solid ' + dark_color,
        'font-size': '1.2em',
        'cursor': 'pointer',
        'height': bodywrapper.height(),
        'padding-top': '1px',
        'margin-left': ssb_width_expanded - 12
    });

    sidebarbutton.hover(
      function () {
          $(this).css('background-color', dark_color);
      },
      function () {
          $(this).css('background-color', light_color);
      }
    );
  }

  function set_position_from_cookie() {
    if (!document.cookie)
      return;
    var items = document.cookie.split(';');
    for(var k=0; k<items.length; k++) {
      var key_val = items[k].split('=');
      var key = key_val[0].replace(/ /, "");  
      if (key == 'sidebar') {
        var value = key_val[1];
        if ((value == 'collapsed') && (!sidebar_is_collapsed()))
          collapse_sidebar();
        else if ((value == 'expanded') && (sidebar_is_collapsed()))
          expand_sidebar();
      }
    }
  }

  add_sidebar_button();
  var sidebarbutton = $('#sidebarbutton');
  set_position_from_cookie();
});