// Copyright (C) 2023 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <winputmethodhelper.h>

#include <QObject>
#include <QQmlEngine>
#include <QQuickItem>

Q_MOC_INCLUDE("wquickseat_p.h")
Q_MOC_INCLUDE("wquicktextinputv1_p.h")
Q_MOC_INCLUDE("wquicktextinputv2_p.h")
Q_MOC_INCLUDE("wquicktextinputv3_p.h")
Q_MOC_INCLUDE("wquickinputmethodv2_p.h")
Q_MOC_INCLUDE("wquickvirtualkeyboardv1_p.h")

WAYLIB_SERVER_BEGIN_NAMESPACE
class WQuickSeat;
class WQuickTextInputManagerV1;
class WQuickTextInputManagerV2;
class WQuickTextInputManagerV3;
class WQuickInputMethodManagerV2;
class WQuickInputMethodV2;
class WQuickInputMethodKeyboardGrabV2;
class WQuickVirtualKeyboardManagerV1;
class WQuickVirtualKeyboardV1;
class WInputMethodHelperPrivate;
class WSeat;
class WInputPopup;
class WQuickInputPopupSurfaceV2;
class WInputMethodHelper : public QObject, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WInputMethodHelper)
    QML_NAMED_ELEMENT(InputMethodHelper)
    Q_PROPERTY(WQuickSeat *seat READ seat WRITE setSeat NOTIFY seatChanged FINAL REQUIRED)
    Q_PROPERTY(WQuickTextInputManagerV1 *textInputManagerV1 READ textInputManagerV1 WRITE setTextInputManagerV1 NOTIFY textInputManagerV1Changed FINAL REQUIRED)
    Q_PROPERTY(WQuickTextInputManagerV2 *textInputManagerV2 READ textInputManagerV2 WRITE setTextInputManagerV2 NOTIFY textInputManagerV2Changed FINAL REQUIRED)
    Q_PROPERTY(WQuickTextInputManagerV3 *textInputManagerV3 READ textInputManagerV3 WRITE setTextInputManagerV3 NOTIFY textInputManagerV3Changed FINAL REQUIRED)
    Q_PROPERTY(WQuickInputMethodManagerV2 *inputMethodManagerV2 READ inputMethodManagerV2 WRITE setInputMethodManagerV2 NOTIFY inputMethodManagerV2Changed FINAL REQUIRED)
    Q_PROPERTY(WQuickVirtualKeyboardManagerV1 *virtualKeyboardManagerV1 READ virtualKeyboardManagerV1 WRITE setVirtualKeyboardManagerV1 NOTIFY virtualKeyboardManagerV1Changed FINAL REQUIRED)
    Q_PROPERTY(QQuickItem* activeFocusItem READ activeFocusItem WRITE setActiveFocusItem NOTIFY activeFocusItemChanged FINAL REQUIRED)
    Q_PROPERTY(QRect cursorRect READ cursorRect NOTIFY cursorRectChanged FINAL)

Q_SIGNALS:
    void seatChanged();
    void textInputManagerV1Changed();
    void textInputManagerV2Changed();
    void textInputManagerV3Changed();
    void inputMethodManagerV2Changed();
    void virtualKeyboardManagerV1Changed();
    void activeFocusItemChanged();
    void cursorRectChanged();
    void inputPopupSurfaceV2Added(WInputPopup *popupSurface);
    void inputPopupSurfaceV2Removed(WInputPopup *popupSurface);
    void activeTextInputChanged();
    void activeInputMethodChanged();

    // void activeTextInputV1Changed(WQuickTextInputV1 *newActiveTextInputV1);
    // void activeTextInputV3Changed(WQuickTextInputV3 *newActiveTextInputV3);
    // void inputMethodV2Changed(WQuickInputMethodV2 *newInputMethod);

public:
    explicit WInputMethodHelper(QObject *parent = nullptr);
    WQuickSeat *seat() const;
    WSeat *wseat() const;
    QString seatName() const;
    WQuickTextInputManagerV1 *textInputManagerV1() const;
    WQuickTextInputManagerV2 *textInputManagerV2() const;
    WQuickTextInputManagerV3 *textInputManagerV3() const;
    WQuickInputMethodManagerV2 *inputMethodManagerV2() const;
    WQuickVirtualKeyboardManagerV1 *virtualKeyboardManagerV1() const;
    QQuickItem *activeFocusItem() const;
    QRect cursorRect() const;
    WTextInputAdaptor *focusedTextInput() const;
    WInputMethodAdaptor *inputMethod() const;
    WTextInputAdaptor *lastFocusedTextInput() const;

    void setSeat(WQuickSeat *qseat);
    void setTextInputManagerV1(WQuickTextInputManagerV1 *textInputManagerV1);
    void setTextInputManagerV2(WQuickTextInputManagerV2 *textInputManagerV2);
    void setTextInputManagerV3(WQuickTextInputManagerV3 *textInputManagerV3);
    void setInputMethodManagerV2(WQuickInputMethodManagerV2 *inputMethodManagerV2);
    void setVirtualKeyboardManagerV1(WQuickVirtualKeyboardManagerV1 *virtualKeyboardManagerV1);
    void setCursorRect(const QRect &rect);
    void setActiveFocusItem(QQuickItem *item);
    void setFocusedTextInput(WTextInputAdaptor *ti);
    void setInputMethod(WInputMethodAdaptor *im);
    void connectActiveIMAndTI();
    void disconnectActiveIMAndTI();

    void handleNewTI(QObject *ti);
    void handleNewIMV2(WQuickInputMethodV2 *imv2);
    void handleNewVKV1(WQuickVirtualKeyboardV1 *vkv1);
    void updateAllPopupSurfaces(QRect cursorRect);
    void updatePopupSurface(WInputPopup *popup, QRect cursorRect);
    void notifyLeave();
    void sendKeyboardFocus();
    void resendKeyboardFocus();
    void tryAddTextInput(WTextInputAdaptor *ti);
    void handleTIEnabled(WTextInputAdaptor *ti);
    void handleTIDisabled(WTextInputAdaptor *ti);
    void handleTIRequestFocus(WTextInputAdaptor *ti);
    void handleTIRequestLeave(WTextInputAdaptor *ti);
    void handleCursorRectChanged(QRect rect);
    void handleActiveTICommitted(WTextInputAdaptor *ti);
    void handleFinalIMCommit(WInputMethodAdaptor *im);

    void connectToTI(WTextInputAdaptor *ti);
    void disconnectTI(WTextInputAdaptor *ti);
    void onNewKeyboardGrab(WKeyboardGrabAdaptor *keyboardGrab);
    void onInputPopupSurface(WInputPopupSurfaceAdaptor *popupSurface);

    // WQuickInputMethodV2 *inputMethodV2() const;
    // void setInputMethodV2(WQuickInputMethodV2 *newInputMethodV2);
    // void onNewTextInputV1(WQuickTextInputV1 *newTextInputV1);
    // void onNewTextInputV3(WQuickTextInputV3 *newTextInputV3);
    // void onTextInputV1Deactivated(WQuickTextInputV1 *textInputV1);
    // void onTextInputV3Enabled(WQuickTextInputV3 *enabledTextInput);
    // void onTextInputV3Disabled(WQuickTextInputV3 *disabledTextInput);
    // void onTextInputV3Committed(WQuickTextInputV3 *textInput);
    // void onTextInputV3Destroyed(WQuickTextInputV3 *destroyedTextInput);
    // void onInputMethodV2Committed();
    // void sendInputMethodV2State(WQuickTextInputV3 *textInputV3);
};
WAYLIB_SERVER_END_NAMESPACE
