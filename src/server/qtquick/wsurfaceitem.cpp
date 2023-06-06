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
#include "wsurfaceitem.h"
#include "wsurface.h"
#include "wtexture.h"
#include "woutput.h"
#include "wseat.h"
#include "wcursor.h"
#include "wthreadutils.h"
#include "wquicksurface.h"

#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <private/qquickitem_p.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

#define STATE_MOVE QStringLiteral("move")
#define STATE_RESIZE QStringLiteral("resize")

class WSGTextureProvider : public QSGTextureProvider
{
public:
    WSGTextureProvider(WSurfaceItemPrivate *item);

public:
    QSGTexture *texture() const override;
    void updateTexture(); // in render thread

private:
    WSurfaceItemPrivate *item;
    QScopedPointer<WTexture> dwtexture;
};

class WSurfaceItemPrivate : public QQuickItemPrivate
{
    friend class WSGTextureProvider;
public:
    WSurfaceItemPrivate()
        : textureProvider(new WSGTextureProvider(this))
    {

    }
    ~WSurfaceItemPrivate() {
        if (textureProvider)
            delete textureProvider;
    }

    // TODO: Don't using WOutput here, wapper by WSurfaceBridge
    inline WOutput *output() const {
        return WOutput::fromScreen(QQuickWindowPrivate::get(window)->topLevelScreen);
    }
    inline qreal scaleFromSurface() const {
        Q_Q(const WSurfaceItem);
        return surface->surface()->size().width() / q->width();
    }

    qreal getImplicitWidth() const override {
        return surface ? surface->size().width() : 0;
    }
    qreal getImplicitHeight() const override {
        return surface ? surface->size().height() : 0;
    }

    void updateImplicitSize() {
        implicitWidthChanged();
        implicitHeightChanged();

        Q_Q(WSurfaceItem);
        if (!widthValid())
            q->resetWidth();
        if (!heightValid())
            q->resetHeight();
    }

    void initForSurface();

    Q_DECLARE_PUBLIC(WSurfaceItem)
    QPointer<WQuickSurface> surface;
    WSGTextureProvider *textureProvider = nullptr;
};

WSGTextureProvider::WSGTextureProvider(WSurfaceItemPrivate *item)
    : item(item)
{
    dwtexture.reset(new WTexture(nullptr));
}

QSGTexture *WSGTextureProvider::texture() const
{
    return dwtexture->getSGTexture(item->window);
}

void WSGTextureProvider::updateTexture()
{
    dwtexture->setHandle(item->surface->texture());
    Q_EMIT textureChanged();
    QMetaObject::invokeMethod(item->q_func(), &QQuickItem::update, Qt::QueuedConnection);
}

WSurfaceItem::WSurfaceItem(QQuickItem *parent)
    : QQuickItem(*new WSurfaceItemPrivate(), parent)
{
    setFlag(QQuickItem::ItemHasContents);
    setFlag(QQuickItem::ItemClipsChildrenToShape);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

bool WSurfaceItem::isTextureProvider() const
{
    return true;
}

QSGTextureProvider *WSurfaceItem::textureProvider() const
{
    Q_D(const WSurfaceItem);
    return d->textureProvider;
}

bool WSurfaceItem::contains(const QPointF &localPos) const
{
    Q_D(const WSurfaceItem);

    if (Q_UNLIKELY(!d->surface))
        return false;

    // TODO: Can't using WSurface here, wapper by WSurfaceBridge
    return d->surface->surface()->inputRegionContains(d->scaleFromSurface(), mapToGlobal(localPos));
}

WQuickSurface *WSurfaceItem::surface() const
{
    Q_D(const WSurfaceItem);
    return d->surface.get();
}

void WSurfaceItem::setSurface(WQuickSurface *surface)
{
    Q_D(WSurfaceItem);
    if (d->surface == surface || (d->componentComplete && d->surface))
        return;

    Q_ASSERT(surface);
    d->surface = surface;
    d->initForSurface();
    update();

    Q_EMIT surfaceChanged();
}

QSGNode *WSurfaceItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    Q_D(WSurfaceItem);

    if (!d->surface || width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    auto node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (Q_UNLIKELY(!node)) {
        node = new QSGSimpleTextureNode;
        node->setOwnsTexture(false);
        node->setTexture(d->textureProvider->texture());
    } else {
        node->markDirty(QSGNode::DirtyMaterial);
    }

    const QRectF textureGeometry(QPointF(0, 0), d->surface->surface()->bufferSize());
    node->setSourceRect(textureGeometry);

    const qreal scale = window()->effectiveDevicePixelRatio();
    const qreal scale_surface = d->scaleFromSurface();
    const QRectF targetGeometry(d->surface->surface()->toEffectivePos(scale, d->surface->surface()->textureOffset()),
                                textureGeometry.size() / d->surface->surface()->scale() / scale_surface);
    node->setRect(targetGeometry);
    node->setFiltering(QSGTexture::Linear);

    d->surface->surface()->notifyFrameDone();

    return node;
}

void WSurfaceItem::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(WSurfaceItem);

    auto we = d->output()->currentInputEvent();
    if (Q_UNLIKELY(!we))
        return;

    event->accept();
    auto seat = we->data->seat;
    seat->server()->threadUtil()->run(this, seat, &WSeat::notifyEnterSurface, d->surface->surface(), we);
}

void WSurfaceItem::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(WSurfaceItem);

    auto we = d->output()->currentInputEvent();
    if (Q_UNLIKELY(!we))
        return;

    auto seat = we->data->seat;

    if (seat->hoverSurface() != d->surface->surface()) {
        return QQuickItem::hoverLeaveEvent(event);
    }

    event->accept();
    seat->server()->threadUtil()->run(this, seat, &WSeat::notifyLeaveSurface, d->surface->surface(), we);
}

void WSurfaceItem::mousePressEvent(QMouseEvent *event)
{
    event->accept();
}

void WSurfaceItem::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(WSurfaceItem);

    auto we = d->output()->currentInputEvent();
    if (we)
        Q_EMIT mouseRelease(we->data->seat);

    QQuickItem::mouseReleaseEvent(event);
}

void WSurfaceItem::releaseResources()
{
    Q_D(WSurfaceItem);

    if (d->textureProvider) {
        class WSurfaceCleanupJob : public QRunnable
        {
        public:
            WSurfaceCleanupJob(QObject *object) : m_object(object) { }
            void run() override {
                delete m_object;
            }
            QObject *m_object;
        };

        // Delay clean the textures on the next render after.
        window()->scheduleRenderJob(new WSurfaceCleanupJob(d->textureProvider),
                                    QQuickWindow::AfterRenderingStage);
        d->textureProvider = nullptr;
    }

    d->surface = nullptr;
    // Force to update the contents, avoid to render the invalid textures
    d->dirty(QQuickItemPrivate::Content);
    QQuickItem::releaseResources();
}

void WSurfaceItem::componentComplete()
{
    Q_D(WSurfaceItem);

    if (d->surface) {
        d->initForSurface();
    }

    QQuickItem::componentComplete();
}

void WSurfaceItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(WSurfaceItem);

    if (d->surface) {
        d->surface->setSize(newGeometry.size());
    }

    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

void WSurfaceItem::invalidateSceneGraph()
{
    Q_D(WSurfaceItem);
    if (d->textureProvider)
        delete d->textureProvider;
    d->textureProvider = nullptr;
    d->surface = nullptr;
}

void WSurfaceItemPrivate::initForSurface()
{
    Q_Q(WSurfaceItem);

    // in render thread
    surface->surface()->server()->threadUtil()->run(q, textureProvider,
                                                    &WSGTextureProvider::updateTexture);

    // in render thread
    QObject::connect(surface->surface(), &WSurface::textureChanged,
                     textureProvider, &WSGTextureProvider::updateTexture,
                     Qt::DirectConnection);
    QObject::connect(surface->surface(), &WSurface::destroyed, q,
                     &WSurfaceItem::releaseResources, Qt::DirectConnection);
    QObject::connect(surface, &WQuickSurface::sizeChanged, q, [this] {
        updateImplicitSize();
    });

    if (widthValid() && heightValid()) {
        surface->setSize(q->size());
    } else {
        updateImplicitSize();
    }
}

WAYLIB_SERVER_END_NAMESPACE

#include "moc_wsurfaceitem.cpp"
