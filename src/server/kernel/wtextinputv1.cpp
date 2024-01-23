// Copyright (C) 2024 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wtextinputv1.h"
#include <wseat.h>
#include <wsurface.h>
#include <wtools.h>

#include <qwcompositor.h>
#include <qwdisplay.h>
#include <qwsignalconnector.h>
#include <qwseat.h>

extern "C" {
#include <wlr/types/wlr_seat.h>
#define static
#include <wlr/types/wlr_compositor.h>
#undef static
#include <wlr/util/box.h>
#include <text-input-unstable-v1-protocol.h>
}


QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE
struct ws_text_input_v1 {
    wl_resource *resource;

    wlr_surface *focused_surface;

    uint32_t current_serial;

    wlr_seat *seat;

    bool active;

    wl_listener surface_destroy;

    struct {
        char *text;
        uint32_t cursor;
        uint32_t anchor;
    } surrounding_text;

    struct {
        uint32_t hint;
        uint32_t purpose;
    } content_type;

    struct {
        uint32_t button;
        uint32_t index;
    } invoke_action;

    wlr_box cursor_rectangle;

    char *preferred_language;

    struct {
        wl_signal activate;
        wl_signal deactivate;
        wl_signal show_input_panel;
        wl_signal hide_input_panel;
        wl_signal reset;
        wl_signal commit_state;
        wl_signal invoke_action;
        wl_signal destroy;

        wl_signal set_surrounding_text;
        wl_signal set_content_type;
        wl_signal set_cursor_rectangle;
        wl_signal set_preferred_language;
        wl_signal set_focused_surface;
        wl_signal seat_changed;
    } events;

    wl_list link;

    ws_text_input_v1()
        : resource{nullptr}
        , focused_surface{nullptr}
        , current_serial{0}
        , seat{nullptr}
        , active{false}
        , surface_destroy{}
        , surrounding_text{}
        , content_type{}
        , invoke_action{}
        , cursor_rectangle{}
        , preferred_language{nullptr}
        , events{}
        , link{}
    {
        wl_signal_init(&events.destroy);
        wl_signal_init(&events.activate);
        wl_signal_init(&events.deactivate);
        wl_signal_init(&events.show_input_panel);
        wl_signal_init(&events.hide_input_panel);
        wl_signal_init(&events.set_surrounding_text);
        wl_signal_init(&events.set_content_type);
        wl_signal_init(&events.set_cursor_rectangle);
        wl_signal_init(&events.set_preferred_language);
        wl_signal_init(&events.commit_state);
        wl_signal_init(&events.invoke_action);
        wl_signal_init(&events.reset);
        wl_signal_init(&events.set_focused_surface);
        wl_signal_init(&events.seat_changed);
        wl_list_init(&surface_destroy.link);
        wl_list_init(&link);
    }
};

struct ws_text_input_manager_v1
{
    wl_list text_inputs;
    wl_global *global;

    wl_listener display_destroy;

    struct {
        wl_signal text_input;
        wl_signal destroy;
    } events;
};

static ws_text_input_manager_v1 *text_input_manager_v1_create(wl_display *display);
static ws_text_input_v1 *text_input_from_resource(wl_resource *resource);
void text_input_handle_activate(struct wl_client *client,
                                struct wl_resource *resource,
                                struct wl_resource *seat,
                                struct wl_resource *surface)
{
    wlr_seat_client *seat_client =  wlr_seat_client_from_resource(seat);
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    bool seat_changed = (text_input->seat == seat_client->seat);
    text_input->seat = seat_client->seat;
    wlr_surface *focused_surface = wlr_surface_from_resource(surface);
    bool focus_changed = (text_input->focused_surface == focused_surface);
    wl_signal_add(&focused_surface->events.destroy, &text_input->surface_destroy);
    text_input->focused_surface = focused_surface;
    // FIXME: Does the order matter?
    if (seat_changed) {
        wl_signal_emit(&text_input->events.seat_changed, text_input);
    }
    if (focus_changed) {
        wl_signal_emit(&text_input->events.set_focused_surface, text_input);
    }
    wl_signal_emit(&text_input->events.activate, text_input);
}

void text_input_handle_deactivate(struct wl_client *client,
                                  struct wl_resource *resource,
                                  struct wl_resource *seat)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    wlr_seat_client *seat_client = wlr_seat_client_from_resource(seat);
    wl_signal_emit(&text_input->events.deactivate, seat_client->seat);
    if (text_input->seat) {
        text_input->seat = nullptr;
        wl_signal_emit(&text_input->events.seat_changed, text_input);
    }
}

void text_input_handle_show_input_panel(struct wl_client *client,
                                        struct wl_resource *resource)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    wl_signal_emit(&text_input->events.show_input_panel, text_input);
}

void text_input_handle_hide_input_panel(struct wl_client *client,
                                        struct wl_resource *resource)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    wl_signal_emit(&text_input->events.hide_input_panel, text_input);
}

void text_input_handle_reset(struct wl_client *client,
                             struct wl_resource *resource)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    free(text_input->surrounding_text.text);
    text_input->surrounding_text = {};
    text_input->cursor_rectangle = {};
    text_input->content_type = {};
    // TODO: Refer to weston-editor, these states must be cleared.
    wl_signal_emit(&text_input->events.reset, text_input);
}

void text_input_handle_set_surrounding_text(struct wl_client *client,
                                            struct wl_resource *resource,
                                            const char *text,
                                            uint32_t cursor,
                                            uint32_t anchor)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    free(text_input->surrounding_text.text);
    text_input->surrounding_text.text = strdup(text);
    text_input->surrounding_text.cursor = cursor;
    text_input->surrounding_text.anchor = anchor;
    wl_signal_emit(&text_input->events.set_surrounding_text, text_input);
}

void text_input_handle_set_content_type(struct wl_client *client,
                                        struct wl_resource *resource,
                                        uint32_t hint,
                                        uint32_t purpose)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    text_input->content_type.hint = hint;
    text_input->content_type.purpose = purpose;
    wl_signal_emit(&text_input->events.set_content_type, text_input);
}

void text_input_handle_set_cursor_rectangle(struct wl_client *client,
                                            struct wl_resource *resource,
                                            int32_t x,
                                            int32_t y,
                                            int32_t width,
                                            int32_t height)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    text_input->cursor_rectangle.x = x;
    text_input->cursor_rectangle.y = y;
    text_input->cursor_rectangle.width = width;
    text_input->cursor_rectangle.height = height;
    wl_signal_emit(&text_input->events.set_cursor_rectangle, text_input);
}

void text_input_handle_set_preferred_language(struct wl_client *client,
                                              struct wl_resource *resource,
                                              const char *language)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    free(text_input->preferred_language);
    text_input->preferred_language = strdup(language);
    if (!text_input->preferred_language) {
        wl_client_post_no_memory(client);
    } else {
        wl_signal_emit(&text_input->events.set_preferred_language, text_input);
    }
}

void text_input_handle_commit_state(struct wl_client *client,
                                    struct wl_resource *resource,
                                    uint32_t serial)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    text_input->current_serial = serial;
    wl_signal_emit(&text_input->events.commit_state, text_input);
}

void text_input_handle_invoke_action(struct wl_client *client,
                                     struct wl_resource *resource,
                                     uint32_t button,
                                     uint32_t index)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    text_input->invoke_action.button = button;
    text_input->invoke_action.index = index;
    wl_signal_emit(&text_input->events.invoke_action, text_input);
}

static const struct zwp_text_input_v1_interface text_input_v1_impl = {
    .activate = text_input_handle_activate,
    .deactivate = text_input_handle_deactivate,
    .show_input_panel = text_input_handle_show_input_panel,
    .hide_input_panel = text_input_handle_hide_input_panel,
    .reset = text_input_handle_reset,
    .set_surrounding_text = text_input_handle_set_surrounding_text,
    .set_content_type = text_input_handle_set_content_type,
    .set_cursor_rectangle = text_input_handle_set_cursor_rectangle,
    .set_preferred_language = text_input_handle_set_preferred_language,
    .commit_state = text_input_handle_commit_state,
    .invoke_action = text_input_handle_invoke_action
};

ws_text_input_v1 *text_input_from_resource(wl_resource *resource)
{
    assert(wl_resource_instance_of(resource, &zwp_text_input_v1_interface, &text_input_v1_impl));
    return reinterpret_cast<ws_text_input_v1 *>(wl_resource_get_user_data(resource));
}

static void text_input_resource_destroy(wl_resource * resource)
{
    ws_text_input_v1 *text_input = text_input_from_resource(resource);
    wl_signal_emit(&text_input->events.destroy, text_input);
    // TODO: Release resource and recycle.
}

static void text_input_handle_focused_surface_destroy(struct wl_listener *listener, void *data)
{
    // FIXME: What should we do when surface is destroyed but deactivate signal not received
    // Should we deactivate the text input automatically or just send leave
    ws_text_input_v1 *text_input = wl_container_of(listener, text_input, surface_destroy);
    wl_list_remove(&text_input->surface_destroy.link);
    wl_list_init(&text_input->surface_destroy.link);
    text_input->focused_surface = nullptr;
    wl_signal_emit(&text_input->events.set_focused_surface, text_input);
    zwp_text_input_v1_send_leave(text_input->resource);
}

ws_text_input_manager_v1 *text_input_manager_from_resource(wl_resource *resource);

static void text_input_manager_get_text_input(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    int version = wl_resource_get_version(resource);
    struct wl_resource *text_input_resource = wl_resource_create(client, &zwp_text_input_v1_interface, version, id);
    if (!text_input_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(text_input_resource, &text_input_v1_impl, nullptr, text_input_resource_destroy);
    ws_text_input_v1 *text_input = new ws_text_input_v1;
    text_input->resource = text_input_resource;
    wl_resource_set_user_data(text_input_resource, text_input);
    text_input->surface_destroy.notify = text_input_handle_focused_surface_destroy;

    ws_text_input_manager_v1 *manager = text_input_manager_from_resource(resource);
    wl_list_insert(&manager->text_inputs, &text_input->link);
    wl_signal_emit(&manager->events.text_input, text_input);
}

static struct zwp_text_input_manager_v1_interface text_input_manager_v1_impl = {
    .create_text_input = text_input_manager_get_text_input
};

ws_text_input_manager_v1 *text_input_manager_from_resource(wl_resource *resource)
{
    assert(wl_resource_instance_of(resource, &zwp_text_input_manager_v1_interface, &text_input_manager_v1_impl));
    return reinterpret_cast<ws_text_input_manager_v1 *>(resource->data);
}

static void text_input_manager_bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id)
{
    ws_text_input_manager_v1 *manager = reinterpret_cast<ws_text_input_manager_v1 *>(data);
    wl_resource *resource = wl_resource_create(wl_client, &zwp_text_input_manager_v1_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(wl_client);
        return;
    }
    wl_resource_set_implementation(resource, &text_input_manager_v1_impl, manager, nullptr);
}

static void handle_display_destroy(wl_listener *listener, void *data)
{
    ws_text_input_manager_v1 *manager = wl_container_of(listener, manager, display_destroy);
    wl_signal_emit(&manager->events.destroy, manager);
    wl_list_remove(&manager->display_destroy.link);
    wl_global_destroy(manager->global);
    delete manager;
}

ws_text_input_manager_v1 *text_input_manager_v1_create(wl_display *display)
{
    ws_text_input_manager_v1 *manager = new ws_text_input_manager_v1;

    wl_list_init(&manager->text_inputs);
    wl_signal_init(&manager->events.text_input);
    wl_signal_init(&manager->events.destroy);

    manager->global = wl_global_create(display,
                                       &zwp_text_input_manager_v1_interface,
                                       1,
                                       manager,
                                       text_input_manager_bind);
    if (!manager->global) {
        delete manager;
        return nullptr;
    }

    manager->display_destroy.notify = handle_display_destroy;
    wl_display_add_destroy_listener(display, &manager->display_destroy);

    return manager;
}

class WTextInputManagerV1Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WTextInputManagerV1)
    explicit WTextInputManagerV1Private(ws_text_input_manager_v1 *h, WTextInputManagerV1 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    {
        sc.connect(&handle->events.text_input, this, &WTextInputManagerV1Private::on_text_input);
    }

    void on_text_input(ws_text_input_v1 *text_input)
    {
        auto ti = new WTextInputV1(text_input, nullptr); // FIXME: should we make this ti a child of manager?
        Q_EMIT q_func()->newTextInput(ti);
    }

    ws_text_input_manager_v1 *const handle;
    QWSignalConnector sc;
};

WTextInputManagerV1::WTextInputManagerV1(ws_text_input_manager_v1 *handle)
    : QObject(nullptr)
    , WObject(*new WTextInputManagerV1Private(handle, this))
{ }

WTextInputManagerV1 *WTextInputManagerV1::create(QWDisplay *display)
{
    auto manager = text_input_manager_v1_create(display->handle());
    return manager ? new WTextInputManagerV1(manager) : nullptr;
}

class WTextInputV1Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WTextInputV1)
    explicit WTextInputV1Private(ws_text_input_v1 *h, WTextInputV1 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    {
        sc.connect(&handle->events.activate, this, &WTextInputV1Private::on_activate);
        sc.connect(&handle->events.deactivate, this, &WTextInputV1Private::on_deactivate);
        sc.connect(&handle->events.show_input_panel, this, &WTextInputV1Private::on_show_input_panel);
        sc.connect(&handle->events.hide_input_panel, this, &WTextInputV1Private::on_hide_input_panel);
        sc.connect(&handle->events.reset, this, &WTextInputV1Private::on_reset);
        sc.connect(&handle->events.commit_state, this, &WTextInputV1Private::on_commit_state);
        sc.connect(&handle->events.invoke_action, this, &WTextInputV1Private::on_invoke_action);
        sc.connect(&handle->events.destroy, this, &WTextInputV1Private::on_destroy);
        sc.connect(&handle->events.set_surrounding_text, qq, &WTextInputV1::surroundingTextChanged);
        sc.connect(&handle->events.set_content_type, qq, &WTextInputV1::contentTypeChanged);
        sc.connect(&handle->events.set_cursor_rectangle, qq, &WTextInputV1::cursorRectangleChanged);
        sc.connect(&handle->events.set_preferred_language, qq, &WTextInputV1::preferredLanguageChanged);
        sc.connect(&handle->events.set_focused_surface, this, &WTextInputV1Private::on_set_focused_surface);
        sc.connect(&handle->events.seat_changed, this, &WTextInputV1Private::on_set_seat);
    }

    void on_activate(ws_text_input_v1 *text_input)
    {
        auto seat = WSeat::fromHandle(QWSeat::from(text_input->seat));
        auto surface = WSurface::fromHandle(text_input->focused_surface);
        Q_ASSERT(seat && surface);
        Q_EMIT q_func()->activate(seat, surface);
    }

    void on_deactivate(wlr_seat *seat)
    {
        Q_ASSERT(seat);
        Q_EMIT q_func()->deactivate(WSeat::fromHandle(QWSeat::from(seat)));
    }

    void on_show_input_panel(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->showInputPanel();
    }

    void on_hide_input_panel(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->hideInputPanel();
    }

    void on_reset(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->reset();
    }

    void on_commit_state(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->commitState();
    }

    void on_invoke_action(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->invokeAction(text_input->invoke_action.button, text_input->invoke_action.index);
    }

    void on_destroy(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->beforeDestroy(q_func());
    }

    void on_set_focused_surface(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->focusedSurfaceChanged(WSurface::fromHandle(text_input->focused_surface));
    }

    void on_set_seat(ws_text_input_v1 *text_input)
    {
        Q_EMIT q_func()->seatChanged(text_input->seat ? WSeat::fromHandle(QWSeat::from(text_input->seat)) : nullptr);
    }

    ws_text_input_v1 *const handle;
    QWSignalConnector sc;
};

WTextInputV1::WTextInputV1(ws_text_input_v1 *handle, QObject *parent)
    : QObject(parent)
    , WObject(*new WTextInputV1Private(handle, this))
{ }

WSeat *WTextInputV1::seat() const
{
    return d_func()->handle->seat ? WSeat::fromHandle(QWSeat::from(d_func()->handle->seat)) : nullptr;
}

WSurface *WTextInputV1::focusedSurface() const
{
    return WSurface::fromHandle(d_func()->handle->focused_surface);
}

QString WTextInputV1::surroundingText() const
{
    return d_func()->handle->surrounding_text.text;
}

uint WTextInputV1::surroundingCursor() const
{
    return d_func()->handle->surrounding_text.cursor;
}

uint WTextInputV1::surroundingAnchor() const
{
    return d_func()->handle->surrounding_text.anchor;
}

uint WTextInputV1::contentHint() const
{
    return d_func()->handle->content_type.hint;
}

uint WTextInputV1::contentPurpose() const
{
    return d_func()->handle->content_type.purpose;
}

QRect WTextInputV1::cursorRectangle() const
{
    return WTools::fromWLRBox(&d_func()->handle->cursor_rectangle);
}

QString WTextInputV1::preferredLanguage() const
{
    return d_func()->handle->preferred_language;
}

wl_client *WTextInputV1::waylandClient() const
{
    return wl_resource_get_client(d_func()->handle->resource);
}

void WTextInputV1::sendEnter(WSurface *surface)
{
    zwp_text_input_v1_send_enter(d_func()->handle->resource, surface->handle()->handle()->resource);
}

void WTextInputV1::sendLeave()
{
    zwp_text_input_v1_send_leave(d_func()->handle->resource);
}

void WTextInputV1::sendModifiersMap(QStringList map)
{
    wl_array arr;
    wl_array_init(&arr);
    for (const auto &modifier : map) {
        char *dest = static_cast<char *>(wl_array_add(&arr, modifier.length() + 1));
        strncpy(dest, modifier.toLatin1().data(), static_cast<uint>(modifier.length()));
    }
    zwp_text_input_v1_send_modifiers_map(d_func()->handle->resource, &arr);
    wl_array_release(&arr);
}

void WTextInputV1::sendInputPanelState(uint state)
{
    zwp_text_input_v1_send_input_panel_state(d_func()->handle->resource, state);
}

void WTextInputV1::sendPreeditString(const QString &text, const QString &commit)
{
    zwp_text_input_v1_send_preedit_string(d_func()->handle->resource, d_func()->handle->current_serial, text.toStdString().c_str(), commit.toStdString().c_str());
}

void WTextInputV1::sendPreeditStyling(uint index, uint length, uint style)
{
    zwp_text_input_v1_send_preedit_styling(d_func()->handle->resource, index, length, style);
}

void WTextInputV1::sendPreeditCursor(int index)
{
    zwp_text_input_v1_send_preedit_cursor(d_func()->handle->resource, index);
}

void WTextInputV1::sendCommitString(const QString &text)
{
    zwp_text_input_v1_send_commit_string(d_func()->handle->resource, d_func()->handle->current_serial, text.toStdString().c_str());
}

void WTextInputV1::sendCursorPosition(int index, int anchor)
{
    zwp_text_input_v1_send_cursor_position(d_func()->handle->resource, index, anchor);
}

void WTextInputV1::sendDeleteSurroundingString(int index, uint length)
{
    zwp_text_input_v1_send_delete_surrounding_text(d_func()->handle->resource, index, length);
}

void WTextInputV1::sendKeysym(uint time, uint sym, uint state, uint modifiers)
{
    zwp_text_input_v1_send_keysym(d_func()->handle->resource, d_func()->handle->current_serial, time, sym, state, modifiers);
}

void WTextInputV1::sendLanguage(const QString &language)
{
    zwp_text_input_v1_send_language(d_func()->handle->resource, d_func()->handle->current_serial, language.toStdString().c_str());
}

void WTextInputV1::sendTextDirection(uint direction)
{
    zwp_text_input_v1_send_text_direction(d_func()->handle->resource, d_func()->handle->current_serial, direction);
}
WAYLIB_SERVER_END_NAMESPACE
