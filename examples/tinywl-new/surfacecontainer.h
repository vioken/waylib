// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#include <QQuickItem>

class SurfaceWrapper;
class SurfaceContainer : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit SurfaceContainer(QQuickItem *parent = nullptr);
    explicit SurfaceContainer(SurfaceContainer *parent);
    ~SurfaceContainer() override;

    SurfaceContainer *parentContainer() const;
    QList<SurfaceContainer*> subContainers() const;

    virtual void addSurface(SurfaceWrapper *surface);
    virtual void removeSurface(SurfaceWrapper *surface);

    const QList<SurfaceWrapper*> &surfaces() const {
        return m_surfaces;
    }

signals:
    void surfaceAdded(SurfaceWrapper *surface);
    void surfaceRemoved(SurfaceWrapper *surface);

protected:
    bool doAddSurface(SurfaceWrapper *surface, bool setContainer);
    bool doRemoveSurface(SurfaceWrapper *surface, bool setContainer);

    virtual void addBySubContainer(SurfaceContainer *sub, SurfaceWrapper *surface);
    virtual void removeBySubContainer(SurfaceContainer *sub, SurfaceWrapper *surface);

    QList<SurfaceWrapper*> m_surfaces;
};
