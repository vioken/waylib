// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include "surfacecontainer.h"

class SurfaceWrapper;
class Workspace;
class WorkspaceModel : public QObject
{
    friend class Workspace;
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(SurfaceListModel* model READ model CONSTANT FINAL)
    Q_PROPERTY(bool visable READ visable WRITE setVisible NOTIFY visableChanged FINAL)

    QML_ELEMENT

public:
    explicit WorkspaceModel(QObject *parent, int index);

    QString name() const;
    void setName(const QString &newName);

    int index() const;
    void setIndex(int newIndex);

    bool visable() const;
    void setVisible(bool visible);

    void addSurface(SurfaceWrapper *surface);
    void removeSurface(SurfaceWrapper *surface);

    const QList<SurfaceWrapper*> &surfaces() const {
        return m_model->surfaces();
    }

    SurfaceListModel *model() const {
        return m_model;
    }

Q_SIGNALS:
    void nameChanged();
    void indexChanged();
    void visableChanged();

private:
    QString m_name;
    SurfaceListModel *m_model = nullptr;
    int m_index = -1;
    bool m_visable;
};
