


































#include "cairoint.h"

#include "cairo-xlib-private.h"
#include "cairo-xlib-xrender-private.h"
#include "cairo-freelist-private.h"
#include "cairo-error-private.h"

#include <X11/Xlibint.h>	

typedef int (*cairo_xlib_error_func_t) (Display     *display,
					XErrorEvent *event);

struct _cairo_xlib_job {
    cairo_xlib_job_t *next;
    enum {
	RESOURCE,
	WORK
    } type;
    union {
	struct {
	    cairo_xlib_notify_resource_func notify;
	    XID xid;
	} resource;
	struct {
	    cairo_xlib_notify_func notify;
	    void *data;
	    void (*destroy) (void *);
	} work;
    } func;
};

static cairo_xlib_display_t *_cairo_xlib_display_list;

static void
_cairo_xlib_remove_close_display_hook_internal (cairo_xlib_display_t *display,
						cairo_xlib_hook_t *hook);

static void
_cairo_xlib_call_close_display_hooks (cairo_xlib_display_t *display)
{
    cairo_xlib_screen_t *screen;
    cairo_xlib_hook_t *hook;

    cairo_list_foreach_entry (screen, cairo_xlib_screen_t, &display->screens, link)
	_cairo_xlib_screen_close_display (display, screen);

    while (TRUE) {
	hook = display->close_display_hooks;
	if (hook == NULL)
	    break;

	_cairo_xlib_remove_close_display_hook_internal (display, hook);

	hook->func (display, hook);
    }
    display->closed = TRUE;
}

static void
_cairo_xlib_display_finish (void *abstract_display)
{
    cairo_xlib_display_t *display = abstract_display;

    display->display = NULL;
}

static void
_cairo_xlib_display_destroy (void *abstract_display)
{
    cairo_xlib_display_t *display = abstract_display;

    
    while (display->workqueue != NULL) {
	cairo_xlib_job_t *job = display->workqueue;
	display->workqueue = job->next;

	if (job->type == WORK && job->func.work.destroy != NULL)
	    job->func.work.destroy (job->func.work.data);

	_cairo_freelist_free (&display->wq_freelist, job);
    }
    _cairo_freelist_fini (&display->wq_freelist);

    while (! cairo_list_is_empty (&display->screens)) {
	_cairo_xlib_screen_destroy (cairo_list_first_entry (&display->screens,
                                                            cairo_xlib_screen_t,
                                                            link));
    }

    free (display);
}

static int
_noop_error_handler (Display     *display,
		     XErrorEvent *event)
{
    return False;		
}

static void
_cairo_xlib_display_notify (cairo_xlib_display_t *display)
{
    cairo_xlib_job_t *jobs, *job, *freelist;
    Display *dpy = display->display;

    


    if (display->workqueue == NULL)
	return;

    jobs = display->workqueue;
    while (jobs != NULL) {
	display->workqueue = NULL;

	
	job = NULL;
	do {
	    cairo_xlib_job_t *next = jobs->next;
	    jobs->next = job;
	    job = jobs;
	    jobs = next;
	} while (jobs != NULL);
	freelist = jobs = job;

	do {
	    job = jobs;
	    jobs = job->next;

	    switch (job->type){
	    case WORK:
		job->func.work.notify (dpy, job->func.work.data);
		if (job->func.work.destroy != NULL)
		    job->func.work.destroy (job->func.work.data);
		break;

	    case RESOURCE:
		job->func.resource.notify (dpy, job->func.resource.xid);
		break;
	    }
	} while (jobs != NULL);

	do {
	    job = freelist;
	    freelist = job->next;
	    _cairo_freelist_free (&display->wq_freelist, job);
	} while (freelist != NULL);

	jobs = display->workqueue;
    }
}

static int
_cairo_xlib_close_display (Display *dpy, XExtCodes *codes)
{
    cairo_xlib_display_t *display, **prev, *next;
    cairo_xlib_error_func_t old_handler;

    CAIRO_MUTEX_LOCK (_cairo_xlib_display_mutex);
    for (display = _cairo_xlib_display_list; display; display = display->next)
	if (display->display == dpy)
	    break;
    CAIRO_MUTEX_UNLOCK (_cairo_xlib_display_mutex);
    if (display == NULL)
	return 0;

    if (! cairo_device_acquire (&display->base)) {
      
      XSync (dpy, False);
      old_handler = XSetErrorHandler (_noop_error_handler);

      _cairo_xlib_display_notify (display);
      _cairo_xlib_call_close_display_hooks (display);

      
      _cairo_xlib_display_notify (display);

      XSync (dpy, False);
      XSetErrorHandler (old_handler);

      cairo_device_release (&display->base);
    }

    


    CAIRO_MUTEX_LOCK (_cairo_xlib_display_mutex);
    prev = &_cairo_xlib_display_list;
    for (display = _cairo_xlib_display_list; display; display = next) {
	next = display->next;
	if (display->display == dpy) {
	    *prev = next;
	    break;
	} else
	    prev = &display->next;
    }
    CAIRO_MUTEX_UNLOCK (_cairo_xlib_display_mutex);

    assert (display != NULL);

    cairo_device_finish (&display->base);
    cairo_device_destroy (&display->base);

    

    return 0;
}

static const cairo_device_backend_t _cairo_xlib_device_backend = {
    CAIRO_DEVICE_TYPE_XLIB,

    NULL,
    NULL,

    NULL, 
    _cairo_xlib_display_finish,
    _cairo_xlib_display_destroy,
};









cairo_device_t *
_cairo_xlib_device_create (Display *dpy)
{
    cairo_xlib_display_t *display;
    cairo_xlib_display_t **prev;
    cairo_device_t *device;
    XExtCodes *codes;
    const char *env;

    static int buggy_repeat_force = -1;

    CAIRO_MUTEX_INITIALIZE ();

    






    CAIRO_MUTEX_LOCK (_cairo_xlib_display_mutex);

    for (prev = &_cairo_xlib_display_list; (display = *prev); prev = &(*prev)->next)
    {
	if (display->display == dpy) {
	    


	    if (prev != &_cairo_xlib_display_list) {
		*prev = display->next;
		display->next = _cairo_xlib_display_list;
		_cairo_xlib_display_list = display;
	    }
            device = cairo_device_reference (&display->base);
	    goto UNLOCK;
	}
    }

    display = malloc (sizeof (cairo_xlib_display_t));
    if (unlikely (display == NULL)) {
	device = _cairo_device_create_in_error (CAIRO_STATUS_NO_MEMORY);
	goto UNLOCK;
    }

    





    display->render_major = display->render_minor = -1;
    XRenderQueryVersion (dpy, &display->render_major, &display->render_minor);
    env = getenv ("CAIRO_DEBUG");
    if (env != NULL && (env = strstr (env, "xrender-version=")) != NULL) {
	int max_render_major, max_render_minor;

	env += sizeof ("xrender-version=") - 1;
	if (sscanf (env, "%d.%d", &max_render_major, &max_render_minor) != 2)
	    max_render_major = max_render_minor = -1;

	if (max_render_major < display->render_major ||
	    (max_render_major == display->render_major &&
	     max_render_minor < display->render_minor))
	{
	    display->render_major = max_render_major;
	    display->render_minor = max_render_minor;
	}
    }

    codes = XAddExtension (dpy);
    if (unlikely (codes == NULL)) {
	device = _cairo_device_create_in_error (CAIRO_STATUS_NO_MEMORY);
	free (display);
	goto UNLOCK;
    }

    _cairo_device_init (&display->base, &_cairo_xlib_device_backend);

    XESetCloseDisplay (dpy, codes->extension, _cairo_xlib_close_display);

    _cairo_freelist_init (&display->wq_freelist, sizeof (cairo_xlib_job_t));

    cairo_device_reference (&display->base); 
    display->display = dpy;
    cairo_list_init (&display->screens);
    display->workqueue = NULL;
    display->close_display_hooks = NULL;
    display->closed = FALSE;

    memset (display->cached_xrender_formats, 0,
	    sizeof (display->cached_xrender_formats));

    


#if RENDER_MAJOR == 0 && RENDER_MINOR < 10
    display->buggy_gradients = TRUE;
#else
    display->buggy_gradients = FALSE;
#endif
    display->buggy_pad_reflect = FALSE;
    display->buggy_repeat = FALSE;

    











































    if (strstr (ServerVendor (dpy), "X.Org") != NULL) {
	if (VendorRelease (dpy) >= 60700000) {
	    if (VendorRelease (dpy) < 70000000)
		display->buggy_repeat = TRUE;

	    
	    if (VendorRelease (dpy) < 70200000)
		display->buggy_gradients = TRUE;

	    
	    display->buggy_pad_reflect = TRUE;
	} else {
	    if (VendorRelease (dpy) < 10400000)
		display->buggy_repeat = TRUE;

	    
	    if (VendorRelease (dpy) < 10699000)
		display->buggy_pad_reflect = TRUE;
	}
    } else if (strstr (ServerVendor (dpy), "XFree86") != NULL) {
	if (VendorRelease (dpy) <= 40500000)
	    display->buggy_repeat = TRUE;

	display->buggy_gradients = TRUE;
	display->buggy_pad_reflect = TRUE;
    }

    
    display->buggy_gradients = TRUE;


    
    




    if (buggy_repeat_force == -1) {
        const char *flag = getenv("MOZ_CAIRO_FORCE_BUGGY_REPEAT");

        buggy_repeat_force = -2;

        if (flag && flag[0] == '0')
            buggy_repeat_force = 0;
        else if (flag && flag[0] == '1')
            buggy_repeat_force = 1;
    }

    if (buggy_repeat_force != -2)
        display->buggy_repeat = (buggy_repeat_force == 1);

    display->next = _cairo_xlib_display_list;
    _cairo_xlib_display_list = display;

    device = &display->base;

UNLOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_xlib_display_mutex);
    return device;
}

void
_cairo_xlib_add_close_display_hook (cairo_xlib_display_t	*display,
				    cairo_xlib_hook_t		*hook)
{
    hook->prev = NULL;
    hook->next = display->close_display_hooks;
    if (hook->next != NULL)
	hook->next->prev = hook;
    display->close_display_hooks = hook;
}

static void
_cairo_xlib_remove_close_display_hook_internal (cairo_xlib_display_t *display,
						cairo_xlib_hook_t *hook)
{
    if (display->close_display_hooks == hook)
	display->close_display_hooks = hook->next;
    else if (hook->prev != NULL)
	hook->prev->next = hook->next;

    if (hook->next != NULL)
	hook->next->prev = hook->prev;

    hook->prev = NULL;
    hook->next = NULL;
}

void
_cairo_xlib_remove_close_display_hook (cairo_xlib_display_t	*display,
				       cairo_xlib_hook_t	*hook)
{
    _cairo_xlib_remove_close_display_hook_internal (display, hook);
}

cairo_status_t
_cairo_xlib_display_queue_resource (cairo_xlib_display_t *display,
	                            cairo_xlib_notify_resource_func notify,
				    XID xid)
{
    cairo_xlib_job_t *job;
    cairo_status_t status = CAIRO_STATUS_NO_MEMORY;

    if (display->closed == FALSE) {
	job = _cairo_freelist_alloc (&display->wq_freelist);
	if (job != NULL) {
	    job->type = RESOURCE;
	    job->func.resource.xid = xid;
	    job->func.resource.notify = notify;

	    job->next = display->workqueue;
	    display->workqueue = job;

	    status = CAIRO_STATUS_SUCCESS;
	}
    }

    return status;
}

cairo_status_t
_cairo_xlib_display_queue_work (cairo_xlib_display_t *display,
	                        cairo_xlib_notify_func notify,
				void *data,
				void (*destroy) (void *))
{
    cairo_xlib_job_t *job;
    cairo_status_t status = CAIRO_STATUS_NO_MEMORY;

    if (display->closed == FALSE) {
	job = _cairo_freelist_alloc (&display->wq_freelist);
	if (job != NULL) {
	    job->type = WORK;
	    job->func.work.data    = data;
	    job->func.work.notify  = notify;
	    job->func.work.destroy = destroy;

	    job->next = display->workqueue;
	    display->workqueue = job;

	    status = CAIRO_STATUS_SUCCESS;
	}
    }

    return status;
}

cairo_status_t
_cairo_xlib_display_acquire (cairo_device_t *device, cairo_xlib_display_t **display)
{
    cairo_status_t status;

    status = cairo_device_acquire (device);
    if (status)
        return status;

    *display = (cairo_xlib_display_t *) device;
    _cairo_xlib_display_notify (*display);
    return status;
}

XRenderPictFormat *
_cairo_xlib_display_get_xrender_format (cairo_xlib_display_t	*display,
	                                cairo_format_t		 format)
{
    XRenderPictFormat *xrender_format;

#if ! ATOMIC_OP_NEEDS_MEMORY_BARRIER
    xrender_format = display->cached_xrender_formats[format];
    if (likely (xrender_format != NULL))
	return xrender_format;
#endif

    xrender_format = display->cached_xrender_formats[format];
    if (xrender_format == NULL) {
	int pict_format;

	switch (format) {
	case CAIRO_FORMAT_A1:
	    pict_format = PictStandardA1; break;
	case CAIRO_FORMAT_A8:
	    pict_format = PictStandardA8; break;
	case CAIRO_FORMAT_RGB24:
	    pict_format = PictStandardRGB24; break;
	case CAIRO_FORMAT_RGB16_565: {
	    Visual *visual = NULL;
	    Screen *screen = DefaultScreenOfDisplay(display->display);
	    int j;
	    for (j = 0; j < screen->ndepths; j++) {
	        Depth *d = &screen->depths[j];
	        if (d->depth == 16 && d->nvisuals && &d->visuals[0]) {
	            if (d->visuals[0].red_mask   == 0xf800 &&
	                d->visuals[0].green_mask == 0x7e0 &&
	                d->visuals[0].blue_mask  == 0x1f)
	                visual = &d->visuals[0];
	            break;
	        }
	    }
	    if (!visual)
	        return NULL;
	    xrender_format = XRenderFindVisualFormat(display->display, visual);
	    break;
	}
	case CAIRO_FORMAT_INVALID:
	default:
	    ASSERT_NOT_REACHED;
	case CAIRO_FORMAT_ARGB32:
	    pict_format = PictStandardARGB32; break;
	}
	if (!xrender_format)
	    xrender_format = XRenderFindStandardFormat (display->display,
		                                        pict_format);
	display->cached_xrender_formats[format] = xrender_format;
    }

    return xrender_format;
}

cairo_xlib_screen_t *
_cairo_xlib_display_get_screen (cairo_xlib_display_t *display,
				Screen *screen)
{
    cairo_xlib_screen_t *info;

    cairo_list_foreach_entry (info, cairo_xlib_screen_t, &display->screens, link) {
	if (info->screen == screen) {
            if (display->screens.next != &info->link)
                cairo_list_move (&info->link, &display->screens);
            return info;
        }
    }

    return NULL;
}

void
_cairo_xlib_display_get_xrender_version (cairo_xlib_display_t *display,
					 int *major, int *minor)
{
    *major = display->render_major;
    *minor = display->render_minor;
}

cairo_bool_t
_cairo_xlib_display_has_repeat (cairo_device_t *device)
{
    return ! ((cairo_xlib_display_t *) device)->buggy_repeat;
}

cairo_bool_t
_cairo_xlib_display_has_reflect (cairo_device_t *device)
{
    return ! ((cairo_xlib_display_t *) device)->buggy_pad_reflect;
}

cairo_bool_t
_cairo_xlib_display_has_gradients (cairo_device_t *device)
{
    return ! ((cairo_xlib_display_t *) device)->buggy_gradients;
}
