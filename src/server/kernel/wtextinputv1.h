// Copyright (C) 2024 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include <wglobal.h>
#include <qwcompositor.h>

#include <QObject>

WAYLIB_SERVER_BEGIN_NAMESPACE
struct ws_text_input_v1;
struct ws_text_input_manager_v1;
class WSurface;
class WSeat;
class WTextInputV1;
class WTextInputManagerV1Private;
class WTextInputV1Private;
class WTextInputManagerV1 : public QObject, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WTextInputManagerV1)
public:
    static WTextInputManagerV1 *create(QW_NAMESPACE::QWDisplay *display);

Q_SIGNALS:
    void newTextInput(WTextInputV1 *ti);

private:
    explicit WTextInputManagerV1(ws_text_input_manager_v1 *handle);
};

class WTextInputV1 : public QObject, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WTextInputV1)
    Q_PROPERTY(WSeat *seat READ seat NOTIFY seatChanged FINAL)
    Q_PROPERTY(WSurface *focusedSurface READ focusedSurface NOTIFY focusedSurfaceChanged FINAL)
    Q_PROPERTY(QString surroundingText READ surroundingText NOTIFY surroundingTextChanged FINAL)
    Q_PROPERTY(uint surroundingCursor READ surroundingCursor NOTIFY surroundingTextChanged FINAL)
    Q_PROPERTY(uint surroundingAnchor READ surroundingAnchor NOTIFY surroundingTextChanged FINAL)
    Q_PROPERTY(uint contentHint READ contentHint NOTIFY contentTypeChanged FINAL)
    Q_PROPERTY(uint contentPurpose READ contentPurpose NOTIFY contentTypeChanged FINAL)
    Q_PROPERTY(QRect cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged FINAL)
    Q_PROPERTY(QString preferredLanguage READ preferredLanguage NOTIFY preferredLanguageChanged FINAL)

public:
    WSeat *seat() const;
    WSurface *focusedSurface() const;
    QString surroundingText() const;
    uint surroundingCursor() const;
    uint surroundingAnchor() const;
    uint contentHint() const;
    uint contentPurpose() const;
    QRect cursorRectangle() const;
    QString preferredLanguage() const;
    wl_client *waylandClient() const;

Q_SIGNALS:
    void activate(WSeat *seat, WSurface *surface);
    void deactivate(WSeat *seat);
    void showInputPanel();
    void hideInputPanel();
    void reset();
    void commitState();
    void invokeAction(uint button, uint index);
    void beforeDestroy(WTextInputV1 *self);
    // As text input v1 does not have a double-buffer mechanism, we should provide change signal for property
    // This is necessary for QML to perform property binding
    void surroundingTextChanged(); // text, cursor and anchor
    void contentTypeChanged(); // hint and purpose
    void cursorRectangleChanged();
    void preferredLanguageChanged();
    void seatChanged(WSeat *seat);
    void focusedSurfaceChanged(WSurface *surface);

public Q_SLOTS:
    void sendEnter(WSurface *surface);
    void sendLeave();
    void sendModifiersMap(QStringList map);
    void sendInputPanelState(uint state);
    void sendPreeditString(const QString &text, const QString &commit);
    void sendPreeditStyling(uint index, uint length, uint style);
    void sendPreeditCursor(int index);
    void sendCommitString(const QString &text);
    void sendCursorPosition(int index, int anchor);
    void sendDeleteSurroundingString(int index, uint length);
    void sendKeysym(uint time, uint sym, uint state, uint modifiers);
    void sendLanguage(const QString &language);
    void sendTextDirection(uint direction);

private:
    explicit WTextInputV1(ws_text_input_v1 *handle, QObject *parent = nullptr);
    friend class WTextInputManagerV1Private;
};

WAYLIB_SERVER_END_NAMESPACE
