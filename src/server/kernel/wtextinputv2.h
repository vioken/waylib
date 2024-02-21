// Copyright (C) 2024 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include <wglobal.h>
#include <wsurface.h>

#include <qwglobal.h>
#include <qwdisplay.h>

#include <QObject>

extern "C" {
#include <text-input-unstable-v2-protocol.h>
}

WAYLIB_SERVER_BEGIN_NAMESPACE
class WTextInputV2Private;
class WTextInputManagerV2Private;
class WTextInputV2;
class WSeat;
struct ws_text_input_v2;
struct ws_text_input_manager_v2;

class WTextInputManagerV2 : public QObject, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WTextInputManagerV2)

public:
    static WTextInputManagerV2 *create(QWLRoots::QWDisplay *display);

Q_SIGNALS:
    void newTextInputV2(WTextInputV2 *textInputV2);
    void beforeDestroy(WTextInputManagerV2 *self);

private:
    WTextInputManagerV2(ws_text_input_manager_v2 *handle);
    ~WTextInputManagerV2() = default;
};

class WTextInputV2 : public QObject, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WTextInputV2)
    Q_PROPERTY(WSeat *seat READ seat CONSTANT)
    Q_PROPERTY(WSurface *focusedSurface READ focusedSurface NOTIFY focusedSurfaceChanged)
    Q_PROPERTY(QString surroundingText READ surroundingText NOTIFY surroundingTextChanged)
    Q_PROPERTY(int surroundingCursor READ surroundingCursor NOTIFY surroundingTextChanged)
    Q_PROPERTY(int surroundingAnchor READ surroundingAnchor NOTIFY surroundingTextChanged)
    Q_PROPERTY(uint contentHint READ contentHint NOTIFY contentTypeChanged)
    Q_PROPERTY(uint contentPurpose READ contentPurpose NOTIFY contentTypeChanged)
    Q_PROPERTY(QRect cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged)
    Q_PROPERTY(QString preferredLanguage READ preferredLanguage NOTIFY preferredLanguageChanged)

public:
    WSeat *seat() const;
    WSurface *focusedSurface() const;
    QString surroundingText() const;
    int surroundingCursor() const;
    int surroundingAnchor() const;
    uint contentHint() const;
    uint contentPurpose() const;
    QRect cursorRectangle() const;
    QString preferredLanguage() const;
    wl_client *waylandClient() const;

Q_SIGNALS:
    void beforeDestroy(WTextInputV2 *self);
    void enable(WSurface *surface);
    void disable(WSurface *surface);
    void showInputPanel();
    void hideInputPanel();
    void updateState(uint reason);
    void focusedSurfaceChanged();
    void surroundingTextChanged();
    void contentTypeChanged();
    void cursorRectangleChanged();
    void preferredLanguageChanged();

public Q_SLOTS:
    void sendEnter(WSurface *surface);
    void sendLeave(WSurface *surface);
    void sendInputPanelState(uint visibility, QRect geometry);
    void sendPreeditString(const QString &text, const QString &commit);
    void sendPreeditStyling(uint index, uint length, uint style);
    void sendPreeditCursor(int index);
    void sendCommitString(const QString &text);
    void sendCursorPosition(int index, int anchor);
    void sendDeleteSurroundingText(uint beforeLength, uint afterLength);
    void sendModifiersMap(const QStringList &map);
    void sendKeysym(uint time, uint sym, uint state, uint modifiers);
    void sendLanguage(const QString &language);
    void sendConfigureSurroundingText(int beforeCursor, int afterCursor);
    void sendInputMethodChanged(uint flags);

private:
    WTextInputV2(ws_text_input_v2 *handle);
    ~WTextInputV2() = default;
    friend class WTextInputManagerV2Private;
};
WAYLIB_SERVER_END_NAMESPACE
