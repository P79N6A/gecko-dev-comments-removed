
































#include "cairoint.h"

#include "cairo-xlib-private.h"

#include <fontconfig/fontconfig.h>

#include <X11/Xlibint.h>	
#include <X11/extensions/Xrender.h>

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
_cairo_xlib_call_close_display_hooks (cairo_xlib_display_t *display)
{
    cairo_xlib_screen_info_t	    *screen;
    cairo_xlib_hook_t		    *hooks, *list;

    
    CAIRO_MUTEX_LOCK (display->mutex);

    for (screen = display->screens; screen != NULL; screen = screen->next)
	_cairo_xlib_screen_info_close_display (screen);

    hooks = display->close_display_hooks;
    while (hooks != NULL) {
	display->close_display_hooks = NULL;
	CAIRO_MUTEX_UNLOCK (display->mutex);

	list = hooks;
	do {
	    cairo_xlib_hook_t *hook = list;
	    list = hook->next;

	    hook->func (display->display, hook->data);
	} while (list != NULL);

	CAIRO_MUTEX_LOCK (display->mutex);
	do {
	    cairo_xlib_hook_t *hook = hooks;
	    hooks = hook->next;

	    _cairo_freelist_free (&display->hook_freelist, hook);
	} while (hooks != NULL);

	hooks = display->close_display_hooks;
    }
    display->closed = TRUE;

    CAIRO_MUTEX_UNLOCK (display->mutex);
}

static void
_cairo_xlib_display_discard_screens (cairo_xlib_display_t *display)
{
    cairo_xlib_screen_info_t *screens;

    CAIRO_MUTEX_LOCK (display->mutex);
    screens = display->screens;
    display->screens = NULL;
    CAIRO_MUTEX_UNLOCK (display->mutex);

    while (screens != NULL) {
	cairo_xlib_screen_info_t *screen = screens;
	screens = screen->next;

	_cairo_xlib_screen_info_destroy (screen);
    }
}

cairo_xlib_display_t *
_cairo_xlib_display_reference (cairo_xlib_display_t *display)
{
    if (display == NULL)
	return NULL;

    
    CAIRO_MUTEX_LOCK (display->mutex);

    assert (display->ref_count > 0);
    display->ref_count++;

    CAIRO_MUTEX_UNLOCK (display->mutex);

    return display;
}

void
_cairo_xlib_display_destroy (cairo_xlib_display_t *display)
{
    if (display == NULL)
	return;

    CAIRO_MUTEX_LOCK (display->mutex);
    assert (display->ref_count > 0);
    if (--display->ref_count == 0) {
	
	while (display->workqueue != NULL) {
	    cairo_xlib_job_t *job = display->workqueue;
	    display->workqueue = job->next;

	    if (job->type == WORK && job->func.work.destroy != NULL)
		job->func.work.destroy (job->func.work.data);

	    _cairo_freelist_free (&display->wq_freelist, job);
	}
	_cairo_freelist_fini (&display->wq_freelist);
	_cairo_freelist_fini (&display->hook_freelist);

	CAIRO_MUTEX_UNLOCK (display->mutex);

	free (display);
    } else
	CAIRO_MUTEX_UNLOCK (display->mutex);
}

static int
_noop_error_handler (Display     *display,
		     XErrorEvent *event)
{
    return False;		
}
static int
_cairo_xlib_close_display (Display *dpy, XExtCodes *codes)
{
    cairo_xlib_display_t *display, **prev, *next;

    


    CAIRO_MUTEX_LOCK (_cairo_xlib_display_mutex);
    prev = &_cairo_xlib_display_list;
    for (display = _cairo_xlib_display_list; display; display = next) {
	next = display->next;
	if (display->display == dpy) {
	    cairo_xlib_error_func_t old_handler;

	    
	    CAIRO_MUTEX_UNLOCK (_cairo_xlib_display_mutex);

	    
	    XSync (dpy, False);
	    old_handler = XSetErrorHandler (_noop_error_handler);

	    _cairo_xlib_display_notify (display);
	    _cairo_xlib_call_close_display_hooks (display);
	    _cairo_xlib_display_discard_screens (display);

	    
	    _cairo_xlib_display_notify (display);

	    XSync (dpy, False);
	    XSetErrorHandler (old_handler);

	    CAIRO_MUTEX_LOCK (_cairo_xlib_display_mutex);
	    _cairo_xlib_display_destroy (display);
	    *prev = next;
	    break;
	} else
	    prev = &display->next;
    }
    CAIRO_MUTEX_UNLOCK (_cairo_xlib_display_mutex);

    

    return 0;
}

cairo_xlib_display_t *
_cairo_xlib_display_get (Display *dpy)
{
    cairo_xlib_display_t *display;
    cairo_xlib_display_t **prev;
    XExtCodes *codes;

    






    CAIRO_MUTEX_LOCK (_cairo_xlib_display_mutex);

    for (prev = &_cairo_xlib_display_list; (display = *prev); prev = &(*prev)->next)
    {
	if (display->display == dpy) {
	    


	    if (prev != &_cairo_xlib_display_list) {
		*prev = display->next;
		display->next = _cairo_xlib_display_list;
		_cairo_xlib_display_list = display;
	    }
	    break;
	}
    }

    if (display != NULL) {
	display = _cairo_xlib_display_reference (display);
	goto UNLOCK;
    }

    display = malloc (sizeof (cairo_xlib_display_t));
    if (display == NULL)
	goto UNLOCK;

    codes = XAddExtension (dpy);
    if (codes == NULL) {
	free (display);
	display = NULL;
	goto UNLOCK;
    }

    XESetCloseDisplay (dpy, codes->extension, _cairo_xlib_close_display);

    _cairo_freelist_init (&display->wq_freelist, sizeof (cairo_xlib_job_t));
    _cairo_freelist_init (&display->hook_freelist, sizeof (cairo_xlib_hook_t));

    display->ref_count = 2; 
    CAIRO_MUTEX_INIT (display->mutex);
    display->display = dpy;
    display->screens = NULL;
    display->workqueue = NULL;
    display->close_display_hooks = NULL;
    display->closed = FALSE;

    display->next = _cairo_xlib_display_list;
    _cairo_xlib_display_list = display;

UNLOCK:
    CAIRO_MUTEX_UNLOCK (_cairo_xlib_display_mutex);
    return display;
}

cairo_bool_t
_cairo_xlib_add_close_display_hook (Display *dpy, void (*func) (Display *, void *), void *data, const void *key)
{
    cairo_xlib_display_t *display;
    cairo_xlib_hook_t *hook;
    cairo_bool_t ret = FALSE;

    display = _cairo_xlib_display_get (dpy);
    if (display == NULL)
	return FALSE;

    CAIRO_MUTEX_LOCK (display->mutex);
    if (display->closed == FALSE) {
	hook = _cairo_freelist_alloc (&display->hook_freelist);
	if (hook != NULL) {
	    hook->func = func;
	    hook->data = data;
	    hook->key = key;

	    hook->next = display->close_display_hooks;
	    display->close_display_hooks = hook;
	    ret = TRUE;
	}
    }
    CAIRO_MUTEX_UNLOCK (display->mutex);

    _cairo_xlib_display_destroy (display);

    return ret;
}

void
_cairo_xlib_remove_close_display_hooks (Display *dpy, const void *key)
{
    cairo_xlib_display_t *display;
    cairo_xlib_hook_t *hook, *next, **prev;

    display = _cairo_xlib_display_get (dpy);
    if (display == NULL)
	return;

    CAIRO_MUTEX_LOCK (display->mutex);
    prev = &display->close_display_hooks;
    for (hook = display->close_display_hooks; hook != NULL; hook = next) {
	next = hook->next;
	if (hook->key == key) {
	    *prev = hook->next;
	    _cairo_freelist_free (&display->hook_freelist, hook);
	} else
	    prev = &hook->next;
    }
    *prev = NULL;
    CAIRO_MUTEX_UNLOCK (display->mutex);

    _cairo_xlib_display_destroy (display);
}

cairo_status_t
_cairo_xlib_display_queue_resource (cairo_xlib_display_t *display,
	                            cairo_xlib_notify_resource_func notify,
				    XID xid)
{
    cairo_xlib_job_t *job;
    cairo_status_t status = CAIRO_STATUS_NO_MEMORY;

    CAIRO_MUTEX_LOCK (display->mutex);
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
    CAIRO_MUTEX_UNLOCK (display->mutex);

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

    CAIRO_MUTEX_LOCK (display->mutex);
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
    CAIRO_MUTEX_UNLOCK (display->mutex);

    return status;
}

void
_cairo_xlib_display_notify (cairo_xlib_display_t *display)
{
    cairo_xlib_job_t *jobs, *job, *freelist;
    Display *dpy = display->display;

    CAIRO_MUTEX_LOCK (display->mutex);
    jobs = display->workqueue;
    while (jobs != NULL) {
	display->workqueue = NULL;
	CAIRO_MUTEX_UNLOCK (display->mutex);

	
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

	CAIRO_MUTEX_LOCK (display->mutex);
	do {
	    job = freelist;
	    freelist = job->next;
	    _cairo_freelist_free (&display->wq_freelist, job);
	} while (freelist != NULL);

	jobs = display->workqueue;
    }
    CAIRO_MUTEX_UNLOCK (display->mutex);
}
