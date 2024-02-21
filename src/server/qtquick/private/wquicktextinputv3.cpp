// Copyright (C) 2023 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquicktextinputv3_p.h"
#include "wseat.h"
#include "wtoplevelsurface.h"
#include "wtools.h"

#include <qwcompositor.h>
#include <qwseat.h>
#include <qwtextinputv3.h>

#include <QLoggingCategory>
#include <QtQml>

extern "C" {
#include <wlr/types/wlr_text_input_v3.h>
#define static
#include <wlr/types/wlr_compositor.h>
#undef static
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(qLcInputMethod)
class WQuickTextInputManagerV3Private : public WObjectPrivate
{
public:
    WQuickTextInputManagerV3Private(WQuickTextInputManagerV3 *qq) :
        WObjectPrivate(qq),
        manager(nullptr)
    {}
    W_DECLARE_PUBLIC(WQuickTextInputManagerV3)

    QWTextInputManagerV3 *manager;
    QList<WQuickTextInputV3 *> textInputs;
};

WQuickTextInputManagerV3::WQuickTextInputManagerV3(QObject *parent) :
    WQuickWaylandServerInterface(parent),
    WObject(*new WQuickTextInputManagerV3Private(this))
{
    connect(this, &WQuickTextInputManagerV3::newTextInput, this, &WQuickTextInputManagerV3::textInputsChanged);
}

QList<WQuickTextInputV3 *> WQuickTextInputManagerV3::textInputs() const
{
    W_DC(WQuickTextInputManagerV3);
    return d->textInputs;
}

void WQuickTextInputManagerV3::create()
{
    W_D(WQuickTextInputManagerV3);
    WQuickWaylandServerInterface::create();
    d->manager = QWTextInputManagerV3::create(server()->handle());
    connect(d->manager, &QWTextInputManagerV3::textInput, this, [this, d] (QWTextInputV3 *textInput) {
        auto quickTextInput = new WQuickTextInputV3(textInput, this);
        connect(quickTextInput->handle(), &QWTextInputV3::beforeDestroy, this, [this, d, quickTextInput] () {
            d->textInputs.removeAll(quickTextInput);
            Q_EMIT textInputsChanged();
            quickTextInput->deleteLater();
        });
        d->textInputs.append(quickTextInput);
        Q_EMIT newTextInput(quickTextInput);
    });
}

class WQuickTextInputV3Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickTextInputV3)
    WQuickTextInputV3Private(QWTextInputV3 *h, WQuickTextInputV3 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    { }

    QWTextInputV3 *const handle;

    wl_client *waylandClient() const override
    {
        return wl_resource_get_client(handle->handle()->resource);
    }
};

WQuickTextInputV3::WQuickTextInputV3(QWTextInputV3 *h, QObject *parent) :
    QObject(parent),
    WObject(*new WQuickTextInputV3Private(h, this), nullptr)
{
    connect(handle(), &QWTextInputV3::enable, this, &WQuickTextInputV3::enabled);
    connect(handle(), &QWTextInputV3::disable, this, &WQuickTextInputV3::disabled);
    connect(handle(), &QWTextInputV3::commit, this, &WQuickTextInputV3::committed);
}

WSeat *WQuickTextInputV3::seat() const
{
    W_DC(WQuickTextInputV3);
    return WSeat::fromHandle(QWSeat::from(handle()->handle()->seat));
}

WSurface *WQuickTextInputV3::focusedSurface() const
{
    W_DC(WQuickTextInputV3);
    return WSurface::fromHandle(handle()->handle()->focused_surface);
}

QString WQuickTextInputV3::surroundingText() const
{
    return handle()->handle()->current.surrounding.text;
}

uint WQuickTextInputV3::surroundingCursor() const
{
    return handle()->handle()->current.surrounding.cursor;
}

uint WQuickTextInputV3::surroundingAnchor() const
{
    return handle()->handle()->current.surrounding.anchor;
}

WQuickTextInputV3::ChangeCause WQuickTextInputV3::textChangeCause() const
{
    return ChangeCause(handle()->handle()->current.text_change_cause);
}

WQuickTextInputV3::ContentHints WQuickTextInputV3::contentHints() const
{
    return ContentHints(handle()->handle()->current.content_type.hint);
}

WQuickTextInputV3::ContentPurpose WQuickTextInputV3::contentPurpose() const
{
    return ContentPurpose(handle()->handle()->current.content_type.purpose);
}

QRect WQuickTextInputV3::cursorRect() const
{
    return WTools::fromWLRBox(&handle()->handle()->current.cursor_rectangle);
}

WQuickTextInputV3::Features WQuickTextInputV3::features() const
{
    return Features(handle()->handle()->current.features);
}

wl_client *WQuickTextInputV3::waylandClient() const
{
    W_DC(WQuickTextInputV3);
    return d->waylandClient();
}

QWTextInputV3 *WQuickTextInputV3::handle() const
{
    return d_func()->handle;
}

void WQuickTextInputV3::sendEnter(WSurface *surface)
{
    // NOTE: As this function may be called in QML,
    // we may not check if one surface belongs to a client or not,
    // check the condition here.
    // TODO: If all protocol entity do the check itself, we might not
    // need to expose waylandClient to public.
    W_D(WQuickTextInputV3);
    wl_client *surfaceClient = wl_resource_get_client(surface->handle()->handle()->resource);
    if (d->waylandClient() == surfaceClient) {
        handle()->sendEnter(surface->handle());
        Q_EMIT focusedSurfaceChanged();
    } else {
        qmlWarning(this) << "Sending surface belongs to another client" << surfaceClient
                         << "to client" << d->waylandClient() << "is not allowed";
    }
}

void WQuickTextInputV3::sendLeave()
{
    if (focusedSurface()) {
        handle()->sendLeave();
        Q_EMIT focusedSurfaceChanged();
    }
}

void WQuickTextInputV3::sendPreeditString(const QString &text, qint32 cursor_begin, qint32 cursor_end)
{
    handle()->sendPreeditString(qPrintable(text), cursor_begin, cursor_end);
}

void WQuickTextInputV3::sendCommitString(const QString &text)
{
    handle()->sendCommitString(qPrintable(text));
}

void WQuickTextInputV3::sendDeleteSurroundingText(quint32 before_length, quint32 after_length)
{
    handle()->sendDeleteSurroundingText(before_length, after_length);
}

void WQuickTextInputV3::sendDone()
{
    handle()->sendDone();
}

WTextInputV3Adaptor::WTextInputV3Adaptor(WQuickTextInputV3 *tiv3)
    : WTextInputAdaptor(tiv3)
    , m_ti(tiv3)
{
    connect(m_ti->handle(), &QWTextInputV3::beforeDestroy, this, [this]() { Q_EMIT beforeDestroy(this); });
    connect(m_ti, &WQuickTextInputV3::enabled, this, [this]() { Q_EMIT enabled(this); });
    connect(m_ti, &WQuickTextInputV3::disabled, this, [this]() { Q_EMIT disabled(this); });
}

WSeat *WTextInputV3Adaptor::seat() const
{
    return m_ti->seat();
}

WSurface *WTextInputV3Adaptor::focusedSurface() const
{
    return m_ti->focusedSurface();
}

QString WTextInputV3Adaptor::surroundingText() const
{
    return m_ti->surroundingText();
}

int WTextInputV3Adaptor::surroundingCursor() const
{
    return m_ti->surroundingCursor();
}

int WTextInputV3Adaptor::surroundingAnchor() const
{
    return m_ti->surroundingAnchor();
}

IM::ChangeCause WTextInputV3Adaptor::textChangeCause() const
{
    return static_cast<IM::ChangeCause>(m_ti->textChangeCause());
}

IM::ContentHints WTextInputV3Adaptor::contentHints() const
{
    return static_cast<IM::ContentHints>(m_ti->contentHints().toInt());
}

IM::ContentPurpose WTextInputV3Adaptor::contentPurpose() const
{
    return static_cast<IM::ContentPurpose>(m_ti->contentPurpose());
}

QRect WTextInputV3Adaptor::cursorRect() const
{
    return m_ti->cursorRect();
}

IM::Features WTextInputV3Adaptor::features() const
{
    return static_cast<IM::Features>(m_ti->features().toInt());
}

wl_client *WTextInputV3Adaptor::waylandClient() const
{
    return m_ti->waylandClient();
}

void WTextInputV3Adaptor::sendEnter(WSurface *surface)
{
    m_ti->sendEnter(surface);
}

void WTextInputV3Adaptor::sendLeave()
{
    m_ti->sendLeave();
}

void WTextInputV3Adaptor::sendDone()
{
    m_ti->sendDone();
}

void WTextInputV3Adaptor::handleIMCommitted(WInputMethodAdaptor *im)
{
    if (!im->preeditString().isEmpty()) {
        m_ti->sendPreeditString(im->preeditString(), im->preeditCursorBegin(), im->preeditCursorEnd());
    }
    if (!im->commitString().isEmpty()) {
        m_ti->sendCommitString(im->commitString());
    }
    if (im->deleteSurroundingBeforeLength() || im->deleteSurroundingAfterLength()) {
        m_ti->sendDeleteSurroundingText(im->deleteSurroundingBeforeLength(), im->deleteSurroundingAfterLength());
    }
    m_ti->sendDone();
}

void WTextInputV3Adaptor::handleKeyboardFocusChanged(WSurface *keyboardFocus)
{
}
WAYLIB_SERVER_END_NAMESPACE
