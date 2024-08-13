/* This is based on foreign_toplevel_management in wlroots. */

#ifndef BOX_FOREIGN_TOPLEVEL_MANAGEMENT_V1_H
#define BOX_FOREIGN_TOPLEVEL_MANAGEMENT_V1_H

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>

struct view;

struct box_foreign_toplevel_manager_v1 {
    struct wl_event_loop *event_loop;
    struct wl_global *global;
    struct wl_list resources; // wl_resource_get_link()
    struct wl_list toplevels; // box_foreign_toplevel_handle_v1.link

    struct wl_listener display_destroy;

    struct {
        struct wl_signal destroy;
    } events;

    void *data;
};

enum box_foreign_toplevel_handle_v1_state {
    BOX_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED = (1 << 0),
    BOX_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED = (1 << 1),
    BOX_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED = (1 << 2),
    BOX_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN = (1 << 3),
};

struct box_foreign_toplevel_handle_v1_output {
    struct wl_list link; // box_foreign_toplevel_handle_v1.outputs
    struct wlr_output *output;
    struct box_foreign_toplevel_handle_v1 *toplevel;

    // private state

    struct wl_listener output_bind;
    struct wl_listener output_destroy;
};

struct box_foreign_toplevel_handle_v1 {
    struct box_foreign_toplevel_manager_v1 *manager;
    struct wl_list resources;
    struct wl_list link;
    struct wl_event_source *idle_source;

    char *title;
    char *app_id;
    struct box_foreign_toplevel_handle_v1 *parent;
    struct wl_list outputs; // box_foreign_toplevel_v1_output.link
    uint32_t state; // enum box_foreign_toplevel_v1_state

    struct {
        // struct box_foreign_toplevel_handle_v1_maximized_event
        struct wl_signal request_maximize;
        // struct box_foreign_toplevel_handle_v1_minimized_event
        struct wl_signal request_minimize;
        // struct box_foreign_toplevel_handle_v1_activated_event
        struct wl_signal request_activate;
        // struct box_foreign_toplevel_handle_v1_fullscreen_event
        struct wl_signal request_fullscreen;
        struct wl_signal request_close;

        // struct box_foreign_toplevel_handle_v1_set_rectangle_event
        struct wl_signal set_rectangle;
        struct wl_signal destroy;

        struct wl_signal request_move;
    } events;

    void *data;
};

struct box_foreign_toplevel_handle_v1_maximized_event {
    struct box_foreign_toplevel_handle_v1 *toplevel;
    bool maximized;
};

struct box_foreign_toplevel_handle_v1_minimized_event {
    struct box_foreign_toplevel_handle_v1 *toplevel;
    bool minimized;
};

struct box_foreign_toplevel_handle_v1_activated_event {
    struct box_foreign_toplevel_handle_v1 *toplevel;
    struct wlr_seat *seat;
};

struct box_foreign_toplevel_handle_v1_fullscreen_event {
    struct box_foreign_toplevel_handle_v1 *toplevel;
    bool fullscreen;
    struct wlr_output *output;
};

struct box_foreign_toplevel_handle_v1_set_rectangle_event {
    struct box_foreign_toplevel_handle_v1 *toplevel;
    struct wlr_surface *surface;
    int32_t x, y, width, height;
};

struct box_foreign_toplevel_handle_v1_move_event {
    struct box_foreign_toplevel_handle_v1 *toplevel;
    int32_t x, y;
};

struct box_foreign_toplevel_manager_v1 *box_foreign_toplevel_manager_v1_create(
    struct wl_display *display);

struct box_foreign_toplevel_handle_v1 *box_foreign_toplevel_handle_v1_create(
    struct box_foreign_toplevel_manager_v1 *manager);
/**
 * Destroy the given toplevel handle, sending the closed event to any
 * client. Also, if the destroyed toplevel is set as a parent of any
 * other valid toplevel, clients still holding a handle to both are
 * sent a parent signal with NULL parent. If this is not desired, the
 * caller should ensure that any child toplevels are destroyed before
 * the parent.
 */
void box_foreign_toplevel_handle_v1_destroy(
    struct box_foreign_toplevel_handle_v1 *toplevel);

void box_foreign_toplevel_handle_v1_set_title(
    struct box_foreign_toplevel_handle_v1 *toplevel, const char *title);
void box_foreign_toplevel_handle_v1_set_app_id(
    struct box_foreign_toplevel_handle_v1 *toplevel, const char *app_id);

void box_foreign_toplevel_handle_v1_output_enter(
    struct box_foreign_toplevel_handle_v1 *toplevel, struct wlr_output *output);
void box_foreign_toplevel_handle_v1_output_leave(
    struct box_foreign_toplevel_handle_v1 *toplevel, struct wlr_output *output);

void box_foreign_toplevel_handle_v1_set_maximized(
    struct box_foreign_toplevel_handle_v1 *toplevel, bool maximized);
void box_foreign_toplevel_handle_v1_set_minimized(
    struct box_foreign_toplevel_handle_v1 *toplevel, bool minimized);
void box_foreign_toplevel_handle_v1_set_activated(
    struct box_foreign_toplevel_handle_v1 *toplevel, bool activated);
void box_foreign_toplevel_handle_v1_set_fullscreen(
    struct box_foreign_toplevel_handle_v1* toplevel, bool fullscreen);

/**
 * Set the parent of a toplevel. If the parent changed from its previous
 * value, also sends a parent event to all clients that hold handles to
 * both toplevel and parent (no message is sent to clients that have
 * previously destroyed their parent handle). NULL is allowed as the
 * parent, meaning no parent exists.
 */
void box_foreign_toplevel_handle_v1_set_parent(
    struct box_foreign_toplevel_handle_v1 *toplevel,
    struct box_foreign_toplevel_handle_v1 *parent);


#endif
