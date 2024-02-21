// Copyright (C) 2023 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquicktextinputv1_p.h"
#include <wtextinputv1.h>
#include <wseat.h>
#include <wsurface.h>

#include <qwdisplay.h>
#include <qwseat.h>
#include <qwcompositor.h>

#include <QRect>

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE
class WQuickTextInputV1Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickTextInputV1)
    explicit WQuickTextInputV1Private(WTextInputV1 *h, WQuickTextInputV1 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    { }

    WTextInputV1 *const handle;
};

class WQuickTextInputManagerV1Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickTextInputManagerV1)
    explicit WQuickTextInputManagerV1Private(WQuickTextInputManagerV1 *qq)
        : WObjectPrivate(qq)
        , handle(nullptr)
    { }

    WTextInputManagerV1 *handle;
    QList<WQuickTextInputV1 *> textInputs;
};

WQuickTextInputManagerV1::WQuickTextInputManagerV1(QObject *parent)
    : WQuickWaylandServerInterface(parent)
    , WObject(*new WQuickTextInputManagerV1Private(this))
{
    connect(this, &WQuickTextInputManagerV1::newTextInput, this, &WQuickTextInputManagerV1::textInputsChanged);
}

QList<WQuickTextInputV1 *> WQuickTextInputManagerV1::textInputs() const
{
    return d_func()->textInputs;
}

WSeat *WQuickTextInputV1::seat() const
{
    return handle()->seat();
}

QString WQuickTextInputV1::surroundingText() const
{
    return handle()->surroundingText();
}

uint WQuickTextInputV1::surroundingTextCursor() const
{
    return handle()->surroundingCursor();
}

uint WQuickTextInputV1::surroundingTextAnchor() const
{
    return handle()->surroundingAnchor();
}

WQuickTextInputV1::ContentHints WQuickTextInputV1::contentHint() const
{
    return ContentHints(handle()->contentHint());
}

WQuickTextInputV1::ContentPurpose WQuickTextInputV1::contentPurpose() const
{
    return static_cast<ContentPurpose>(handle()->contentPurpose());
}

QRect WQuickTextInputV1::cursorRectangle() const
{
    return handle()->cursorRectangle();
}

QString WQuickTextInputV1::preferredLanguage() const
{
    return handle()->preferredLanguage();
}

WSurface *WQuickTextInputV1::focusedSurface() const
{
    return handle()->focusedSurface();
}

wl_client *WQuickTextInputV1::waylandClient() const
{
    return handle()->waylandClient();
}

WTextInputV1 *WQuickTextInputV1::handle() const
{
    return d_func()->handle;
}

void WQuickTextInputV1::sendEnter(WSurface *surface)
{
    handle()->sendEnter(surface);
}

void WQuickTextInputV1::sendLeave()
{
    handle()->sendLeave();
}

void WQuickTextInputV1::sendModifiersMap(QStringList modifiersMap)
{
    handle()->sendModifiersMap(modifiersMap);
}

void WQuickTextInputV1::sendInputPanelState(uint state)
{
    handle()->sendInputPanelState(state);
}

void WQuickTextInputV1::sendPreeditString(QString text, QString commit)
{
    handle()->sendPreeditString(text, commit);
}

void WQuickTextInputV1::sendPreeditStyling(uint index, uint length, PreeditStyle style)
{
    handle()->sendPreeditStyling(index, length, style);
}

void WQuickTextInputV1::sendPreeditCursor(int index)
{
    handle()->sendPreeditCursor(index);
}

void WQuickTextInputV1::sendCommitString(QString text)
{
    handle()->sendCommitString(text);
}

void WQuickTextInputV1::sendCursorPosition(int index, int anchor)
{
    handle()->sendCursorPosition(index, anchor);
}

void WQuickTextInputV1::sendDeleteSurroundingText(int index, uint length)
{
    handle()->sendDeleteSurroundingString(index, length);
}

void WQuickTextInputV1::sendKeySym(uint time, uint sym, uint state, uint modifiers)
{
    handle()->sendKeysym(time, sym, state, modifiers);
}

void WQuickTextInputV1::sendLanguage(QString language)
{
    handle()->sendLanguage(language);
}

void WQuickTextInputV1::sendTextDirection(TextDirection textDirection)
{
    handle()->sendTextDirection(textDirection);
}

WQuickTextInputV1::WQuickTextInputV1(WTextInputV1 *handle, QObject *parent)
    : QObject(parent)
    , WObject(*new WQuickTextInputV1Private(handle, this))
{
    W_D(WQuickTextInputV1);
    connect(d->handle, &WTextInputV1::showInputPanel, this, &WQuickTextInputV1::showInputPanel);
    connect(d->handle, &WTextInputV1::hideInputPanel, this, &WQuickTextInputV1::hideInputPanel);
    connect(d->handle, &WTextInputV1::activate, this, &WQuickTextInputV1::activated);
    connect(d->handle, &WTextInputV1::deactivate, this, &WQuickTextInputV1::deactivated);
    connect(d->handle, &WTextInputV1::commitState, this, &WQuickTextInputV1::committed);
    connect(d->handle, &WTextInputV1::invokeAction, this, &WQuickTextInputV1::invokeAction);
    connect(d->handle, &WTextInputV1::reset, this, &WQuickTextInputV1::reset);
    connect(d->handle, &WTextInputV1::surroundingTextChanged, this, &WQuickTextInputV1::surroundingTextChanged);
    connect(d->handle, &WTextInputV1::contentTypeChanged, this, &WQuickTextInputV1::contentTypeChanged);
    connect(d->handle, &WTextInputV1::cursorRectangleChanged, this, &WQuickTextInputV1::cursorRectangleChanged);
    connect(d->handle, &WTextInputV1::preferredLanguageChanged, this, &WQuickTextInputV1::preferredLanguageChanged);
}

void WQuickTextInputManagerV1::create()
{
    W_D(WQuickTextInputManagerV1);
    WQuickWaylandServerInterface::create();
    d->handle = WTextInputManagerV1::create(server()->handle());
    Q_ASSERT(d->handle);
    connect(d->handle, &WTextInputManagerV1::newTextInput, this, [this, d](WTextInputV1 *ti) {
        WQuickTextInputV1 *textInputV1 = new WQuickTextInputV1(ti, this);
        d->textInputs.append(textInputV1);
        connect(ti, &WTextInputV1::beforeDestroy, this, [this, d, textInputV1](){
            d->textInputs.removeAll(textInputV1);
            Q_EMIT textInputsChanged();
            textInputV1->deleteLater();
        });
        Q_EMIT newTextInput(textInputV1);
    });
}

WTextInputV1Adaptor::WTextInputV1Adaptor(WQuickTextInputV1 *ti)
    : WTextInputAdaptor(ti)
    , m_ti(ti)
{
    // TODO: Handle text input v1's activate signal and seat changed signal
    connect(m_ti, &WQuickTextInputV1::activated, this, [this](){ Q_EMIT this->requestFocus(this); });
    connect(m_ti, &WQuickTextInputV1::deactivated, this, [this]() {
        // Disconnect all signals excluding requestFocus, as text input may be activated again in
        // another seat, leaving signal connected might interfere another seat.
        Q_EMIT this->requestLeave(this);
        disconnect(SIGNAL(committed(WTextInputAdaptor*)));
        disconnect(SIGNAL(enabled(WTextInputAdaptor*)));
        disconnect(SIGNAL(disabled(WTextInputAdaptor*)));
        disconnect(SIGNAL(requestLeave(WTextInputAdaptor*)));
    });
    connect(m_ti, &WQuickTextInputV1::committed, this, [this](){ Q_EMIT committed(this); });
    connect(m_ti->handle(), &WTextInputV1::beforeDestroy, this, [this]() { Q_EMIT beforeDestroy(this); });
}

WSeat *WTextInputV1Adaptor::seat() const
{
    return m_ti->seat();
}

WSurface *WTextInputV1Adaptor::focusedSurface() const
{
    return m_ti->focusedSurface();
}

QString WTextInputV1Adaptor::surroundingText() const
{
    return m_ti->surroundingText();
}

int WTextInputV1Adaptor::surroundingCursor() const
{
    return m_ti->surroundingTextCursor();
}

int WTextInputV1Adaptor::surroundingAnchor() const
{
    return m_ti->surroundingTextAnchor();
}

im::ContentHints WTextInputV1Adaptor::contentHints() const
{
    // Note:: Only convert trivial flag
    const QMap<WQuickTextInputV1::ContentHint, IM::ContentHint> hintsMap = {
        {WQuickTextInputV1::CH_HiddenText, IM::CH_HiddenText},
        {WQuickTextInputV1::CH_AutoCapitalization, IM::CH_AutoCapitalization},
        {WQuickTextInputV1::CH_AutoCompletion, IM::CH_Completion},
        {WQuickTextInputV1::CH_AutoCorrection, IM::CH_Spellcheck},
        {WQuickTextInputV1::CH_Lowercase, IM::CH_Lowercase},
        {WQuickTextInputV1::CH_Uppercase, IM::CH_Uppercase},
        {WQuickTextInputV1::CH_Latin, IM::CH_Latin},
        {WQuickTextInputV1::CH_Titlecase, IM::CH_Titlecase},
        {WQuickTextInputV1::CH_Multiline, IM::CH_Multiline},
        {WQuickTextInputV1::CH_SensitiveData, IM::CH_SensitiveData}
    };
    auto convertToHints = [&hintsMap](WQuickTextInputV1::ContentHints hints) -> IM::ContentHints {
        IM::ContentHints result;
        for (auto key : hintsMap.keys()) {
            result.setFlag(hintsMap.value(key), hints.testFlag(key));
        }
        return result;
    };
    return convertToHints(m_ti->contentHint());
}

im::ContentPurpose WTextInputV1Adaptor::contentPurpose() const
{
    const QMap<WQuickTextInputV1::ContentPurpose, IM::ContentPurpose> purposeMap = {
        {WQuickTextInputV1::CP_Alpha, IM::CP_Alpha},
        {WQuickTextInputV1::CP_Normal, IM::CP_Normal},
        {WQuickTextInputV1::CP_Date, IM::CP_Date},
        {WQuickTextInputV1::CP_Time, IM::CP_Time},
        {WQuickTextInputV1::CP_Terminal, IM::CP_Terminal},
        {WQuickTextInputV1::CP_Digits, IM::CP_Digits},
        {WQuickTextInputV1::CP_Email, IM::CP_Email},
        {WQuickTextInputV1::CP_Name, IM::CP_Name},
        {WQuickTextInputV1::CP_Number, IM::CP_Number},
        {WQuickTextInputV1::CP_Password, IM::CP_Password},
        {WQuickTextInputV1::CP_Phone, IM::CP_Phone},
        {WQuickTextInputV1::CP_Datetime, IM::CP_Datetime},
        {WQuickTextInputV1::CP_Url, IM::CP_Url}
    };
    return purposeMap[m_ti->contentPurpose()];
}

QRect WTextInputV1Adaptor::cursorRect() const
{
    return m_ti->cursorRectangle();
}

im::Features WTextInputV1Adaptor::features() const
{
    return IM::Features(IM::F_SurroundingText | IM::F_ContentType | IM::F_CursorRect);
}

void WTextInputV1Adaptor::sendEnter(WSurface *surface)
{
    // For text input v1, client activate first, we assume that after sendEnter to client,
    // this text input is enabled.
    m_ti->sendEnter(surface);
    Q_EMIT enabled(this);
}

void WTextInputV1Adaptor::sendLeave()
{
    m_ti->sendLeave();
    Q_EMIT disabled(this);
}

void WTextInputV1Adaptor::sendDone()
{ /* Not needed */ }

void WTextInputV1Adaptor::handleIMCommitted(WInputMethodAdaptor *im)
{
    if (im->deleteSurroundingBeforeLength() || im->deleteSurroundingAfterLength()) {
        m_ti->sendDeleteSurroundingText(0, surroundingText().length() - im->deleteSurroundingBeforeLength() - im->deleteSurroundingAfterLength());
    }
    m_ti->sendCommitString(im->commitString());
    if (!im->preeditString().isEmpty()) {
        m_ti->sendPreeditCursor(im->preeditCursorBegin());
        m_ti->sendPreeditStyling(0, im->preeditString().length(), WQuickTextInputV1::PS_Active);
        m_ti->sendPreeditString(im->preeditString(), im->commitString());
    }
}

void WTextInputV1Adaptor::handleKeyboardFocusChanged(WSurface *keyboardFocus)
{
}

wl_client *WTextInputV1Adaptor::waylandClient() const
{
    return m_ti->waylandClient();
}

WAYLIB_SERVER_END_NAMESPACE



