// Copyright (C) 2024 Yixue Wang <wangyixue@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <wquickwaylandserver.h>
#include <wtextinputv2.h>
#include <winputmethodhelper.h>

#include <qwglobal.h>

#include <QObject>
#include <QQmlEngine>

Q_MOC_INCLUDE(<wsurfaceitem.h>)

WAYLIB_SERVER_BEGIN_NAMESPACE
class WQuickTextInputManagerV2Private;
class WQuickTextInputV2Private;
class WQuickTextInputV2;
class WTextInputV2;

class WQuickTextInputManagerV2 : public WQuickWaylandServerInterface, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WQuickTextInputManagerV2)
    QML_NAMED_ELEMENT(TextInputManagerV2)
    Q_PROPERTY(QList<WQuickTextInputV2 *> textInputs READ textInputs NOTIFY textInputsChanged FINAL)

public:
    explicit WQuickTextInputManagerV2(QObject *parent = nullptr);
    QList<WQuickTextInputV2 *> textInputs() const;

Q_SIGNALS:
    void newTextInput(WQuickTextInputV2 *textInput);
    void textInputsChanged();

protected:
    void create() override;
};

class WQuickTextInputV2 : public QObject, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WQuickTextInputV2)
    QML_NAMED_ELEMENT(TextInputV2)
    QML_UNCREATABLE("Only created in C++ by WQuickTextInputManagerV2.")
    Q_PROPERTY(WSeat *seat READ seat CONSTANT FINAL)
    Q_PROPERTY(WSurface *focusedSurface READ focusedSurface NOTIFY focusedSurfaceChanged FINAL)
    Q_PROPERTY(QString surroundingText READ surroundingText NOTIFY surroundingTextChanged FINAL)
    Q_PROPERTY(int surroundingCursor READ surroundingCursor NOTIFY surroundingTextChanged FINAL)
    Q_PROPERTY(int surroundingAnchor READ surroundingAnchor NOTIFY surroundingTextChanged FINAL)
    Q_PROPERTY(ContentHints contentHints READ contentHints NOTIFY contentTypeChanged FINAL)
    Q_PROPERTY(ContentPurpose contentPurpose READ contentPurpose NOTIFY contentTypeChanged FINAL)
    Q_PROPERTY(QRect cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged FINAL)
    Q_PROPERTY(QString preferredLanguage READ preferredLanguage NOTIFY preferredLanguageChanged FINAL)

public:
    enum ContentHint {
        CH_None = 0x0,
        CH_AutoCompletion = 0x1,
        CH_AutoCorrection = 0x2,
        CH_AutoCapitalization = 0x4,
        CH_Lowercase = 0x8,
        CH_Uppercase = 0x10,
        CH_Titlecase = 0x20,
        CH_HiddenText = 0x40,
        CH_SensitiveData = 0x80,
        CH_Latin = 0x100,
        CH_Multiline = 0x200
    };
    Q_FLAG(ContentHint)
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
        CP_Date,
        CP_Time,
        CP_Datetime,
        CP_Terminal
    };
    Q_ENUM(ContentPurpose)

    enum UpdateState {
        US_Change,
        US_Full,
        US_Reset,
        US_Enter
    };
    Q_ENUM(UpdateState)

    enum InputPanelVisibility {
        IPV_Hidden,
        IPV_Visible
    };
    Q_ENUM(InputPanelVisibility)

    enum PreeditStyle {
        PS_Default,
        PS_None,
        PS_Active,
        PS_Inactive,
        PS_Highlight,
        PS_Underline,
        PS_Selection,
        PS_Incorrect
    };
    Q_ENUM(PreeditStyle)

    enum TextDirection {
        TD_Auto,
        TD_Ltr,
        TD_Rtl
    };
    Q_ENUM(TextDirection)

    WSeat *seat() const;
    WSurface *focusedSurface() const;
    QString surroundingText() const;
    int surroundingCursor() const;
    int surroundingAnchor() const;
    ContentHints contentHints() const;
    ContentPurpose contentPurpose() const;
    QRect cursorRectangle() const;
    QString preferredLanguage() const;
    WTextInputV2 *handle() const;

Q_SIGNALS:
    void enabled(WSurface *surface);
    void disabled(WSurface *surface);
    void showInputPanel();
    void hideInputPanel();
    void stateUpdated(UpdateState reason);
    void focusedSurfaceChanged();
    void surroundingTextChanged();
    void cursorRectangleChanged();
    void contentTypeChanged();
    void preferredLanguageChanged();

public Q_SLOTS:
    void sendEnter(WSurface *surface);
    void sendLeave(WSurface *surface);
    void sendInputPanelState(InputPanelVisibility visibility, QRect geometry);
    void sendPreeditString(const QString &text, const QString &commit);
    void sendPreeditStyling(uint index, uint length, PreeditStyle style);
    void sendPreeditCursor(int index);
    void sendCommitString(const QString &text);
    void sendCursorPosition(int index, int anchor);
    void sendDeleteSurroundingText(uint beforeLength, uint afterLength);
    void sendModifiersMap(QStringList map);
    void sendKeysym(uint time, Qt::Key sym, uint state, uint modifiers);
    void sendLanguage(const QString &language);
    void sendConfigureSurroundingText(int beforeCursor, int afterCursor);
    void sendInputMethodChanged(uint flags);

private:
    explicit WQuickTextInputV2(WTextInputV2 *handle, QObject *parent = nullptr);
    friend class WQuickTextInputManagerV2;
};

class WTextInputV2Adaptor : public WTextInputAdaptor
{
    Q_OBJECT
public:
    explicit WTextInputV2Adaptor(WQuickTextInputV2 *ti);
    WSeat *seat() const override;
    WSurface *focusedSurface() const override;
    QString surroundingText() const override;
    int surroundingCursor() const override;
    int surroundingAnchor() const override;
    im::ChangeCause textChangeCause() const override;
    im::ContentHints contentHints() const override;
    im::ContentPurpose contentPurpose() const override;
    QRect cursorRect() const override;
    im::Features features() const override;
    wl_client *waylandClient() const override;

public slots:
    void sendEnter(WSurface *surface) override;
    void sendLeave() override;
    void sendDone() override;
    void handleIMCommitted(WInputMethodAdaptor *im) override;
    void handleKeyboardFocusChanged(WSurface *keyboardFocus) override;

private:
    WQuickTextInputV2 *m_ti;
};
WAYLIB_SERVER_END_NAMESPACE
