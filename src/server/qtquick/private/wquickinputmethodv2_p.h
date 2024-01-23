// Copyright (C) 2023 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <wquickwaylandserver.h>
#include <wtoplevelsurface.h>
#include <winputmethodhelper.h>
#include <wsurfaceitem.h>

#include <qwglobal.h>
#include <qwinputmethodv2.h>

#include <QObject>
#include <QQmlEngine>

Q_MOC_INCLUDE("wquicktextinputv3_p.h")
Q_MOC_INCLUDE("wsurface.h")

QW_BEGIN_NAMESPACE
class QWInputMethodV2;
class QWInputPopupSurfaceV2;
class QWInputMethodKeyboardGrabV2;
QW_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE
class WQuickInputPopupSurfaceV2Private;
class WQuickInputMethodKeyboardGrabV2Private;
class WQuickInputMethodV2Private;
class WInputDevice;
class WSeat;
class WQuickInputMethodV2;
class WQuickInputMethodManagerV2Private;
class WQuickTextInputV3;
class WSurface;
class WInputPopupPrivate;

class WQuickInputMethodManagerV2 : public WQuickWaylandServerInterface, public WObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InputMethodManagerV2)
    W_DECLARE_PRIVATE(WQuickInputMethodManagerV2)
    Q_PROPERTY(QList<WQuickInputMethodV2 *> inputMethods READ inputMethods NOTIFY inputMethodsChanged FINAL)

public:
    explicit WQuickInputMethodManagerV2(QObject *parent = nullptr);
    QList<WQuickInputMethodV2 *> inputMethods() const;

Q_SIGNALS:
    void newInputMethod(WQuickInputMethodV2 *im);
    void inputMethodsChanged();

private:
    void create() override;
};

class WQuickInputPopupSurfaceV2 : public QObject, public WObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InputPopupSurfaceV2)
    QML_UNCREATABLE("Only created by InputMethodV2 in C++")
    W_DECLARE_PRIVATE(WQuickInputPopupSurfaceV2)
    Q_PROPERTY(WSurface* surface READ surface CONSTANT FINAL)

public:
    explicit WQuickInputPopupSurfaceV2(QW_NAMESPACE::QWInputPopupSurfaceV2 *handle, QObject *parent = nullptr);
    WSurface *surface() const;
    QW_NAMESPACE::QWInputPopupSurfaceV2 *handle() const;

public Q_SLOTS:
    void sendTextInputRectangle(const QRect &sbox);
};

class WInputPopup : public WToplevelSurface, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WInputPopup)
    QML_NAMED_ELEMENT(WaylandInputPopupSurface)
    QML_UNCREATABLE("Only created in C++")

public:
    WInputPopup(WInputPopupSurfaceAdaptor *surface, WSurface *parentSurface, QObject *parent = nullptr);
    WSurface *surface() const override;
    WInputPopupSurfaceAdaptor *handle() const;
    QRect getContentGeometry() const override;
    bool doesNotAcceptFocus() const override;
    bool isActivated() const override;
    WSurface *parentSurface() const override;

public Q_SLOTS:
    bool checkNewSize(const QSize &size) override;
};

class WInputPopupItem : public WSurfaceItem
{
    Q_OBJECT
    Q_PROPERTY(WInputPopup* surface READ surface WRITE setSurface NOTIFY surfaceChanged FINAL REQUIRED)
    QML_NAMED_ELEMENT(InputPopupSurfaceItem)

public:
    explicit WInputPopupItem(QQuickItem *parent = nullptr);

    WInputPopup *surface() const;
    void setSurface(WInputPopup *surface);

Q_SIGNALS:
    void surfaceChanged();

private:
    WInputPopup *m_inputPopupSurface;
};

class WQuickInputMethodKeyboardGrabV2 : public QObject, public WObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InputMethodKeyboardGrabV2)
    QML_UNCREATABLE("Only created by InputMethodV2 in C++")
    W_DECLARE_PRIVATE(WQuickInputMethodKeyboardGrabV2)

public:
    explicit WQuickInputMethodKeyboardGrabV2(QW_NAMESPACE::QWInputMethodKeyboardGrabV2 *handle, QObject *parent = nullptr);
    QW_NAMESPACE::QWInputMethodKeyboardGrabV2 *handle() const;
    WInputDevice *keyboard() const;

public Q_SLOTS:
    void sendKey(uint time, Qt::Key key, uint state);
    void sendModifiers(uint depressed, uint latched, uint locked, uint group);
    void setKeyboard(WInputDevice *keyboard);
};

class WQuickInputMethodV2 : public QObject, public WObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InputMethodV2)
    QML_UNCREATABLE("Only created by InputMethodManagerV2 in C++")
    W_DECLARE_PRIVATE(WQuickInputMethodV2)
    Q_PROPERTY(WSeat *seat READ seat CONSTANT FINAL)
    Q_PROPERTY(QList<WQuickInputPopupSurfaceV2 *> popupSurfaces READ popupSurfaces NOTIFY popupSurfacesChanged FINAL)
    Q_PROPERTY(WQuickInputMethodKeyboardGrabV2 *activeKeyboardGrab READ activeKeyboardGrab NOTIFY activeKeyboardGrabChanged FINAL)
    Q_PROPERTY(QString commitString READ commitString NOTIFY committed FINAL)
    Q_PROPERTY(quint32 deleteSurroundingBeforeLength READ deleteSurroundingBeforeLength NOTIFY committed FINAL)
    Q_PROPERTY(quint32 deleteSurroundingAfterLength READ deleteSurroundingAfterLength NOTIFY committed FINAL)
    Q_PROPERTY(QString preeditString READ preeditString NOTIFY committed FINAL)
    Q_PROPERTY(qint32 preeditCursorBegin READ preeditCursorBegin NOTIFY committed FINAL)
    Q_PROPERTY(qint32 preeditCursorEnd READ preeditCursorEnd NOTIFY committed FINAL)

public:
    WSeat *seat() const;
    QString commitString() const;
    uint deleteSurroundingBeforeLength() const;
    uint deleteSurroundingAfterLength() const;
    QString preeditString() const;
    int preeditCursorBegin() const;
    int preeditCursorEnd() const;
    QList<WQuickInputPopupSurfaceV2 *> popupSurfaces() const;
    WQuickInputMethodKeyboardGrabV2 *activeKeyboardGrab() const;
    QW_NAMESPACE::QWInputMethodV2 *handle() const;

public Q_SLOTS:
    void sendContentType(quint32 hint, quint32 purpose);
    void sendActivate();
    void sendDeactivate();
    void sendDone();
    void sendSurroundingText(const QString &text, quint32 cursor, quint32 anchor);
    void sendTextChangeCause(quint32 cause);
    void sendUnavailable();

Q_SIGNALS:
    void committed();
    void newPopupSurface(WQuickInputPopupSurfaceV2 *surface);
    void newKeyboardGrab(WQuickInputMethodKeyboardGrabV2 *keyboardGrab);
    void popupSurfacesChanged();
    void activeKeyboardGrabChanged();

private:
    explicit WQuickInputMethodV2(QW_NAMESPACE::QWInputMethodV2 *handle, QObject *parent = nullptr);
    friend class WQuickInputMethodManagerV2;
};

class WInputMethodV2Adaptor : public WInputMethodAdaptor
{
    Q_OBJECT
public:
    explicit WInputMethodV2Adaptor(WQuickInputMethodV2 *imv2);
    WSeat *seat() const override;
    QString commitString() const override;
    uint deleteSurroundingBeforeLength() const override;
    uint deleteSurroundingAfterLength() const override;
    QString preeditString() const override;
    int preeditCursorBegin() const override;
    int preeditCursorEnd() const override;

public Q_SLOTS:
    void activate() override;
    void deactivate() override;
    void sendDone() override;
    void sendUnavailable() override;
    void sendContentType(im::ContentHints hints, im::ContentPurpose purpose) override;
    void sendSurroundingText(const QString &text, uint cursor, uint anchor) override;
    void sendTextChangeCause(im::ChangeCause cause) override;
    void handleTICommitted(WTextInputAdaptor *textInput) override;

private:
    WQuickInputMethodV2 *m_im;
};

class WKeyboardGrabV2Adaptor : public WKeyboardGrabAdaptor
{
    Q_OBJECT
public:
    explicit WKeyboardGrabV2Adaptor(WQuickInputMethodKeyboardGrabV2 *kgv2, WSeat *seat);
    WInputDevice *keyboard() const override;

public Q_SLOTS:
    void sendKey(uint time, Qt::Key key, uint state) override;
    void sendModifiers(uint depressed, uint latched, uint locked, uint group) override;
    void setKeyboard(WInputDevice *keyboard) override;
    void startGrab(wlr_seat_keyboard_grab *grab) override;
    void endGrab() override;

private:
    WQuickInputMethodKeyboardGrabV2 *m_kg;
    WSeat *m_seat; // save the seat where the grab is created
};

class WInputPopupSurfaceV2Adaptor : public WInputPopupSurfaceAdaptor
{
    Q_OBJECT
public:
    explicit WInputPopupSurfaceV2Adaptor(WQuickInputPopupSurfaceV2 *ipsv2);
    WSurface *surface() const override;

public Q_SLOTS:
    void sendTextInputRectangle(const QRect &sbox) override;

private:
    WQuickInputPopupSurfaceV2 *m_ips;
};
WAYLIB_SERVER_END_NAMESPACE
