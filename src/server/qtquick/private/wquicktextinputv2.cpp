// Copyright (C) 2024 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wquicktextinputv2_p.h"
#include <wsurfaceitem.h>

#include <wtextinputv2.h>

WAYLIB_SERVER_BEGIN_NAMESPACE
class WQuickTextInputManagerV2Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickTextInputManagerV2)
    explicit WQuickTextInputManagerV2Private(WQuickTextInputManagerV2 *qq)
        : WObjectPrivate(qq)
        , handle(nullptr)
    { }

    WTextInputManagerV2 *handle;
    QList<WQuickTextInputV2 *> textInputs;
};

class WQuickTextInputV2Private : public WObjectPrivate
{
public:
    W_DECLARE_PUBLIC(WQuickTextInputV2)
    explicit WQuickTextInputV2Private(WTextInputV2 *h, WQuickTextInputV2 *qq)
        : WObjectPrivate(qq)
        , handle(h)
    { }

    WTextInputV2 *const handle;
};

WQuickTextInputManagerV2::WQuickTextInputManagerV2(QObject *parent)
    : WQuickWaylandServerInterface(parent)
    , WObject(*new WQuickTextInputManagerV2Private(this))
{
    connect(this, &WQuickTextInputManagerV2::newTextInput, this, &WQuickTextInputManagerV2::textInputsChanged);
}

QList<WQuickTextInputV2 *> WQuickTextInputManagerV2::textInputs() const
{
    return d_func()->textInputs;
}

void WQuickTextInputManagerV2::create()
{
    W_D(WQuickTextInputManagerV2);
    WQuickWaylandServerInterface::create();
    d->handle = WTextInputManagerV2::create(server()->handle());
    Q_ASSERT(d->handle);
    connect(d->handle, &WTextInputManagerV2::newTextInputV2, this, [this, d](WTextInputV2 *handle){
        // Assert that only one quick text input v2 is created for each handle
        Q_ASSERT(std::find_if(d->textInputs.begin(), d->textInputs.end(), [handle](WQuickTextInputV2 *ele){ return ele->handle() == handle; }) == d->textInputs.end());
        auto textInputV2 = new WQuickTextInputV2(handle);
        connect(handle, &WTextInputV2::beforeDestroy, this, [this, d, textInputV2](){
            d->textInputs.removeOne(textInputV2);
            Q_EMIT textInputsChanged();
            textInputV2->deleteLater();
        });
        d->textInputs.append(textInputV2);
        Q_EMIT newTextInput(textInputV2);
    });
}

WSeat *WQuickTextInputV2::seat() const
{
    return handle()->seat();
}

WSurface *WQuickTextInputV2::focusedSurface() const
{
    return handle()->focusedSurface();
}

QString WQuickTextInputV2::surroundingText() const
{
    return handle()->surroundingText();
}

int WQuickTextInputV2::surroundingCursor() const
{
    return handle()->surroundingCursor();
}

int WQuickTextInputV2::surroundingAnchor() const
{
    return handle()->surroundingAnchor();
}

WQuickTextInputV2::ContentHints WQuickTextInputV2::contentHints() const
{
    return ContentHints(handle()->contentHint());
}

WQuickTextInputV2::ContentPurpose WQuickTextInputV2::contentPurpose() const
{
    return ContentPurpose(handle()->contentPurpose());
}

QRect WQuickTextInputV2::cursorRectangle() const
{
    return handle()->cursorRectangle();
}

QString WQuickTextInputV2::preferredLanguage() const
{
    return handle()->preferredLanguage();
}


WTextInputV2 *WQuickTextInputV2::handle() const
{
    return d_func()->handle;
}

void WQuickTextInputV2::sendEnter(WSurface *surface)
{
    handle()->sendEnter(surface);
}

void WQuickTextInputV2::sendLeave(WSurface *surface)
{
    handle()->sendLeave(surface);
}

void WQuickTextInputV2::sendInputPanelState(WQuickTextInputV2::InputPanelVisibility visibility, QRect geometry)
{
    handle()->sendInputPanelState(visibility, geometry);
}

void WQuickTextInputV2::sendPreeditString(const QString &text, const QString &commit)
{
    handle()->sendPreeditString(text, commit);
}

void WQuickTextInputV2::sendPreeditStyling(uint index, uint length, WQuickTextInputV2::PreeditStyle style)
{
    handle()->sendPreeditStyling(index, length, style);
}

void WQuickTextInputV2::sendPreeditCursor(int index)
{
    handle()->sendPreeditCursor(index);
}

void WQuickTextInputV2::sendCommitString(const QString &text)
{
    handle()->sendCommitString(text);
}

void WQuickTextInputV2::sendCursorPosition(int index, int anchor)
{
    handle()->sendCursorPosition(index, anchor);
}

void WQuickTextInputV2::sendDeleteSurroundingText(uint beforeLength, uint afterLength)
{
    handle()->sendDeleteSurroundingText(beforeLength, afterLength);
}

void WQuickTextInputV2::sendModifiersMap(QStringList map)
{
    handle()->sendModifiersMap(map);
}

void WQuickTextInputV2::sendKeysym(uint time, Qt::Key sym, uint state, uint modifiers)
{
    handle()->sendKeysym(time, sym, state, modifiers);
}

void WQuickTextInputV2::sendLanguage(const QString &language)
{
    handle()->sendLanguage(language);
}

void WQuickTextInputV2::sendConfigureSurroundingText(int beforeCursor, int afterCursor)
{
    handle()->sendConfigureSurroundingText(beforeCursor, afterCursor);
}

void WQuickTextInputV2::sendInputMethodChanged(uint flags)
{
    handle()->sendInputMethodChanged(flags);
}

WQuickTextInputV2::WQuickTextInputV2(WTextInputV2 *handle, QObject *parent)
    : QObject(parent)
    , WObject(*new WQuickTextInputV2Private(handle, this))
{
    connect(handle, &WTextInputV2::enable, this, &WQuickTextInputV2::enabled);
    connect(handle, &WTextInputV2::disable, this, &WQuickTextInputV2::disabled);
    connect(handle, &WTextInputV2::showInputPanel, this, &WQuickTextInputV2::showInputPanel);
    connect(handle, &WTextInputV2::hideInputPanel, this, &WQuickTextInputV2::hideInputPanel);
    connect(handle, &WTextInputV2::updateState, this, [this](uint reason) {
        Q_EMIT stateUpdated(UpdateState(reason));
    });
    connect(handle, &WTextInputV2::surroundingTextChanged, this, &WQuickTextInputV2::surroundingTextChanged);
    connect(handle, &WTextInputV2::contentTypeChanged, this, &WQuickTextInputV2::contentTypeChanged);
    connect(handle, &WTextInputV2::cursorRectangleChanged, this, &WQuickTextInputV2::cursorRectangleChanged);
    connect(handle, &WTextInputV2::preferredLanguageChanged, this, &WQuickTextInputV2::preferredLanguageChanged);
    connect(handle, &WTextInputV2::focusedSurfaceChanged, this, &WQuickTextInputV2::focusedSurfaceChanged);
}

WTextInputV2Adaptor::WTextInputV2Adaptor(WQuickTextInputV2 *ti)
    : WTextInputAdaptor(ti)
    , m_ti(ti)
{
    connect(m_ti->handle(), &WTextInputV2::beforeDestroy, this, [this]() {
        Q_EMIT beforeDestroy(this);
    });
    connect(m_ti, &WQuickTextInputV2::enabled, this, [this]() {
        Q_EMIT enabled(this);
    });
    connect(m_ti, &WQuickTextInputV2::disabled, this, [this]() {
        Q_EMIT disabled(this);
    });
    connect(m_ti, &WQuickTextInputV2::stateUpdated, this, [this]() {
        Q_EMIT committed(this);
    });
}

WSeat *WTextInputV2Adaptor::seat() const
{
    return m_ti->seat();
}

WSurface *WTextInputV2Adaptor::focusedSurface() const
{
    return m_ti->focusedSurface();
}

QString WTextInputV2Adaptor::surroundingText() const
{
    return m_ti->surroundingText();
}

int WTextInputV2Adaptor::surroundingCursor() const
{
    return m_ti->surroundingCursor();
}

int WTextInputV2Adaptor::surroundingAnchor() const
{
    return m_ti->surroundingAnchor();
}

IM::ChangeCause WTextInputV2Adaptor::textChangeCause() const
{
    return IM::CC_InputMethod;
}

IM::ContentHints WTextInputV2Adaptor::contentHints() const
{
    const QMap<WQuickTextInputV2::ContentHint, IM::ContentHint> hintsMap = {
        {WQuickTextInputV2::CH_HiddenText, IM::CH_HiddenText},
        {WQuickTextInputV2::CH_AutoCapitalization, IM::CH_AutoCapitalization},
        {WQuickTextInputV2::CH_AutoCompletion, IM::CH_Completion},
        {WQuickTextInputV2::CH_AutoCorrection, IM::CH_Spellcheck},
        {WQuickTextInputV2::CH_Lowercase, IM::CH_Lowercase},
        {WQuickTextInputV2::CH_Uppercase, IM::CH_Uppercase},
        {WQuickTextInputV2::CH_Latin, IM::CH_Latin},
        {WQuickTextInputV2::CH_Titlecase, IM::CH_Titlecase},
        {WQuickTextInputV2::CH_Multiline, IM::CH_Multiline},
        {WQuickTextInputV2::CH_SensitiveData, IM::CH_SensitiveData}
    };
    auto convertToHints = [&hintsMap](WQuickTextInputV2::ContentHints hints) -> IM::ContentHints {
        IM::ContentHints result;
        for (auto key : hintsMap.keys()) {
            result.setFlag(hintsMap.value(key), hints.testFlag(key));
        }
        return result;
    };
    return convertToHints(m_ti->contentHints());
}

IM::ContentPurpose WTextInputV2Adaptor::contentPurpose() const
{
    const QMap<WQuickTextInputV2::ContentPurpose, IM::ContentPurpose> purposeMap = {
        {WQuickTextInputV2::CP_Alpha, IM::CP_Alpha},
        {WQuickTextInputV2::CP_Normal, IM::CP_Normal},
        {WQuickTextInputV2::CP_Date, IM::CP_Date},
        {WQuickTextInputV2::CP_Time, IM::CP_Time},
        {WQuickTextInputV2::CP_Terminal, IM::CP_Terminal},
        {WQuickTextInputV2::CP_Digits, IM::CP_Digits},
        {WQuickTextInputV2::CP_Email, IM::CP_Email},
        {WQuickTextInputV2::CP_Name, IM::CP_Name},
        {WQuickTextInputV2::CP_Number, IM::CP_Number},
        {WQuickTextInputV2::CP_Password, IM::CP_Password},
        {WQuickTextInputV2::CP_Phone, IM::CP_Phone},
        {WQuickTextInputV2::CP_Datetime, IM::CP_Datetime},
        {WQuickTextInputV2::CP_Url, IM::CP_Url}
    };
    return purposeMap[m_ti->contentPurpose()];
}

QRect WTextInputV2Adaptor::cursorRect() const
{
    return m_ti->cursorRectangle();
}

IM::Features WTextInputV2Adaptor::features() const
{
    return IM::Features(IM::F_SurroundingText | IM::F_ContentType | IM::F_CursorRect);
}

wl_client *WTextInputV2Adaptor::waylandClient() const
{
    return m_ti->handle()->waylandClient();
}

void WTextInputV2Adaptor::sendEnter(WSurface *surface)
{
    m_ti->sendEnter(surface);
}

void WTextInputV2Adaptor::sendLeave()
{
    m_ti->sendLeave(m_ti->focusedSurface());
}

void WTextInputV2Adaptor::sendDone()
{ /* Not needed */ }

void WTextInputV2Adaptor::handleIMCommitted(WInputMethodAdaptor *im)
{
    // FIXME: Should we send cursor position to text input v2?
    // Actually we don't receive any request from input method v2 to change cursor position
    // If we send cursor position to text input v2, all we can do is to send previous cursor'
    // and anchor to text input
    // m_ti->sendCursorPosition(m_ti->surroundingCursor(), m_ti->surroundingAnchor());
    if (im->deleteSurroundingAfterLength() != 0 || im->deleteSurroundingBeforeLength() != 0) {
        m_ti->sendDeleteSurroundingText(im->deleteSurroundingBeforeLength(), im->deleteSurroundingAfterLength());
    }
    m_ti->sendCommitString(im->commitString());
    if (!im->preeditString().isEmpty()) {
        m_ti->sendPreeditStyling(0, im->preeditString().length(), WQuickTextInputV2::PS_Active);
        m_ti->sendPreeditCursor(im->preeditCursorBegin());
        m_ti->sendPreeditString(im->preeditString(), im->commitString());
    }
}

void WTextInputV2Adaptor::handleKeyboardFocusChanged(WSurface *keyboardFocus)
{
}

WAYLIB_SERVER_END_NAMESPACE
