// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <WSurfaceLayout>

#include <QQuickWindow>

WAYLIB_SERVER_BEGIN_NAMESPACE

class WOutputWindowPrivate;
class WAYLIB_SERVER_EXPORT WOutputWindow : public QQuickWindow, public WObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WOutputWindow)
    Q_PROPERTY(WOutput* output READ output WRITE setOutput NOTIFY outputChanged)
    QML_NAMED_ELEMENT(OutputWindow)

public:
    explicit WOutputWindow(QObject *parent = nullptr);
    ~WOutputWindow();

    QQuickRenderControl *renderControl() const;
    WOutput *output() const;
    void setOutput(WOutput *newOutput);

Q_SIGNALS:
    void outputChanged();
};

WAYLIB_SERVER_END_NAMESPACE
