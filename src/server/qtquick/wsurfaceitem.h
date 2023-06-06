/*
 * Copyright (C) 2021 zkyd
 *
 * Author:     zkyd <zkyd@zjide.org>
 *
 * Maintainer: zkyd <zkyd@zjide.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <wglobal.h>
#include <WSurface>

Q_MOC_INCLUDE("wquicksurface.h")

#include <QQuickItem>

QT_BEGIN_NAMESPACE
class QSGTexture;
QT_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WCursor;
class WOutput;
class WQuickSurface;
class WTextureHandle;
class WSurfaceItemPrivate;
class WAYLIB_SERVER_EXPORT WSurfaceItem : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WSurfaceItem)
    Q_PROPERTY(WQuickSurface* surface READ surface WRITE setSurface NOTIFY surfaceChanged REQUIRED)
    QML_NAMED_ELEMENT(SurfaceItem)

public:
    explicit WSurfaceItem(QQuickItem *parent = nullptr);

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

    bool contains(const QPointF &localPos) const override;

    WQuickSurface *surface() const;
    void setSurface(WQuickSurface *newSurface);

Q_SIGNALS:
    void surfaceChanged();
    void mouseRelease(WSeat *seat);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void releaseResources() override;
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private Q_SLOTS:
    void invalidateSceneGraph();
};

WAYLIB_SERVER_END_NAMESPACE

Q_DECLARE_METATYPE(WAYLIB_SERVER_NAMESPACE::WSurfaceItem*)
