// Copyright (C) 2023 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "winputmethodhelper_p.h"
#include <wquickinputmethodv2_p.h>
#include <wquicktextinputv1_p.h>
#include <wquicktextinputv2_p.h>
#include <wquicktextinputv3_p.h>
#include <wquickvirtualkeyboardv1_p.h>
#include <wquickseat_p.h>
#include <wseat.h>
#include <wsurface.h>
#include <wxdgsurface.h>

#include <qwcompositor.h>
#include <qwtextinputv3.h>
#include <qwvirtualkeyboardv1.h>
#include <qwseat.h>

#include <QLoggingCategory>
#include <QQmlInfo>

extern "C" {
#define static
#include <wlr/types/wlr_compositor.h>
#undef static
#include <wlr/types/wlr_text_input_v3.h>
#define delete delete_c
#include <wlr/types/wlr_input_method_v2.h>
#undef delete
#include <wlr/types/wlr_seat.h>
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE
Q_LOGGING_CATEGORY(qLcInputMethod, "waylib.server.im", QtInfoMsg)

struct GrabHandlerArg {
    const WInputMethodHelper *const helper;
    WKeyboardGrabAdaptor * grab;
};

static inline void handleKey(struct wlr_seat_keyboard_grab *grab, uint32_t time_msec, uint32_t key, uint32_t state)
{
    auto arg = reinterpret_cast<GrabHandlerArg*>(grab->data);
    for (auto vk: arg->helper->virtualKeyboardManagerV1()->virtualKeyboards()) {
        if (wlr_keyboard_from_input_device(vk->keyboard()->handle()->handle()) == grab->seat->keyboard_state.keyboard) {
            grab->seat->keyboard_state.default_grab->interface->key(grab, time_msec, key, state);
            return;
        }
    }
    arg->grab->sendKey(time_msec, Qt::Key(key), state);
}

static inline void handleModifiers(struct wlr_seat_keyboard_grab *grab, const struct wlr_keyboard_modifiers *modifiers)
{
    auto arg = reinterpret_cast<GrabHandlerArg*>(grab->data);
    for (auto vk: arg->helper->virtualKeyboardManagerV1()->virtualKeyboards()) {
        if (wlr_keyboard_from_input_device(vk->keyboard()->handle()->handle()) == grab->seat->keyboard_state.keyboard) {
            grab->seat->keyboard_state.default_grab->interface->modifiers(grab, modifiers);
            return;
        }
    }
    arg->grab->sendModifiers(modifiers->depressed, modifiers->latched, modifiers->locked, modifiers->group);
}

class WInputMethodHelperPrivate : public WObjectPrivate
{
    W_DECLARE_PUBLIC(WInputMethodHelper)
public:
    explicit WInputMethodHelperPrivate(WInputMethodHelper *qq)
        : WObjectPrivate(qq)
        , handlerArg({.helper = qq})
    {}

    WQuickSeat *seat;
    WQuickInputMethodManagerV2 *inputMethodManagerV2;
    WQuickTextInputManagerV1 *textInputManagerV1;
    WQuickTextInputManagerV2 *textInputManagerV2;
    WQuickTextInputManagerV3 *textInputManagerV3;
    WQuickVirtualKeyboardManagerV1 *virtualKeyboardManagerV1;

    QList<WInputPopup *> popupSurfaces;
    QRect cursorRect;
    QQuickItem *activeFocusItem;

    wlr_seat_keyboard_grab keyboardGrab;
    wlr_keyboard_grab_interface grabInterface;
    GrabHandlerArg handlerArg;

    WTextInputAdaptor *activeTextInput;
    WTextInputAdaptor *lastTextInput;
    WInputMethodAdaptor *activeInputMethod;
    QList<WTextInputAdaptor *> textInputs;
    QList<WVirtualKeyboardAdaptor *> virtualKeyboards;
};

WInputMethodHelper::WInputMethodHelper(QObject *parent)
    : QObject(parent)
    , WObject(*new WInputMethodHelperPrivate(this))
{
    W_D(WInputMethodHelper);
    // Every time active input method or text input changed, reconnect committed signal
    connect(this, &WInputMethodHelper::activeInputMethodChanged, this, &WInputMethodHelper::connectActiveIMAndTI);
    connect(this, &WInputMethodHelper::activeTextInputChanged, this, &WInputMethodHelper::connectActiveIMAndTI);
    connect(this, &WInputMethodHelper::activeTextInputChanged, this, [this]() {
        if (auto ti = focusedTextInput()) {
            connect(ti, &WTextInputAdaptor::committed, this, &WInputMethodHelper::handleActiveTICommitted, Qt::UniqueConnection);
        } else {
            disconnect(ti, &WTextInputAdaptor::committed, this, &WInputMethodHelper::handleActiveTICommitted);
        }
    });
}

WQuickTextInputManagerV1 *WInputMethodHelper::textInputManagerV1() const
{
    W_DC(WInputMethodHelper);
    return d->textInputManagerV1;
}

void WInputMethodHelper::setTextInputManagerV1(WQuickTextInputManagerV1 *textInputManagerV1)
{
    W_D(WInputMethodHelper);
    if (d->textInputManagerV1 == textInputManagerV1)
        return;
    if (d->textInputManagerV1) {
        d->textInputManagerV1->disconnect(this);
    }
    d->textInputManagerV1 = textInputManagerV1;
    if (textInputManagerV1) {
        connect(d->textInputManagerV1, &WQuickTextInputManagerV1::newTextInput, this, &WInputMethodHelper::handleNewTI);
    }
    Q_EMIT textInputManagerV1Changed();
}

WQuickTextInputManagerV2 *WInputMethodHelper::textInputManagerV2() const
{
    W_DC(WInputMethodHelper);
    return d->textInputManagerV2;
}

void WInputMethodHelper::setTextInputManagerV2(WQuickTextInputManagerV2 *textInputManagerV2)
{
    W_D(WInputMethodHelper);
    if (d->textInputManagerV2 == textInputManagerV2)
        return;
    if (d->textInputManagerV2) {
        d->textInputManagerV2->disconnect(this);
    }
    d->textInputManagerV2 = textInputManagerV2;
    if (textInputManagerV2) {
        connect(d->textInputManagerV2, &WQuickTextInputManagerV2::newTextInput, this, &WInputMethodHelper::handleNewTI);
    }
    Q_EMIT textInputManagerV2Changed();
}

WQuickTextInputManagerV3 *WInputMethodHelper::textInputManagerV3() const
{
    W_DC(WInputMethodHelper);
    return d->textInputManagerV3;
}

void WInputMethodHelper::setTextInputManagerV3(WQuickTextInputManagerV3 *textInputManagerV3)
{
    W_D(WInputMethodHelper);
    if (d->textInputManagerV3 == textInputManagerV3)
        return;
    if (d->textInputManagerV3) {
        d->textInputManagerV3->disconnect(this);
    }
    d->textInputManagerV3 = textInputManagerV3;
    if (textInputManagerV3) {
        connect(d->textInputManagerV3, &WQuickTextInputManagerV3::newTextInput, this, &WInputMethodHelper::handleNewTI);
    }
    Q_EMIT textInputManagerV3Changed();
}

WQuickInputMethodManagerV2 *WInputMethodHelper::inputMethodManagerV2() const
{
    W_DC(WInputMethodHelper);
    return d->inputMethodManagerV2;
}

void WInputMethodHelper::setInputMethodManagerV2(WQuickInputMethodManagerV2 *inputMethodManagerV2)
{
    W_D(WInputMethodHelper);
    if (d->inputMethodManagerV2 == inputMethodManagerV2)
        return;
    if (d->inputMethodManagerV2) {
        d->inputMethodManagerV2->disconnect(this);
    }
    d->inputMethodManagerV2 = inputMethodManagerV2;
    if (inputMethodManagerV2) {
        connect(d->inputMethodManagerV2, &WQuickInputMethodManagerV2::newInputMethod, this, &WInputMethodHelper::handleNewIMV2);
    }
    Q_EMIT inputMethodManagerV2Changed();
}

WQuickVirtualKeyboardManagerV1 *WInputMethodHelper::virtualKeyboardManagerV1() const
{
    W_DC(WInputMethodHelper);
    return d->virtualKeyboardManagerV1;
}

void WInputMethodHelper::setVirtualKeyboardManagerV1(WQuickVirtualKeyboardManagerV1 *virtualKeyboardManagerV1)
{
    W_D(WInputMethodHelper);
    if (d->virtualKeyboardManagerV1 == virtualKeyboardManagerV1)
        return;
    if (d->virtualKeyboardManagerV1) {
        d->virtualKeyboardManagerV1->disconnect(this);
    }
    d->virtualKeyboardManagerV1 = virtualKeyboardManagerV1;
    if (virtualKeyboardManagerV1) {
        connect(d->virtualKeyboardManagerV1, &WQuickVirtualKeyboardManagerV1::newVirtualKeyboard, this, &WInputMethodHelper::handleNewVKV1);
    }
    Q_EMIT virtualKeyboardManagerV1Changed();
}

// WQuickInputMethodV2 *WInputMethodHelper::inputMethodV2() const
// {
//     W_DC(WInputMethodHelper);
//     return d->inputMethodV2;
// }

// void WInputMethodHelper::setInputMethodV2(WQuickInputMethodV2 *newInputMethodV2)
// {
//     W_D(WInputMethodHelper);
//     if (d->inputMethodV2 != newInputMethodV2) {
//         d->inputMethodV2 = newInputMethodV2;
//         Q_EMIT this->inputMethodV2Changed(newInputMethodV2);
//     }
// }

QRect WInputMethodHelper::cursorRect() const
{
    W_DC(WInputMethodHelper);
    return d->cursorRect;
}

void WInputMethodHelper::setCursorRect(const QRect &rect)
{
    W_D(WInputMethodHelper);
    if (d->cursorRect == rect)
        return;
    d->cursorRect = rect;
    Q_EMIT cursorRectChanged();
}

QQuickItem *WInputMethodHelper::activeFocusItem() const
{
    W_DC(WInputMethodHelper);
    return d->activeFocusItem;
}

void WInputMethodHelper::setActiveFocusItem(QQuickItem *item)
{
    W_D(WInputMethodHelper);
    if (d->activeFocusItem == item)
        return;
    d->activeFocusItem = item;
    Q_EMIT this->activeFocusItemChanged();
}

WTextInputAdaptor *WInputMethodHelper::focusedTextInput() const
{
    W_DC(WInputMethodHelper);
    return d->activeTextInput;
}

void WInputMethodHelper::setFocusedTextInput(WTextInputAdaptor *ti)
{
    W_D(WInputMethodHelper);
    if (d->activeTextInput == ti)
        return;
    disconnectActiveIMAndTI();
    d->activeTextInput = ti;
    Q_EMIT activeTextInputChanged();
}

WInputMethodAdaptor *WInputMethodHelper::inputMethod() const
{
    W_DC(WInputMethodHelper);
    return d->activeInputMethod;
}

WTextInputAdaptor *WInputMethodHelper::lastFocusedTextInput() const
{
    return d_func()->lastTextInput;
}

void WInputMethodHelper::setInputMethod(WInputMethodAdaptor *im)
{
    W_D(WInputMethodHelper);
    if (d->activeInputMethod == im)
        return;
    disconnectActiveIMAndTI();
    d->activeInputMethod = im;
    Q_EMIT activeInputMethodChanged();
}

void WInputMethodHelper::connectActiveIMAndTI()
{
    // When active text input or active input method changed, we should reconnect signals
    auto im = inputMethod();
    auto ti = focusedTextInput();
    if (!im || !ti)
        return;
    connect(ti, &WTextInputAdaptor::committed, im, &WInputMethodAdaptor::handleTICommitted, Qt::UniqueConnection);
    connect(im, &WInputMethodAdaptor::committed, ti, &WTextInputAdaptor::handleIMCommitted, Qt::UniqueConnection);
}

void WInputMethodHelper::disconnectActiveIMAndTI()
{
    auto im = inputMethod();
    auto ti = focusedTextInput();
    if (!im || !ti)
        return;
    disconnect(ti, &WTextInputAdaptor::committed, im, &WInputMethodAdaptor::handleTICommitted);
    disconnect(im, &WInputMethodAdaptor::committed, ti, &WTextInputAdaptor::handleIMCommitted);
}

void WInputMethodHelper::handleNewTI(QObject *ti)
{
    if (auto ti1 = qobject_cast<WQuickTextInputV1*>(ti)) {
        tryAddTextInput(new WTextInputV1Adaptor(ti1));
    } else if (auto ti2 = qobject_cast<WQuickTextInputV2 *>(ti)) {
        tryAddTextInput(new WTextInputV2Adaptor(ti2));
    } else if (auto ti3 = qobject_cast<WQuickTextInputV3 *>(ti)) {
        tryAddTextInput(new WTextInputV3Adaptor(ti3));
    } else {
        Q_UNREACHABLE();
    }
}

void WInputMethodHelper::handleNewIMV2(WQuickInputMethodV2 *imv2)
{
    auto adaptor = new WInputMethodV2Adaptor(imv2);
    if (wseat()->name() != adaptor->seat()->name())
        return;
    if (inputMethod()) {
        qCWarning(qLcInputMethod) << "Ignore second creation of input on the same seat.";
        adaptor->sendUnavailable();
        return;
    }
    setInputMethod(adaptor);
    // Once input method is online, try to resend enter to textInput
    resendKeyboardFocus();
    // For text input v1, when after sendEnter, enabled signal will be emitted
    connect(adaptor, &WInputMethodAdaptor::beforeDestroy, this, [this, adaptor]{
        if (inputMethod() == adaptor) {
            setInputMethod(nullptr);
        }
        notifyLeave();
    });
    // connect(adaptor, &WInputMethodAdaptor::committed, this, &WInputMethodHelper::onInputMethodV2Committed);
    connect(adaptor, &WInputMethodAdaptor::newKeyboardGrab, this, &WInputMethodHelper::onNewKeyboardGrab);
    connect(adaptor, &WInputMethodAdaptor::newInputPopupSurface, this, &WInputMethodHelper::onInputPopupSurface);
}

void WInputMethodHelper::handleNewVKV1(WQuickVirtualKeyboardV1 *vkv1)
{
    W_D(WInputMethodHelper);
    auto adaptor = new WVirtualKeyboardV1Adaptor(vkv1);
    d->virtualKeyboards.append(adaptor);
    wseat()->attachInputDevice(adaptor->keyboard());
    connect(adaptor, &WVirtualKeyboardAdaptor::beforeDestroy, this, [this, adaptor] () {
        wseat()->detachInputDevice(adaptor->keyboard());
    });
}

// void WInputMethodHelper::onNewTextInputV1(WQuickTextInputV1 *ti)
// {
//     // Actually when textInputV1 is not activated, just created, we don't need to do anything, cause it is not associated with seat
//     // Unlike text input v3, we should listen on each text input v1 object cause it might be activated on this seat at any time
//     connect(ti, &WQuickTextInputV1::seatChanged, this, [this, ti](){
//         auto st = ti->seat();
//         if (st && this->wseat()->name() == st->name()) {
//             if (activeTextInputV1()) {
//                 // Let previous active text input leave
//                 activeTextInputV1()->sendLeave();
//             }
//             if (activeTextInputV3()) {
//                 activeTextInputV3()->sendLeave();
//             }
//             ti->sendEnter(ti->focusedSurface());
//             setActiveTextInputV1(ti);
//             if (inputMethodV2()) {
//                 inputMethodV2()->sendActivate();
//                 inputMethodV2()->sendDone();
//             }
//         } else {
//             onTextInputV1Deactivated(ti);
//         }
//     });
//     connect(ti, &WQuickTextInputV1::commit, this, [this, ti] (){
//         auto im = inputMethodV2();
//         if (!im)
//             return;
//         im->sendContentType(ti->contentHint(), ti->contentPurpose());
//         im->sendSurroundingText(ti->surroundingText(), ti->surroundingTextCursor(), ti->surroundingTextAnchor());
//         setCursorRect(ti->cursorRectangle());
//         updateAllPopupSurfaces(ti->cursorRectangle());
//         im->sendDone();
//     });
//     connect(ti, &WQuickTextInputV1::beforeDestroy, this, [this, ti] (){
//         onTextInputV1Deactivated(ti);
//     });
// }

// void WInputMethodHelper::onTextInputV1Deactivated(WQuickTextInputV1 *textInputV1)
// {
//     if (activeTextInputV1() && activeTextInputV1() == textInputV1) {
//         activeTextInputV1()->sendLeave();
//         setActiveTextInputV1(nullptr);
//         if (inputMethodV2()) {
//             inputMethodV2()->sendDeactivate();
//             inputMethodV2()->sendDone();
//         }
//     }
// }

// void WInputMethodHelper::onNewTextInputV3(WQuickTextInputV3 *newTextInput)
// {
//     W_D(WInputMethodHelper);
//     // Only cares about current seat's text input
//     if (wseat()->name() == newTextInput->seat()->name()) {
//         connect(newTextInput, &WQuickTextInputV3::enable, this, [this, newTextInput]() { Q_EMIT this->onTextInputV3Enabled(newTextInput); });
//         connect(newTextInput, &WQuickTextInputV3::disable, this, [this, newTextInput]() { Q_EMIT this->onTextInputV3Disabled(newTextInput); });
//         connect(newTextInput, &WQuickTextInputV3::commit, this, [this, newTextInput]() { Q_EMIT this->onTextInputV3Committed(newTextInput); });
//         connect(newTextInput->handle(), &QWTextInputV3::beforeDestroy, this, [this, newTextInput]() { Q_EMIT this->onTextInputV3Destroyed(newTextInput); });
//     }
// }


// void WInputMethodHelper::onTextInputV3Enabled(WQuickTextInputV3 *enabledTextInputV3)
// {
//     if (activeTextInputV3()) {
//         // Assume that previous active text input has been disabled
//         onTextInputV3Disabled(activeTextInputV3());
//     }
//     if (activeTextInputV1()) {
//         onTextInputV1Deactivated(activeTextInputV1());
//     }
//     setActiveTextInputV3(enabledTextInputV3);
//     auto im = inputMethodV2();
//     if (im) {
//         im->sendActivate();
//         sendInputMethodV2State(enabledTextInputV3);
//     }
// }

// void WInputMethodHelper::onTextInputV3Disabled(WQuickTextInputV3 *disabledTextInputV3)
// {
//     if (activeTextInputV3() == disabledTextInputV3) {
//         // Should we consider the case when the same text input is disabled and then enabled at the same time.
//         setActiveTextInputV3(nullptr);
//         auto im = inputMethodV2();
//         if (im) {
//             im->sendDeactivate();
//             sendInputMethodV2State(disabledTextInputV3);
//         }
//     }
// }

// void WInputMethodHelper::onTextInputV3Committed(WQuickTextInputV3 *textInputV3)
// {
//     auto im = inputMethodV2();
//     if (!im)
//         return;
//     [[maybe_unused]] const WTextInputV3State *const current = textInputV3->state();
//     sendInputMethodV2State(textInputV3);
// }

// void WInputMethodHelper::onTextInputV3Destroyed(WQuickTextInputV3 *destroyedTextInputV3)
// {
//     onTextInputV3Disabled(destroyedTextInputV3);
// }

// void WInputMethodHelper::onInputMethodV2Committed()
// {
//     W_D(WInputMethodHelper);
//     auto inputMethod = qobject_cast<WQuickInputMethodV2 *>(sender());
//     Q_ASSERT(inputMethod);
//     const WInputMethodV2State *const current = inputMethod->state();
//     auto textInputV3 = activeTextInputV3();
//     if (textInputV3) {
//         if (!current->preeditStringText().isEmpty()) {
//             textInputV3->sendPreeditString(current->preeditStringText(), current->preeditStringCursorBegin(), current->preeditStringCursorEnd());
//         }
//         if (!current->commitString().isEmpty()) {
//             textInputV3->sendCommitString(current->commitString());
//         }
//         if (current->deleteSurroundingBeforeLength() || current->deleteSurroundingAfterLength()) {
//             textInputV3->sendDeleteSurroundingText(current->deleteSurroundingBeforeLength(), current->deleteSurroundingAfterLength());
//         }
//         textInputV3->sendDone();
//     }
//     auto textInputV1 = activeTextInputV1();
//     if (textInputV1) {
//         if (current->deleteSurroundingBeforeLength() || current->deleteSurroundingAfterLength()) {
//             textInputV1->sendDeleteSurroundingText(0, textInputV1->surroundingText().length() - current->deleteSurroundingBeforeLength() - current->deleteSurroundingAfterLength());
//         }
//         textInputV1->sendCommitString(current->commitString());
//         if (!current->preeditStringText().isEmpty()) {
//             textInputV1->sendPreeditStyling(0, current->preeditStringCursorEnd() - current->preeditStringCursorBegin(), WQuickTextInputV1::PS_Active);
//             textInputV1->sendPreeditCursor(current->preeditStringCursorEnd() - current->preeditStringCursorBegin());
//             textInputV1->sendPreeditString(current->preeditStringText(), current->commitString());
//         }
//     }
// }

void WInputMethodHelper::resendKeyboardFocus()
{
    notifyLeave();
    sendKeyboardFocus();
}

QString WInputMethodHelper::seatName() const
{
    return wseat()->name();
}

void WInputMethodHelper::tryAddTextInput(WTextInputAdaptor *ti)
{
    W_D(WInputMethodHelper);
    if (d->textInputs.contains(ti))
        return;
    d->textInputs.append(ti);
    // Whether this text input belongs to current seat or not, we should connect
    // its requestFocus signal for it might request focus from another seat to activate
    // itself here. For example, text input v1.
    connect(ti, &WTextInputAdaptor::requestFocus, this, &WInputMethodHelper::handleTIRequestFocus);
    if (ti->seat() && seatName() == ti->seat()->name()) {
        connectToTI(ti);
    }
}

void WInputMethodHelper::handleTIEnabled(WTextInputAdaptor *ti)
{
    W_D(WInputMethodHelper);
    auto im = inputMethod();
    auto activeTI = focusedTextInput();
    if (activeTI == ti)
        return;
    if (activeTI) {
        d->lastTextInput = activeTI;
        if (im) {
            // If current active input method is not null, notify it to deactivate.
            im->deactivate();
            im->sendDone();
            connect(im, &WInputMethodAdaptor::committed, this, &WInputMethodHelper::handleFinalIMCommit);
        }
        // Notify last active text input to leave.
        activeTI->sendLeave();
    }
    setFocusedTextInput(ti);
    // Try to activate input method.
    if (im) {
        im->activate();
        im->sendDone();
    }
}

void WInputMethodHelper::handleTIDisabled(WTextInputAdaptor *ti)
{
    W_D(WInputMethodHelper);
    if (focusedTextInput() == ti) {
        d->lastTextInput = ti;
        // Should we consider the case when the same text input is disabled and then enabled at the same time.
        auto im = inputMethod();
        if (im) {
            im->deactivate();
            // When text input is disabled, commit current composing text
            im->sendDone();
            connect(im, &WInputMethodAdaptor::committed, this, &WInputMethodHelper::handleFinalIMCommit);
        }
        setFocusedTextInput(nullptr);
    }
}

void WInputMethodHelper::handleTIRequestFocus(WTextInputAdaptor *ti)
{
    if (ti->seat() && seatName() == ti->seat()->name()) {
        connectToTI(ti);
        if (seat()->keyboardFocus()) {
            ti->sendEnter(seat()->keyboardFocus());
        }
    }
}

void WInputMethodHelper::handleTIRequestLeave(WTextInputAdaptor *ti)
{
    ti->sendLeave();
}

void WInputMethodHelper::handleActiveTICommitted(WTextInputAdaptor *ti)
{
    setCursorRect(ti->cursorRect());
    updateAllPopupSurfaces(ti->cursorRect());
}

void WInputMethodHelper::handleFinalIMCommit(WInputMethodAdaptor *im)
{
    auto ti = lastFocusedTextInput();
    if (ti) {
        ti->handleIMCommitted(im);
    }
    // Only execute once
    disconnect(im, &WInputMethodAdaptor::committed, this, &WInputMethodHelper::handleFinalIMCommit);
}

void WInputMethodHelper::connectToTI(WTextInputAdaptor *ti)
{
    // We do not need to handle committed signal as when active text input or
    // active input method changes, committed will be connected or disconnected.
    connect(ti, &WTextInputAdaptor::enabled, this, &WInputMethodHelper::handleTIEnabled, Qt::UniqueConnection);
    connect(ti, &WTextInputAdaptor::disabled, this, &WInputMethodHelper::handleTIDisabled, Qt::UniqueConnection);
    connect(ti, &WTextInputAdaptor::requestLeave, this, &WInputMethodHelper::handleTIRequestLeave, Qt::UniqueConnection);
}

void WInputMethodHelper::disconnectTI(WTextInputAdaptor *ti)
{
    disconnect(ti, &WTextInputAdaptor::enabled, this, &WInputMethodHelper::handleTIEnabled);
    disconnect(ti, &WTextInputAdaptor::disabled, this, &WInputMethodHelper::handleTIDisabled);
    disconnect(ti, &WTextInputAdaptor::requestLeave, this, &WInputMethodHelper::handleTIRequestLeave);
}

void WInputMethodHelper::onNewKeyboardGrab(WKeyboardGrabAdaptor *keyboardGrab)
{
    W_D(WInputMethodHelper);
    keyboardGrab->setKeyboard(wseat()->keyboard());
    connect(wseat(), &WSeat::keyboardChanged, keyboardGrab, [this, keyboardGrab](){
        keyboardGrab->setKeyboard(wseat()->keyboard());
    });
    d->grabInterface = *wseat()->nativeHandle()->keyboard_state.grab->interface;
    d->grabInterface.key = handleKey;
    d->grabInterface.modifiers = handleModifiers;
    d->keyboardGrab.seat = wseat()->nativeHandle();
    d->handlerArg.grab = keyboardGrab;
    d->keyboardGrab.data = &d->handlerArg;
    d->keyboardGrab.interface = &d->grabInterface;
    wseat()->handle()->keyboardStartGrab(&d->keyboardGrab);
    connect(keyboardGrab, &WKeyboardGrabAdaptor::beforeDestroy, keyboardGrab, &WKeyboardGrabAdaptor::endGrab);
}


void WInputMethodHelper::onInputPopupSurface(WInputPopupSurfaceAdaptor *popupSurface)
{
    W_D(WInputMethodHelper);

    auto createPopupSurface = [this, d] (WSurface *focus, QRect cursorRect, WInputPopupSurfaceAdaptor *popupSurface){
        auto surface = new WInputPopup(popupSurface, focus, this);
        d->popupSurfaces.append(surface);
        Q_EMIT inputPopupSurfaceV2Added(surface);
        updatePopupSurface(surface, cursorRect);
        connect(popupSurface, &WInputPopupSurfaceAdaptor::beforeDestroy, this, [this, d, surface](){
            d->popupSurfaces.removeAll(surface);
            Q_EMIT inputPopupSurfaceV2Removed(surface);
            surface->deleteLater();
        });
    };

    if (auto ti = focusedTextInput()) {
        createPopupSurface(ti->focusedSurface(), ti->cursorRect(), popupSurface);
    }
}

void WInputMethodHelper::notifyLeave()
{
    if (auto ti = focusedTextInput()) {
        ti->sendLeave();
    }
}

void WInputMethodHelper::updateAllPopupSurfaces(QRect cursorRect)
{
    qDebug() << "popup surfaces:" << d_func()->popupSurfaces.size();
    for (auto popup : d_func()->popupSurfaces) {
        updatePopupSurface(popup, cursorRect);
    }
}

void WInputMethodHelper::updatePopupSurface(WInputPopup *popup, QRect cursorRect)
{
    Q_ASSERT(popup->handle());
    popup->handle()->sendTextInputRectangle(cursorRect);
}

void WInputMethodHelper::sendKeyboardFocus()
{
    W_D(WInputMethodHelper);
    auto focus = seat()->keyboardFocus();
    if (!focus)
        return;
    for (auto textInput : d->textInputs) {
        if (wl_resource_get_client(focus->handle()->handle()->resource) == textInput->waylandClient()) {
            textInput->sendEnter(focus);
        }
    }
}

WQuickSeat *WInputMethodHelper::seat() const
{
    W_DC(WInputMethodHelper);
    return d->seat;
}

void WInputMethodHelper::setSeat(WQuickSeat *seat)
{
    W_D(WInputMethodHelper);
    if (d->seat == seat)
        return;
    if (d->seat) {
        d->seat->disconnect(this);
    }
    d->seat = seat;
    if (seat) {
        connect(d->seat, &WQuickSeat::keyboardFocusChanged, this, &WInputMethodHelper::resendKeyboardFocus);
    }
    Q_EMIT seatChanged();
}

WSeat *WInputMethodHelper::wseat() const
{
    return seat()->seat();
}
WAYLIB_SERVER_END_NAMESPACE
