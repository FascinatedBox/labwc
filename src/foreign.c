// SPDX-License-Identifier: GPL-2.0-only
#include <assert.h>
#include "labwc.h"
#include "view.h"
#include "workspaces.h"

static void
handle_request_minimize(struct wl_listener *listener, void *data)
{
	struct view *view = wl_container_of(listener, view, toplevel.minimize);
	struct box_foreign_toplevel_handle_v1_minimized_event *event = data;
	view_minimize(view, event->minimized);
}

static void
handle_request_maximize(struct wl_listener *listener, void *data)
{
	struct view *view = wl_container_of(listener, view, toplevel.maximize);
	struct box_foreign_toplevel_handle_v1_maximized_event *event = data;
	view_maximize(view, event->maximized ? VIEW_AXIS_BOTH : VIEW_AXIS_NONE,
		/*store_natural_geometry*/ true);
}

static void
handle_request_fullscreen(struct wl_listener *listener, void *data)
{
	struct view *view = wl_container_of(listener, view, toplevel.fullscreen);
	struct box_foreign_toplevel_handle_v1_fullscreen_event *event = data;
	view_set_fullscreen(view, event->fullscreen);
}

static void
handle_request_activate(struct wl_listener *listener, void *data)
{
	struct view *view = wl_container_of(listener, view, toplevel.activate);
	// struct box_foreign_toplevel_handle_v1_activated_event *event = data;

	if (view->server->osd_state.cycle_view) {
		wlr_log(WLR_INFO, "Preventing focus request while in window switcher");
		return;
	}

	/* In a multi-seat world we would select seat based on event->seat here. */
	desktop_focus_view(view, /*raise*/ true);
}

static void
handle_request_close(struct wl_listener *listener, void *data)
{
	struct view *view = wl_container_of(listener, view, toplevel.close);
	view_close(view);
}

static void
handle_destroy(struct wl_listener *listener, void *data)
{
	struct view *view = wl_container_of(listener, view, toplevel.destroy);
	struct foreign_toplevel *toplevel = &view->toplevel;
	wl_list_remove(&toplevel->maximize.link);
	wl_list_remove(&toplevel->minimize.link);
	wl_list_remove(&toplevel->fullscreen.link);
	wl_list_remove(&toplevel->activate.link);
	wl_list_remove(&toplevel->close.link);
	wl_list_remove(&toplevel->destroy.link);
	toplevel->handle = NULL;
}

static void
handle_move(struct wl_listener *listener, void *data)
{
	struct view *view = wl_container_of(listener, view, toplevel.move);
	struct box_foreign_toplevel_handle_v1_move_event *event = data;

	// Account for SSD (margins are 0 for CSD).
	struct border b = ssd_get_margin(view->ssd);

	event->x += b.left;
	event->y += b.top;

	view_move(view, event->x, event->y);
}

void
foreign_toplevel_handle_create(struct view *view)
{
	struct foreign_toplevel *toplevel = &view->toplevel;

	toplevel->handle = box_foreign_toplevel_handle_v1_create(
		view->server->box_auto_manager);
	if (!toplevel->handle) {
		wlr_log(WLR_ERROR, "cannot create foreign toplevel handle for (%s)",
			view_get_string_prop(view, "title"));
		return;
	}

	toplevel->move.notify = handle_move;
	wl_signal_add(&toplevel->handle->events.request_move, &toplevel->move);

	toplevel->maximize.notify = handle_request_maximize;
	wl_signal_add(&toplevel->handle->events.request_maximize,
		&toplevel->maximize);

	toplevel->minimize.notify = handle_request_minimize;
	wl_signal_add(&toplevel->handle->events.request_minimize,
		&toplevel->minimize);

	toplevel->fullscreen.notify = handle_request_fullscreen;
	wl_signal_add(&toplevel->handle->events.request_fullscreen,
		&toplevel->fullscreen);

	toplevel->activate.notify = handle_request_activate;
	wl_signal_add(&toplevel->handle->events.request_activate,
		&toplevel->activate);

	toplevel->close.notify = handle_request_close;
	wl_signal_add(&toplevel->handle->events.request_close,
		&toplevel->close);

	toplevel->destroy.notify = handle_destroy;
	wl_signal_add(&toplevel->handle->events.destroy, &toplevel->destroy);
}

/*
 * Loop over all outputs and notify foreign_toplevel clients about changes.
 * box_foreign_toplevel_handle_v1_output_xxx() keeps track of the active
 * outputs internally and merges the events. It also listens to output
 * destroy events so its fine to just relay the current state and let
 * box_foreign_toplevel handle the rest.
 */
void
foreign_toplevel_update_outputs(struct view *view)
{
	assert(view->toplevel.handle);

	struct output *output;
	wl_list_for_each(output, &view->server->outputs, link) {
		if (view_on_output(view, output)) {
			box_foreign_toplevel_handle_v1_output_enter(
				view->toplevel.handle, output->wlr_output);
		} else {
			box_foreign_toplevel_handle_v1_output_leave(
				view->toplevel.handle, output->wlr_output);
		}
	}
}
