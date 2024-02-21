// Copyright (C) 2024 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wtextinputv2.h"
#include <wtools.h>
#include <qwcompositor.h>
#include <wseat.h>
#include <qwsignalconnector.h>
#include <qwseat.h>
extern "C" {
#include <wlr/types/wlr_seat.h>
#define static
#include <wlr/types/wlr_compositor.h>
#undef static
#include <wlr/util/box.h>
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE
struct ws_text_input_manager_v2 {
    wl_list text_inputs;
    wl_global *global;

    wl_listener display_destroy;

    struct {
        wl_signal text_input;
        wl_signal destroy;
    } events;
};

struct ws_text_input_v2 {
    wl_resource *resource;

    wlr_surface *focused_surface;

    uint32_t current_serial;
    uint32_t reason;

    wlr_seat *seat;
    bool active;

    wl_listener surface_destroy;

    struct {
        char *text;
        int cursor;
        int anchor;
    } surrounding_text;

    struct {
        uint32_t hint;
        uint32_t purpose;
    } content_type;

    char *preferred_language;

    wlr_box cursor_rectangle;

    struct {
        wl_signal destroy;
        wl_signal enable;
        wl_signal disable;
        wl_signal show_input_panel;
        wl_signal hide_input_panel;
        wl_signal update_state;
        wl_signal set_surrounding_text;
        wl_signal set_content_type;
        wl_signal set_cursor_rectangle;
        wl_signal set_preferred_language;
        wl_signal set_focused_surface;
    } events;

    wl_list link;
};

class WTextInputV2Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WTextInputV2)
    WTextInputV2Private(ws_text_input_v2 *h, WTextInputV2 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    {
        sc.connect(&handle->events.destroy, qq, &WTextInputV2::beforeDestroy);
        sc.connect(&handle->events.enable, this, &WTextInputV2Private::on_enable);
        sc.connect(&handle->events.disable, this, &WTextInputV2Private::on_disable);
        sc.connect(&handle->events.show_input_panel, qq, &WTextInputV2::showInputPanel);
        sc.connect(&handle->events.hide_input_panel, qq, &WTextInputV2::hideInputPanel);
        sc.connect(&handle->events.update_state, this, &WTextInputV2Private::on_update_state);
        sc.connect(&handle->events.set_surrounding_text, qq, &WTextInputV2::surroundingTextChanged);
        sc.connect(&handle->events.set_content_type, qq, &WTextInputV2::contentTypeChanged);
        sc.connect(&handle->events.set_cursor_rectangle, qq, &WTextInputV2::cursorRectangleChanged);
        sc.connect(&handle->events.set_preferred_language, qq, &WTextInputV2::preferredLanguageChanged);
        sc.connect(&handle->events.set_focused_surface, qq, &WTextInputV2::focusedSurfaceChanged);
    }

    WSurface *focusedSurface() const
    {
        Q_ASSERT(handle);
        WSurface *surface = WSurface::fromHandle(handle->focused_surface);
        Q_ASSERT(surface);
        return surface;
    }

    void on_enable(ws_text_input_v2 *text_input)
    {
        Q_EMIT q_func()->enable(focusedSurface());
    }

    void on_disable(ws_text_input_v2 *text_input)
    {
        Q_EMIT q_func()->disable(focusedSurface());
    }

    void on_update_state(ws_text_input_v2 *text_input)
    {
        Q_EMIT q_func()->updateState(text_input->reason);
    }

    ws_text_input_v2 *const handle;
    QWSignalConnector sc;
};

class WTextInputManagerV2Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WTextInputManagerV2)
    WTextInputManagerV2Private(ws_text_input_manager_v2 *h, WTextInputManagerV2 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    {
        sc.connect(&handle->events.text_input, this, &WTextInputManagerV2Private::on_new_text_input);
        sc.connect(&handle->events.destroy, qq, &WTextInputManagerV2::beforeDestroy);
    }

    void on_new_text_input(ws_text_input_v2 *text_input)
    {
        W_Q(WTextInputManagerV2);
        Q_EMIT q->newTextInputV2(new WTextInputV2(text_input));
    }

    ws_text_input_manager_v2 *const handle;
    QWSignalConnector sc;
};

WSeat *WTextInputV2::seat() const
{
    W_DC(WTextInputV2);
    return WSeat::fromHandle(QWSeat::from(d->handle->seat));
}

WSurface *WTextInputV2::focusedSurface() const
{
    W_DC(WTextInputV2);
    return d->focusedSurface();
}

QString WTextInputV2::surroundingText() const
{
    return d_func()->handle->surrounding_text.text;
}

int WTextInputV2::surroundingCursor() const
{
    return d_func()->handle->surrounding_text.cursor;
}

int WTextInputV2::surroundingAnchor() const
{
    return d_func()->handle->surrounding_text.anchor;
}

uint WTextInputV2::contentHint() const
{
    return d_func()->handle->content_type.hint;
}

uint WTextInputV2::contentPurpose() const
{
    return d_func()->handle->content_type.purpose;
}

QRect WTextInputV2::cursorRectangle() const
{
    return WTools::fromWLRBox(&d_func()->handle->cursor_rectangle);
}

QString WTextInputV2::preferredLanguage() const
{
    return d_func()->handle->preferred_language;
}

wl_client *WTextInputV2::waylandClient() const
{
    return wl_resource_get_client(d_func()->handle->resource);
}

void WTextInputV2::sendEnter(WSurface *surface)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_enter(handle->resource, handle->current_serial, surface->handle()->handle()->resource);
}

void WTextInputV2::sendLeave(WSurface *surface)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_leave(handle->resource, handle->current_serial, surface->handle()->handle()->resource);
}

void WTextInputV2::sendInputPanelState(uint visibility, QRect geometry)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_input_panel_state(handle->resource,
                                             visibility,
                                             geometry.x(),
                                             geometry.y(),
                                             geometry.width(),
                                             geometry.height());
}

void WTextInputV2::sendPreeditString(const QString &text, const QString &commit)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_preedit_string(handle->resource, text.toStdString().c_str(), commit.toStdString().c_str());
}

void WTextInputV2::sendPreeditStyling(uint index, uint length, uint style)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_preedit_styling(handle->resource, index, length, style);
}

void WTextInputV2::sendPreeditCursor(int index)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_preedit_cursor(handle->resource, index);
}

void WTextInputV2::sendCommitString(const QString &text)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_commit_string(handle->resource, text.toStdString().c_str());
}

void WTextInputV2::sendCursorPosition(int index, int anchor)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_cursor_position(handle->resource, index, anchor);
}

void WTextInputV2::sendDeleteSurroundingText(uint beforeLength, uint afterLength)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_delete_surrounding_text(handle->resource, beforeLength, afterLength);
}

void WTextInputV2::sendModifiersMap(const QStringList &map)
{
    auto handle = d_func()->handle;
    wl_array modifiers_map_arr;
    wl_array_init(&modifiers_map_arr);
    for (const QString &modifier : map) {
        QByteArray ba = modifier.toLatin1();
        char *p = static_cast<char *>(wl_array_add(&modifiers_map_arr, ba.length() + 1));
        strncpy(p, ba.data(), ba.length());
    }
    zwp_text_input_v2_send_modifiers_map(handle->resource, &modifiers_map_arr);
    wl_array_release(&modifiers_map_arr);
}

void WTextInputV2::sendKeysym(uint time, uint sym, uint state, uint modifiers)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_keysym(handle->resource, time, sym, state, modifiers);
}

void WTextInputV2::sendLanguage(const QString &language)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_language(handle->resource, language.toStdString().c_str());
}

void WTextInputV2::sendConfigureSurroundingText(int beforeCursor, int afterCursor)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_configure_surrounding_text(handle->resource, beforeCursor, afterCursor);
}

void WTextInputV2::sendInputMethodChanged(uint flags)
{
    auto handle = d_func()->handle;
    zwp_text_input_v2_send_input_method_changed(handle->resource, handle->current_serial, flags);
}

WTextInputV2::WTextInputV2(ws_text_input_v2 *handle)
    : QObject(nullptr)
    , WObject(*new WTextInputV2Private(handle, this))
{
}

static void text_input_destroy(wl_client *client, wl_resource *resource);
static void text_input_enable(wl_client *client, wl_resource *resource, wl_resource *surface);
static void text_input_disable(wl_client *client, wl_resource *resource, wl_resource *surface);
static void text_input_show_input_panel(wl_client *client, wl_resource *resource);
static void text_input_hide_input_panel(wl_client *client, wl_resource *resource);
static void text_input_set_surrounding_text(wl_client *client, wl_resource *resource, const char *text, int32_t cursor, int32_t anchor);
static void text_input_set_content_type(wl_client *client, wl_resource *resource, uint32_t hint, uint32_t purpose);
static void text_input_set_cursor_rectangle(wl_client *client, wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void text_input_set_preferred_language(wl_client *client, wl_resource *resource, const char *language);
static void text_input_update_state(wl_client *client, wl_resource *resource, uint32_t serial, uint32_t reason);
static ws_text_input_v2 *text_input_from_resource(wl_resource *resource);

static struct zwp_text_input_v2_interface text_input_v2_impl = {
    .destroy = text_input_destroy,
    .enable = text_input_enable,
    .disable = text_input_disable,
    .show_input_panel = text_input_show_input_panel,
    .hide_input_panel = text_input_hide_input_panel,
    .set_surrounding_text = text_input_set_surrounding_text,
    .set_content_type = text_input_set_content_type,
    .set_cursor_rectangle = text_input_set_cursor_rectangle,
    .set_preferred_language = text_input_set_preferred_language,
    .update_state = text_input_update_state
};

static void text_input_handle_focused_surface_destroy(wl_listener *listener, void *data)
{
    ws_text_input_v2 *text_input = wl_container_of(listener, text_input, surface_destroy);
    wl_list_remove(&text_input->surface_destroy.link);
    wl_list_init(&text_input->surface_destroy.link);
    text_input_disable(wl_resource_get_client(text_input->resource), text_input->resource, text_input->focused_surface->resource);
}

static void text_input_enable(wl_client *client, wl_resource *resource, wl_resource *surface)
{
    assert(client == wl_resource_get_client(resource) && client == wl_resource_get_client(surface));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    text_input->active = true;
    wlr_surface *focused_surface = wlr_surface_from_resource(surface);
    wl_signal_add(&focused_surface->events.destroy, &text_input->surface_destroy);
    text_input->focused_surface = focused_surface;
    wl_signal_emit(&text_input->events.set_focused_surface, text_input);
    wl_signal_emit(&text_input->events.enable, text_input);
}

static void text_input_disable(wl_client *client, wl_resource *resource, wl_resource *surface)
{
    assert(client == wl_resource_get_client(resource) && client == wl_resource_get_client(surface));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    assert(text_input->focused_surface = wlr_surface_from_resource(surface));
    text_input->active = false;
    text_input->focused_surface = nullptr;
    wl_signal_emit(&text_input->events.set_focused_surface, text_input);
    wl_signal_emit(&text_input->events.disable, text_input);
}

static void text_input_show_input_panel(wl_client *client, wl_resource *resource)
{
    assert(client == wl_resource_get_client(resource));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    wl_signal_emit(&text_input->events.show_input_panel, text_input);
}

static void text_input_hide_input_panel(wl_client *client, wl_resource *resource)
{
    assert(client == wl_resource_get_client(resource));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    wl_signal_emit(&text_input->events.hide_input_panel, text_input);
}

static void text_input_set_surrounding_text(wl_client *client, wl_resource *resource, const char *text, int32_t cursor, int32_t anchor)
{
    assert(client == wl_resource_get_client(resource));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    free(text_input->surrounding_text.text);
    text_input->surrounding_text.text = strdup(text);
    text_input->surrounding_text.cursor = cursor;
    text_input->surrounding_text.anchor = anchor;
    wl_signal_emit(&text_input->events.set_surrounding_text, text_input);
}

static void text_input_set_content_type(wl_client *client, wl_resource *resource, uint32_t hint, uint32_t purpose)
{
    assert(client == wl_resource_get_client(resource));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    text_input->content_type.hint = hint;
    text_input->content_type.purpose = purpose;
    wl_signal_emit(&text_input->events.set_content_type, text_input);
}

static void text_input_set_cursor_rectangle(wl_client *client, wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
    assert(client == wl_resource_get_client(resource));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    text_input->cursor_rectangle.x = x;
    text_input->cursor_rectangle.y = y;
    text_input->cursor_rectangle.width = width;
    text_input->cursor_rectangle.height = height;
    wl_signal_emit(&text_input->events.set_cursor_rectangle, text_input);
}

static void text_input_set_preferred_language(wl_client *client, wl_resource *resource, const char *language)
{
    assert(client == wl_resource_get_client(resource));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    free(text_input->preferred_language);
    text_input->preferred_language = strdup(language);
    wl_signal_emit(&text_input->events.set_preferred_language, text_input);
}

static void text_input_update_state(wl_client *client, wl_resource *resource, uint32_t serial, uint32_t reason)
{
    assert(client == wl_resource_get_client(resource));
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    text_input->current_serial = serial;
    text_input->reason = reason;
    wl_signal_emit(&text_input->events.update_state, text_input);
}

static void text_input_resource_destroy(wl_resource *resource)
{ }

static void text_input_destroy(wl_client *client, wl_resource *resource)
{
    ws_text_input_v2 *text_input = text_input_from_resource(resource);
    wl_signal_emit(&text_input->events.destroy, text_input);
    wl_resource_destroy(resource);
}

static ws_text_input_v2 *text_input_from_resource(wl_resource *resource)
{
    assert(wl_resource_instance_of(resource, &zwp_text_input_v2_interface, &text_input_v2_impl));
    return reinterpret_cast<ws_text_input_v2 *>(wl_resource_get_user_data(resource));
}

static ws_text_input_manager_v2 *text_input_manager_from_resource(wl_resource *resource);
static void text_input_manager_get_text_input(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *seat)
{
    int version = wl_resource_get_version(resource);
    struct wl_resource *text_input_resource = wl_resource_create(client, &zwp_text_input_v2_interface, version, id);
    if (!text_input_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(text_input_resource, &text_input_v2_impl, nullptr, text_input_resource_destroy);
    ws_text_input_v2 *text_input = new ws_text_input_v2;
    text_input->resource = text_input_resource;
    wlr_seat_client *seat_client = wlr_seat_client_from_resource(seat);
    text_input->seat = seat_client->seat;
    wl_resource_set_user_data(text_input_resource, text_input);
    text_input->surface_destroy.notify = text_input_handle_focused_surface_destroy;
    wl_signal_init(&text_input->events.destroy);
    wl_signal_init(&text_input->events.enable);
    wl_signal_init(&text_input->events.disable);
    wl_signal_init(&text_input->events.show_input_panel);
    wl_signal_init(&text_input->events.hide_input_panel);
    wl_signal_init(&text_input->events.update_state);
    wl_signal_init(&text_input->events.set_surrounding_text);
    wl_signal_init(&text_input->events.set_content_type);
    wl_signal_init(&text_input->events.set_cursor_rectangle);
    wl_signal_init(&text_input->events.set_preferred_language);
    wl_signal_init(&text_input->events.set_focused_surface);
    ws_text_input_manager_v2 *manager = text_input_manager_from_resource(resource);
    wl_list_insert(&manager->text_inputs, &text_input->link);
    wl_signal_emit_mutable(&manager->events.text_input, text_input);
}

static struct zwp_text_input_manager_v2_interface text_input_manager_v2_impl = {
    .get_text_input = text_input_manager_get_text_input
};

static ws_text_input_manager_v2 *text_input_manager_from_resource(wl_resource *resource)
{
    assert(wl_resource_instance_of(resource, &zwp_text_input_manager_v2_interface, &text_input_manager_v2_impl));
    return reinterpret_cast<ws_text_input_manager_v2 *>(wl_resource_get_user_data(resource));
}

static void text_input_manager_bind(struct wl_client *wl_client, void *data, uint32_t version, uint32_t id)
{
    ws_text_input_manager_v2 *manager = reinterpret_cast<ws_text_input_manager_v2 *>(data);
    wl_resource *resource = wl_resource_create(wl_client, &zwp_text_input_manager_v2_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(wl_client);
        return;
    }
    wl_resource_set_implementation(resource, &text_input_manager_v2_impl, manager, nullptr);
}

static void handle_display_destroy(struct wl_listener *listener, void *data)
{
    ws_text_input_manager_v2 *manager = wl_container_of(listener, manager, display_destroy);
    wl_signal_emit_mutable(&manager->events.destroy, manager);
    wl_list_remove(&manager->display_destroy.link);
    wl_global_destroy(manager->global);
    delete manager;
}

static ws_text_input_manager_v2 *text_input_manager_v2_create(struct wl_display *display)
{
    ws_text_input_manager_v2 *manager = new ws_text_input_manager_v2;

    manager->global = wl_global_create(display,
                                       &zwp_text_input_manager_v2_interface,
                                       1,
                                       manager,
                                       text_input_manager_bind);
    if (!manager->global) {
        delete manager;
        return nullptr;
    }

    wl_list_init(&manager->text_inputs);
    wl_signal_init(&manager->events.text_input);
    wl_signal_init(&manager->events.destroy);

    manager->display_destroy.notify = handle_display_destroy;
    wl_display_add_destroy_listener(display, &manager->display_destroy);

    return manager;
}

WTextInputManagerV2 *WTextInputManagerV2::create(QWLRoots::QWDisplay *display)
{
    ws_text_input_manager_v2 *manager = text_input_manager_v2_create(display->handle());
    return manager ? new WTextInputManagerV2(manager) : nullptr;
}

WTextInputManagerV2::WTextInputManagerV2(ws_text_input_manager_v2 *handle)
    : QObject(nullptr)
    , WObject(*new WTextInputManagerV2Private(handle, this))
{ }
WAYLIB_SERVER_END_NAMESPACE
