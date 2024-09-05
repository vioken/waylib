// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QQmlApplicationEngine>
#include <QQmlComponent>

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

class SurfaceWrapper;
class Output;
class QmlEngine : public QQmlApplicationEngine
{
    Q_OBJECT
public:
    explicit QmlEngine(QObject *parent = nullptr);

    QQuickItem *createTitleBar(SurfaceWrapper *surface, QQuickItem *parent);
    QQuickItem *createDecoration(SurfaceWrapper *surface, QQuickItem *parent);
    QQuickItem *createTaskBar(Output *output, QQuickItem *parent);

private:
    QQmlComponent titleBarComponent;
    QQmlComponent decorationComponent;
    QQmlComponent taskBarComponent;
};
