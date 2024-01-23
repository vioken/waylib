// Copyright (C) 2023 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <QObject>
#include <QQmlEngine>
#include <wquickwaylandserver.h>

Q_MOC_INCLUDE(<wseat.h>)
Q_MOC_INCLUDE(<wsurface.h>)
Q_MOC_INCLUDE(<wquickinputmethodv2_p.h>)

#define IM WAYLIB_SERVER_NAMESPACE::im
#define IM_USE_NAMESPACE using IM;
struct wlr_seat_keyboard_grab;
WAYLIB_SERVER_BEGIN_NAMESPACE
class WSeat;
class WSurface;
class WQuickInputMethodV2;

namespace im {
Q_NAMESPACE
QML_NAMED_ELEMENT(WIM)
enum ChangeCause {
    CC_InputMethod,
    CC_Other
};
Q_ENUM_NS(ChangeCause)

enum ContentHint {
    CH_None = 0x0,
    CH_Completion = 0x1,
    CH_Spellcheck = 0x2,
    CH_AutoCapitalization = 0x4,
    CH_Lowercase = 0x8,
    CH_Uppercase = 0x10,
    CH_Titlecase = 0x20,
    CH_HiddenText = 0x40,
    CH_SensitiveData = 0x80,
    CH_Latin = 0x100,
    CH_Multiline = 0x200
};
Q_FLAG_NS(ContentHint)
Q_DECLARE_FLAGS(ContentHints, ContentHint)

enum ContentPurpose {
    CP_Normal,
    CP_Alpha,
    CP_Digits,
    CP_Number,
    CP_Phone,
    CP_Url,
    CP_Email,
    CP_Name,
    CP_Password,
    CP_Pin,
    CP_Date,
    CP_Time,
    CP_Datetime,
    CP_Terminal
};
Q_ENUM_NS(ContentPurpose)

enum Feature {
    F_SurroundingText,
    F_ContentType,
    F_CursorRect
};
Q_FLAG_NS(Feature)
Q_DECLARE_FLAGS(Features, Feature)

enum KeyboardModifier {
    KM_Shift = 1 << 0,
    KM_Caps = 1 << 1,
    KM_Ctrl = 1 << 2,
    KM_Alt = 1 << 3,
    KM_Mod2 = 1 << 4,
    KM_Mod3 = 1 << 5,
    KM_Logo = 1 << 6,
    KM_Mod5 = 1 << 7,
};
Q_ENUM_NS(KeyboardModifier)
}
class WTextInputAdaptor;
class WInputPopupSurfaceAdaptor;
class WKeyboardGrabAdaptor;
class WVirtualKeyboardManagerAdaptor;
class WInputMethodAdaptor;
class WVirtualKeyboardAdaptor;
class WInputDevice;

class WVirtualKeyboardAdaptor : public QObject
{
    Q_OBJECT
public:
    explicit WVirtualKeyboardAdaptor(QObject *parent = nullptr) : QObject(nullptr) { }
    virtual WInputDevice *keyboard() const = 0;

Q_SIGNALS:
    void beforeDestroy();
};

class WInputMethodAdaptor : public QObject
{
    Q_OBJECT
public:
    virtual WSeat *seat() const = 0;
    virtual QString commitString() const = 0;
    virtual uint deleteSurroundingBeforeLength() const = 0;
    virtual uint deleteSurroundingAfterLength() const = 0;
    virtual QString preeditString() const = 0;
    virtual int preeditCursorBegin() const = 0;
    virtual int preeditCursorEnd() const = 0;
    explicit WInputMethodAdaptor(QObject *parent = nullptr) : QObject(parent) { }

Q_SIGNALS:
    void committed(WInputMethodAdaptor *self);
    void newInputPopupSurface(WInputPopupSurfaceAdaptor *surface);
    void newKeyboardGrab(WKeyboardGrabAdaptor *keyboardGrab);
    void beforeDestroy(WInputMethodAdaptor *self);

public Q_SLOTS:
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual void sendDone() = 0;
    virtual void sendUnavailable() = 0;
    virtual void sendContentType(IM::ContentHints hints, IM::ContentPurpose purpose) = 0;
    virtual void sendSurroundingText(const QString &text, uint cursor, uint anchor) = 0;
    virtual void sendTextChangeCause(IM::ChangeCause cause) = 0;
    virtual void handleTICommitted(WTextInputAdaptor *textInput) = 0;
};

class WKeyboardGrabAdaptor : public QObject
{
    Q_OBJECT
public:
    explicit WKeyboardGrabAdaptor(QObject *parent = nullptr) : QObject(parent) {}
    virtual WInputDevice *keyboard() const = 0;

Q_SIGNALS:
    void beforeDestroy(WKeyboardGrabAdaptor *self);

public Q_SLOTS:
    virtual void sendKey(uint time, Qt::Key key, uint state) = 0;
    virtual void sendModifiers(uint depressed, uint latched, uint locked, uint group) = 0;
    virtual void setKeyboard(WInputDevice *keyboard) = 0;
    virtual void startGrab(wlr_seat_keyboard_grab *grab) = 0;
    virtual void endGrab() = 0;
};

class WInputPopupSurfaceAdaptor : public QObject
{
    Q_OBJECT
public:
    explicit WInputPopupSurfaceAdaptor(QObject *parent = nullptr) : QObject(parent) {}
    virtual WSurface *surface() const = 0;

Q_SIGNALS:
    void beforeDestroy(WInputPopupSurfaceAdaptor *self);
public Q_SLOTS:
    virtual void sendTextInputRectangle(const QRect &sbox) = 0;
};

class WTextInputAdaptor : public QObject
{
    Q_OBJECT
public:
    virtual WSeat *seat() const = 0;
    virtual WSurface *focusedSurface() const = 0;
    virtual QString surroundingText() const = 0;
    virtual int surroundingCursor() const = 0;
    virtual int surroundingAnchor() const = 0;
    virtual IM::ChangeCause textChangeCause() const { return IM::CC_InputMethod; }
    virtual IM::ContentHints contentHints() const { return IM::CH_None; }
    virtual IM::ContentPurpose contentPurpose() const { return IM::CP_Normal; }
    virtual QRect cursorRect() const = 0;
    virtual IM::Features features() const = 0;
    virtual wl_client* waylandClient() const = 0;

    explicit WTextInputAdaptor(QObject *parent = nullptr) : QObject(parent) { }

Q_SIGNALS:
    void beforeDestroy(WTextInputAdaptor *self);
    void enabled(WTextInputAdaptor *self);
    void disabled(WTextInputAdaptor *self);
    void committed(WTextInputAdaptor *self);
    void requestFocus(WTextInputAdaptor *self); // This is for text input v1's activate signal
    void requestLeave(WTextInputAdaptor *self); // For text input v1's deactivate signal

public Q_SLOTS:
    virtual void sendEnter(WSurface *surface) = 0;
    virtual void sendLeave() = 0;
    virtual void sendDone() = 0;
    virtual void handleIMCommitted(WInputMethodAdaptor *im) = 0;
    virtual void handleKeyboardFocusChanged(WSurface *keyboardFocus) = 0;
};
WAYLIB_SERVER_END_NAMESPACE
