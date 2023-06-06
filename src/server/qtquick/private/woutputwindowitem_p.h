// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>

#include <QQuickItem>

WAYLIB_SERVER_BEGIN_NAMESPACE

class WOutputWindow;
class WOutputWindowItemPrivate;
class WAYLIB_SERVER_EXPORT WOutputWindowItem : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WOutputWindowItem)
    Q_PRIVATE_PROPERTY(WOutputWindowItem::d_func()->window, WOutput* output READ output WRITE setOutput NOTIFY outputChanged)
    Q_PRIVATE_PROPERTY(WOutputWindowItem::d_func(), QQmlListProperty<QObject> data READ data DESIGNABLE false)
    QML_NAMED_ELEMENT(OutputWindowItem)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    explicit WOutputWindowItem(QQuickItem *parent = nullptr);

    WOutputWindow *window() const;

Q_SIGNALS:
    void outputChanged();

private:
    void classBegin() override;
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
};

WAYLIB_SERVER_END_NAMESPACE
