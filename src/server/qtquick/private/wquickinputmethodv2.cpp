// Copyright (C) 2023 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquickinputmethodv2_p.h"
#include "winputmethodhelper.h"
#include "wseat.h"
#include "winputdevice.h"
#include "wsurface.h"
#include "wxdgsurface.h"

#include <qwcompositor.h>
#include <qwinputmethodv2.h>
#include <qwseat.h>
#include <qwkeyboard.h>
#include <qwvirtualkeyboardv1.h>

#include <QKeySequence>
#include <QLoggingCategory>
#include <QRect>

extern "C" {
#define delete delete_c
#include <wlr/types/wlr_input_method_v2.h>
#undef delete
#include <wlr/types/wlr_keyboard.h>
#define static
#include <wlr/types/wlr_compositor.h>
#undef static
#include <wlr/types/wlr_virtual_keyboard_v1.h>
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(qLcInputMethod)
class WQuickInputMethodManagerV2Private : public WObjectPrivate
{
public:
    explicit WQuickInputMethodManagerV2Private(WQuickInputMethodManagerV2 *qq)
        : WObjectPrivate(qq)
        , manager(nullptr)
    {}
    W_DECLARE_PUBLIC(WQuickInputMethodManagerV2)

    QWInputMethodManagerV2 *manager;
    QList<WQuickInputMethodV2 *> inputMethods;
};

WQuickInputMethodManagerV2::WQuickInputMethodManagerV2(QObject *parent) :
    WQuickWaylandServerInterface(parent),
    WObject(*new WQuickInputMethodManagerV2Private(this), nullptr)
{
    connect(this, &WQuickInputMethodManagerV2::newInputMethod, this, &WQuickInputMethodManagerV2::inputMethodsChanged);
}

QList<WQuickInputMethodV2 *> WQuickInputMethodManagerV2::inputMethods() const
{
    return d_func()->inputMethods;
}

void WQuickInputMethodManagerV2::create()
{
    W_D(WQuickInputMethodManagerV2);
    WQuickWaylandServerInterface::create();
    d->manager = QWInputMethodManagerV2::create(server()->handle());
    connect(d->manager, &QWInputMethodManagerV2::inputMethod, this, [this, d] (QWInputMethodV2 *im) {
        auto quickInputMethod = new WQuickInputMethodV2(im, this);
        connect(quickInputMethod->handle(), &QWInputMethodV2::beforeDestroy, this, [this, d, quickInputMethod]() {
            d->inputMethods.removeAll(quickInputMethod);
            Q_EMIT inputMethodsChanged();
            quickInputMethod->deleteLater();
        });
        d->inputMethods.append(quickInputMethod);
        Q_EMIT newInputMethod(quickInputMethod);
    });
}

class WQuickInputPopupSurfaceV2Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickInputPopupSurfaceV2)
    WQuickInputPopupSurfaceV2Private(QWInputPopupSurfaceV2 *h, WQuickInputPopupSurfaceV2 *qq)
        : WObjectPrivate(qq)
        , handle(h)
        , popupSurface(WSurface::fromHandle(h->surface()))
    {
        if (!popupSurface) {
            popupSurface = new WSurface(h->surface());
            QObject::connect(h->surface(), &QWSurface::beforeDestroy, popupSurface, &WSurface::deleteLater);
        }
    }

    QWInputPopupSurfaceV2 *const handle;
    WSurface *popupSurface;
};

class WQuickInputMethodKeyboardGrabV2Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickInputMethodKeyboardGrabV2)
    WQuickInputMethodKeyboardGrabV2Private(QWInputMethodKeyboardGrabV2 *h, WQuickInputMethodKeyboardGrabV2 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    { }

    wl_client *waylandClient() const override
    {
        Q_ASSERT(handle);
        return wl_resource_get_client(handle->handle()->resource);
    }

    QWInputMethodKeyboardGrabV2 *const handle;
};

class WQuickInputMethodV2Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickInputMethodV2)
    WQuickInputMethodV2Private(QWInputMethodV2 *h, WQuickInputMethodV2 *qq)
        : WObjectPrivate(qq)
        , handle(h)
        , activeKeyboardGrab(nullptr)
    { }

    QWInputMethodV2 *const handle;
    QList<WQuickInputPopupSurfaceV2 *> popupSurfaces;
    WQuickInputMethodKeyboardGrabV2 *activeKeyboardGrab;
};

WQuickInputMethodV2::WQuickInputMethodV2(QWInputMethodV2 *h, QObject *parent) :
    QObject(parent),
    WObject(*new WQuickInputMethodV2Private(h, this), nullptr)
{
    W_D(WQuickInputMethodV2);
    connect(handle(), &QWInputMethodV2::commit, this, &WQuickInputMethodV2::committed);
    connect(handle(), &QWInputMethodV2::newPopupSurface, this, [this, d](QWInputPopupSurfaceV2 *surface) {
        auto quickPopupSurface = new WQuickInputPopupSurfaceV2(surface, this);
        d->popupSurfaces.append(quickPopupSurface);
        connect(quickPopupSurface->handle(), &QWInputPopupSurfaceV2::beforeDestroy, this, [this, d, quickPopupSurface]() {
            d->popupSurfaces.removeAll(quickPopupSurface);
            Q_EMIT popupSurfacesChanged();
            quickPopupSurface->deleteLater();
        });
        Q_EMIT newPopupSurface(quickPopupSurface);
    });
    connect(handle(), &QWInputMethodV2::grabKeybord, this, [this, d](QWInputMethodKeyboardGrabV2 *keyboardGrab){
        auto quickKeyboardGrab = new WQuickInputMethodKeyboardGrabV2(keyboardGrab, this);
        d->activeKeyboardGrab = quickKeyboardGrab;
        connect(quickKeyboardGrab->handle(), &QWInputMethodKeyboardGrabV2::beforeDestroy, this, [this, d, quickKeyboardGrab]() {
            d->activeKeyboardGrab = nullptr;
            Q_EMIT activeKeyboardGrabChanged();
            quickKeyboardGrab->deleteLater();
        });
        Q_EMIT newKeyboardGrab(quickKeyboardGrab);
    });
    connect(this, &WQuickInputMethodV2::newPopupSurface, this, &WQuickInputMethodV2::popupSurfacesChanged);
}

QList<WQuickInputPopupSurfaceV2 *> WQuickInputMethodV2::popupSurfaces() const
{
    return d_func()->popupSurfaces;
}

WQuickInputMethodKeyboardGrabV2 *WQuickInputMethodV2::activeKeyboardGrab() const
{
    return d_func()->activeKeyboardGrab;
}

QWInputMethodV2 *WQuickInputMethodV2::handle() const
{
    return d_func()->handle;
}

WSeat *WQuickInputMethodV2::seat() const
{
    return WSeat::fromHandle(QWSeat::from(d_func()->handle->handle()->seat));
}

void WQuickInputMethodV2::sendContentType(quint32 hint, quint32 purpose)
{
    handle()->sendContentType(hint, purpose);
}

void WQuickInputMethodV2::sendActivate()
{
    handle()->sendActivate();
}

void WQuickInputMethodV2::sendDeactivate()
{
    handle()->sendDeactivate();
}

void WQuickInputMethodV2::sendDone()
{
    handle()->sendDone();
}

void WQuickInputMethodV2::sendSurroundingText(const QString &text, quint32 cursor, quint32 anchor)
{
    handle()->sendSurroundingText(qPrintable(text), cursor, anchor);
}

void WQuickInputMethodV2::sendTextChangeCause(quint32 cause)
{
    handle()->sendTextChangeCause(cause);
}

void WQuickInputMethodV2::sendUnavailable()
{
    handle()->sendUnavailable();
}

WQuickInputPopupSurfaceV2::WQuickInputPopupSurfaceV2(QWInputPopupSurfaceV2 *handle, QObject *parent) :
    QObject(parent),
    WObject(*new WQuickInputPopupSurfaceV2Private(handle, this), nullptr)
{ }

WSurface *WQuickInputPopupSurfaceV2::surface() const
{
    W_DC(WQuickInputPopupSurfaceV2);
    return d->popupSurface;
}

QWInputPopupSurfaceV2 *WQuickInputPopupSurfaceV2::handle() const
{
    return d_func()->handle;
}

void WQuickInputPopupSurfaceV2::sendTextInputRectangle(const QRect &sbox)
{
    handle()->send_text_input_rectangle(sbox);
}

WQuickInputMethodKeyboardGrabV2::WQuickInputMethodKeyboardGrabV2(QWInputMethodKeyboardGrabV2 *handle, QObject *parent):
    QObject(parent),
    WObject(*new WQuickInputMethodKeyboardGrabV2Private(handle, this), nullptr)
{ }

QWInputMethodKeyboardGrabV2 *WQuickInputMethodKeyboardGrabV2::handle() const
{
    return d_func()->handle;
}

WInputDevice *WQuickInputMethodKeyboardGrabV2::keyboard() const
{
    W_DC(WQuickInputMethodKeyboardGrabV2);
    if (handle()->handle()->keyboard) {
        return WInputDevice::fromHandle(QWInputDevice::from(&handle()->handle()->keyboard->base));
    } else {
        return nullptr;
    }
}

void WQuickInputMethodKeyboardGrabV2::sendKey(quint32 time, Qt::Key key, quint32 state)
{
    W_D(WQuickInputMethodKeyboardGrabV2);
    handle()->sendKey(time, key, state);
}

void WQuickInputMethodKeyboardGrabV2::sendModifiers(uint depressed, uint latched, uint locked, uint group)
{
    W_D(WQuickInputMethodKeyboardGrabV2);
    wlr_keyboard_modifiers wlrModifiers = {.depressed = depressed, .latched = latched, .locked = locked, .group = group};
    handle()->sendModifiers(&wlrModifiers);
}

void WQuickInputMethodKeyboardGrabV2::setKeyboard(WInputDevice *keyboard)
{
    W_D(WQuickInputMethodKeyboardGrabV2);
    if (keyboard) {
        auto qwKeyboard = qobject_cast<QWKeyboard *>(keyboard->handle());
        auto *virtualKeyboard = qobject_cast<QWVirtualKeyboardV1*>(qwKeyboard);
        // refer to: https://github.com/swaywm/sway/blob/master/sway/input/keyboard.c#L391
        if (virtualKeyboard &&
            wl_resource_get_client(virtualKeyboard->handle()->resource) == d->waylandClient()) {
            return;
        }
        handle()->setKeyboard(qwKeyboard);
    } else {
        handle()->setKeyboard(nullptr);
    }
}

class WInputPopupPrivate : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WInputPopup)
    explicit WInputPopupPrivate(WInputPopupSurfaceAdaptor *surface, WSurface *parentSurface, WInputPopup *qq)
        : WObjectPrivate(qq)
        , handle(surface)
        , parent(parentSurface)
    { }

    QSize size() const
    {
        return {handle->surface()->handle()->handle()->current.width, handle->surface()->handle()->handle()->current.height};
    }

    WInputPopupSurfaceAdaptor *const handle;
    WSurface *const parent;
};

WInputPopup::WInputPopup(WInputPopupSurfaceAdaptor *surface, WSurface *parentSurface, QObject *parent)
    : WToplevelSurface(parent)
    , WObject(*new WInputPopupPrivate(surface, parentSurface, this))
{ }

bool WInputPopup::doesNotAcceptFocus() const
{
    return true;
}

WSurface *WInputPopup::surface() const
{
    return handle()->surface();
}

WInputPopupSurfaceAdaptor *WInputPopup::handle() const
{
    return d_func()->handle;
}

bool WInputPopup::isActivated() const
{
    return true;
}

QRect WInputPopup::getContentGeometry() const
{
    return {0, 0, d_func()->size().width(), d_func()->size().height()};
}

WSurface *WInputPopup::parentSurface() const
{
    return d_func()->parent;
}

bool WInputPopup::checkNewSize(const QSize &size)
{
    return size == d_func()->size();
}

WInputPopupItem::WInputPopupItem(QQuickItem *parent)
    : WSurfaceItem(parent)
    , m_inputPopupSurface(nullptr)
{ }

WInputPopup *WInputPopupItem::surface() const
{
    return m_inputPopupSurface;
}

void WInputPopupItem::setSurface(WInputPopup *surface)
{
    if (m_inputPopupSurface == surface)
        return;
    m_inputPopupSurface = surface;
    WSurfaceItem::setSurface(surface ? surface->surface() : nullptr);
    Q_EMIT this->surfaceChanged();
}

QString WQuickInputMethodV2::commitString() const
{
    return d_func()->handle->handle()->current.commit_text;
}

uint WQuickInputMethodV2::deleteSurroundingBeforeLength() const
{
    return d_func()->handle->handle()->current.delete_c.before_length;
}

uint WQuickInputMethodV2::deleteSurroundingAfterLength() const
{
    return d_func()->handle->handle()->current.delete_c.after_length;
}

QString WQuickInputMethodV2::preeditString() const
{
    return d_func()->handle->handle()->current.preedit.text;
}

int WQuickInputMethodV2::preeditCursorBegin() const
{
    return d_func()->handle->handle()->current.preedit.cursor_begin;
}

int WQuickInputMethodV2::preeditCursorEnd() const
{
    return d_func()->handle->handle()->current.preedit.cursor_end;
}

WInputMethodV2Adaptor::WInputMethodV2Adaptor(WQuickInputMethodV2 *imv2)
    : WInputMethodAdaptor(imv2)
    , m_im(imv2)
{
    connect(m_im->handle(), &QWInputMethodV2::beforeDestroy, this, [this]() { Q_EMIT beforeDestroy(this); });
    connect(m_im, &WQuickInputMethodV2::newKeyboardGrab, this, [this](WQuickInputMethodKeyboardGrabV2 *kg) {
        Q_EMIT newKeyboardGrab(new WKeyboardGrabV2Adaptor(kg, m_im->seat()));
    });
    connect(m_im, &WQuickInputMethodV2::newPopupSurface, this, [this](WQuickInputPopupSurfaceV2 *ipsv2) {
        Q_EMIT newInputPopupSurface(new WInputPopupSurfaceV2Adaptor(ipsv2));
    });
    connect(m_im, &WQuickInputMethodV2::committed, this, [this]() { Q_EMIT committed(this); });
}

WSeat *WInputMethodV2Adaptor::seat() const
{
    return m_im->seat();
}

QString WInputMethodV2Adaptor::commitString() const
{
    return m_im->commitString();
}

uint WInputMethodV2Adaptor::deleteSurroundingBeforeLength() const
{
    return m_im->deleteSurroundingBeforeLength();
}

uint WInputMethodV2Adaptor::deleteSurroundingAfterLength() const
{
    return m_im->deleteSurroundingAfterLength();
}

QString WInputMethodV2Adaptor::preeditString() const
{
    return m_im->preeditString();
}

int WInputMethodV2Adaptor::preeditCursorBegin() const
{
    return m_im->preeditCursorBegin();
}

int WInputMethodV2Adaptor::preeditCursorEnd() const
{
    return m_im->preeditCursorEnd();
}

void WInputMethodV2Adaptor::activate()
{
    m_im->sendActivate();
}

void WInputMethodV2Adaptor::deactivate()
{
    m_im->sendDeactivate();
}

void WInputMethodV2Adaptor::sendDone()
{
    m_im->sendDone();
}

void WInputMethodV2Adaptor::sendUnavailable()
{
    m_im->sendUnavailable();
}

void WInputMethodV2Adaptor::sendContentType(im::ContentHints hints, im::ContentPurpose purpose)
{
    m_im->sendContentType(hints.toInt(), purpose);
}

void WInputMethodV2Adaptor::sendSurroundingText(const QString &text, uint cursor, uint anchor)
{
    m_im->sendSurroundingText(text, cursor, anchor);
}

void WInputMethodV2Adaptor::sendTextChangeCause(im::ChangeCause cause)
{
    m_im->sendTextChangeCause(cause);
}

void WInputMethodV2Adaptor::handleTICommitted(WTextInputAdaptor *ti)
{
    Q_ASSERT(ti);
    IM::Features features = ti->features();
    if (features.testFlag(IM::F_SurroundingText)) {
        sendSurroundingText(ti->surroundingText(), ti->surroundingCursor(), ti->surroundingAnchor());
    }
    sendTextChangeCause(ti->textChangeCause());
    if (features.testFlag(IM::F_ContentType)) {
        sendContentType(ti->contentHints(), ti->contentPurpose());
    }
    sendDone();
}

WKeyboardGrabV2Adaptor::WKeyboardGrabV2Adaptor(WQuickInputMethodKeyboardGrabV2 *kgv2, WSeat *seat)
    : WKeyboardGrabAdaptor(kgv2)
    , m_kg(kgv2)
    , m_seat(seat)
{
    connect(m_kg->handle(), &QWInputMethodKeyboardGrabV2::beforeDestroy, this, [this]() { Q_EMIT beforeDestroy(this); });
}

WInputDevice *WKeyboardGrabV2Adaptor::keyboard() const
{
    return m_kg->keyboard();
}

WInputPopupSurfaceV2Adaptor::WInputPopupSurfaceV2Adaptor(WQuickInputPopupSurfaceV2 *ipsv2)
    : WInputPopupSurfaceAdaptor(ipsv2)
    , m_ips(ipsv2)
{
    connect(m_ips->handle(), &QWInputPopupSurfaceV2::beforeDestroy, this, [this]() {
        Q_EMIT beforeDestroy(this);
    });
}

WSurface *WInputPopupSurfaceV2Adaptor::surface() const
{
    return m_ips->surface();
}

void WInputPopupSurfaceV2Adaptor::sendTextInputRectangle(const QRect &sbox)
{
    m_ips->sendTextInputRectangle(sbox);
}

void WKeyboardGrabV2Adaptor::sendKey(uint time, Qt::Key key, uint state)
{
    m_kg->sendKey(time, key, state);
}

void WKeyboardGrabV2Adaptor::sendModifiers(uint depressed, uint latched, uint locked, uint group)
{
    m_kg->sendModifiers(depressed, latched, locked, group);
}

void WKeyboardGrabV2Adaptor::setKeyboard(WInputDevice *keyboard)
{
    m_kg->setKeyboard(keyboard);
}

void WKeyboardGrabV2Adaptor::endGrab()
{
    if (m_kg->keyboard()) {
        m_seat->handle()->keyboardSendModifiers(&m_kg->handle()->handle()->keyboard->modifiers);
    }
    m_seat->handle()->keyboardEndGrab();
}

void WKeyboardGrabV2Adaptor::startGrab(wlr_seat_keyboard_grab *grab)
{
    m_seat->handle()->keyboardStartGrab(grab);
}
WAYLIB_SERVER_END_NAMESPACE
