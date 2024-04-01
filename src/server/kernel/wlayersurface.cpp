// Copyright (C) 2023 rewine <luhongxu@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wlayersurface.h"
#include "private/wsurface_p.h"
#include "wseat.h"
#include "wtools.h"
#include "wsurface.h"
#include "woutput.h"

#include <qwlayershellv1.h>
#include <qwseat.h>
#include <qwcompositor.h>
#include <qwoutput.h>

#include <QDebug>

extern "C" {
// avoid replace namespace
#include <math.h>
#define namespace scope
#define static
#include <wlr/types/wlr_layer_shell_v1.h>
#undef namespace
#undef static
}

QW_USE_NAMESPACE
WAYLIB_SERVER_BEGIN_NAMESPACE

class Q_DECL_HIDDEN WLayerSurfacePrivate : public WObjectPrivate {
public:
    WLayerSurfacePrivate(WLayerSurface *qq, QWLayerSurfaceV1 *handle);
    ~WLayerSurfacePrivate();

    inline wlr_layer_surface_v1 *nativeHandle() const {
        Q_ASSERT(handle);
        return handle->handle();
    }

    wl_client *waylandClient() const {
        return nativeHandle()->resource->client;
    }

    // begin slot function
    void updateLayerProperty();
    // end slot function

    void init();
    void connect();
    void updatePosition();

    bool setDesiredSize(QSize newSize);
    bool setLayer(WLayerSurface::LayerType layer);
    bool setAncher(WLayerSurface::AnchorTypes ancher);
    bool setExclusiveZone(int32_t zone);
    bool setLeftMargin(int32_t margin);
    bool setRightMargin(int32_t margin);
    bool setTopMargin(int32_t margin);
    bool setBottomMargin(int32_t margin);
    bool setKeyboardInteractivity(WLayerSurface::KeyboardInteractivity interactivity);

    W_DECLARE_PUBLIC(WLayerSurface)

    QPointer<QWLayerSurfaceV1> handle;
    WSurface *surface = nullptr;
    uint activated:1;
    QSize desiredSize;
    WLayerSurface::LayerType layer = WLayerSurface::LayerType::Bottom;
    WLayerSurface::AnchorTypes ancher =  WLayerSurface::AnchorType::None;
    int32_t leftMargin = 0, rightMargin = 0;
    int32_t topMargin = 0, bottomMargin = 0;
    int32_t exclusiveZone = 0;
    WLayerSurface::KeyboardInteractivity keyboardInteractivity = WLayerSurface::KeyboardInteractivity::None;
    WOutput *output = nullptr;
};

WLayerSurfacePrivate::WLayerSurfacePrivate(WLayerSurface *qq, QWLayerSurfaceV1 *hh)
    : WObjectPrivate(qq)
    , handle(hh)
    , activated(false)
{

}

WLayerSurfacePrivate::~WLayerSurfacePrivate()
{
    if (handle)
        handle->setData(this, nullptr);
    surface->removeAttachedData<WLayerSurface>();
}

void WLayerSurfacePrivate::init()
{
    W_Q(WLayerSurface);
    handle->setData(this, q);

    Q_ASSERT(!q->surface());
    surface = new WSurface(handle->surface(), q);
    surface->setAttachedData<WLayerSurface>(q);
    updateLayerProperty();
    output = nativeHandle()->output ? WOutput::fromHandle(QWOutput::from(nativeHandle()->output)) : nullptr;

    connect();
}

void WLayerSurfacePrivate::connect()
{
    W_Q(WLayerSurface);
    // TODO(@rewine): Support popup surface
    //QObject::connect(handle, &QWLayerSurfaceV1::newPopup, q, [this] (QWXdgPopup *popup) {});

    QObject::connect(handle->surface(), &QWSurface::commit, q, [this] () {
        updateLayerProperty();
    });
}

static inline void debugOutput(wlr_layer_surface_v1_state s)
{
    qDebug() << "committed: " << s.committed << " "
             << "configure_serial: " << s.configure_serial << "\n"
             << "anchor: " << s.anchor << " "
             << "layer: " << s.layer << " "
             << "exclusive_zone: " << s.exclusive_zone << " "
             << "keyboard_interactive: " << s.keyboard_interactive << "\n"
             << "margin: " << s.margin.left << s.margin.right << s.margin.top << s.margin.bottom << "\n"
             << "desired_width/height: " << s.desired_width << " " << s.desired_height << "\n"
             << "actual_width/height" << s.actual_width << " " << s.actual_height << "\n";
}

void WLayerSurfacePrivate::updateLayerProperty()
{
    W_Q(WLayerSurface);

    wlr_layer_surface_v1_state state = q->nativeHandle()->current;

    int hasChange = false;
    hasChange |= setDesiredSize(QSize(state.desired_width, state.desired_height));
    hasChange |= setLayer(static_cast<WLayerSurface::LayerType>(state.layer));
    hasChange |= setAncher(WLayerSurface::AnchorTypes::fromInt(state.anchor));
    hasChange |= setExclusiveZone(state.exclusive_zone);
    hasChange |= setLeftMargin(state.margin.left);
    hasChange |= setRightMargin(state.margin.right);
    hasChange |= setTopMargin(state.margin.top);
    hasChange |= setBottomMargin(state.margin.bottom);
    hasChange |= setKeyboardInteractivity(static_cast<WLayerSurface::KeyboardInteractivity>(state.keyboard_interactive));

    if (hasChange)
        Q_EMIT q->layerPropertiesChanged();

}

// begin set property

bool WLayerSurfacePrivate::setDesiredSize(QSize newSize)
{
    W_Q(WLayerSurface);
    if (desiredSize == newSize)
        return false;
    desiredSize = newSize;
    Q_EMIT q->desiredSizeChanged();
    return true;
}


bool WLayerSurfacePrivate::setLayer(WLayerSurface::LayerType layer)
{
    W_Q(WLayerSurface);
    if (this->layer == layer)
        return false;
    this->layer = layer;
    Q_EMIT q->layerChanged();
    return true;
}

bool WLayerSurfacePrivate::setAncher(WLayerSurface::AnchorTypes ancher)
{
    W_Q(WLayerSurface);
    if (this->ancher == ancher)
        return false;
    this->ancher = ancher;
    Q_EMIT q->ancherChanged();
    return true;
}

bool WLayerSurfacePrivate::setExclusiveZone(int32_t zone)
{
    W_Q(WLayerSurface);
    if (exclusiveZone == zone)
        return false;
    exclusiveZone = zone;
    Q_EMIT q->exclusiveZoneChanged();
    return true;
}

bool WLayerSurfacePrivate::setLeftMargin(int32_t margin)
{
    W_Q(WLayerSurface);
    if (leftMargin == margin)
        return false;
    leftMargin = margin;
    Q_EMIT q->leftMarginChanged();
    return true;
}

bool WLayerSurfacePrivate::setRightMargin(int32_t margin)
{
    W_Q(WLayerSurface);
    if (rightMargin == margin)
        return false;
    rightMargin = margin;
    Q_EMIT q->rightMarginChanged();
    return true;
}

bool WLayerSurfacePrivate::setTopMargin(int32_t margin)
{
    W_Q(WLayerSurface);
    if (topMargin == margin)
        return false;
    topMargin = margin;
    Q_EMIT q->topMarginChanged();
    return true;
}

bool WLayerSurfacePrivate::setBottomMargin(int32_t margin)
{
    W_Q(WLayerSurface);
    if (bottomMargin == margin)
        return false;
    bottomMargin = margin;
    Q_EMIT q->bottomMarginChanged();
    return true;
}

bool WLayerSurfacePrivate::setKeyboardInteractivity(WLayerSurface::KeyboardInteractivity interactivity)
{
    W_Q(WLayerSurface);
    if (keyboardInteractivity == interactivity)
        return false;
    keyboardInteractivity = interactivity;
    Q_EMIT q->keyboardInteractivityChanged();
    return true;
}

// end set property

WLayerSurface::WLayerSurface(QWLayerSurfaceV1 *handle, QObject *parent)
    : WToplevelSurface(parent)
    , WObject(*new WLayerSurfacePrivate(this, handle))
{
    d_func()->init();
}

WLayerSurface::~WLayerSurface()
{

}

bool WLayerSurface::isPopup() const
{
    return false;
}

bool WLayerSurface::doesNotAcceptFocus() const
{
    W_DC(WLayerSurface);
    if (d->keyboardInteractivity == WLayerSurface::KeyboardInteractivity::None)
        return true;
    return false;
}

bool WLayerSurface::isActivated() const
{
    W_D(const WLayerSurface);
    return d->activated;
}

WSurface *WLayerSurface::surface() const
{
    W_D(const WLayerSurface);
    return d->surface;
}

QWLayerSurfaceV1 *WLayerSurface::handle() const
{
    W_DC(WLayerSurface);
    return d->handle;
}

wlr_layer_surface_v1 *WLayerSurface::nativeHandle() const
{
    W_D(const WLayerSurface);
    return d->handle->handle();
}

QWSurface *WLayerSurface::inputTargetAt(QPointF &localPos) const
{
    W_DC(WLayerSurface);
    // find a wlr_suface object who can receive the events
    const QPointF pos = localPos;
    auto sur = d->handle->surfaceAt(pos, &localPos);

    return sur;
}

WLayerSurface *WLayerSurface::fromHandle(QWLayerSurfaceV1 *handle)
{
    return handle->getData<WLayerSurface>();
}

WLayerSurface *WLayerSurface::fromSurface(WSurface *surface)
{
    return surface->getAttachedData<WLayerSurface>();
}

QRect WLayerSurface::getContentGeometry() const
{
    W_DC(WLayerSurface);
    return QRect(0, 0, d->nativeHandle()->current.actual_width, d->nativeHandle()->current.actual_height);
}

int WLayerSurface::keyboardFocusPriority() const
{
    W_DC(WLayerSurface);

    if (d->keyboardInteractivity == KeyboardInteractivity::None)
        return -1;

    if (d->keyboardInteractivity == KeyboardInteractivity::Exclusive) {
        if (d->layer == LayerType::Overlay)
            return 2;
        if (d->layer == LayerType::Top)
            return 1;
        // For the bottom and background layers
        // the compositor is allowed to use normal focus semantics
    }

    return 0;
}

QSize WLayerSurface::desiredSize() const
{
    W_DC(WLayerSurface);
    return d->desiredSize;
}

WLayerSurface::LayerType WLayerSurface::layer() const
{
    W_DC(WLayerSurface);
    return d->layer;
}

WLayerSurface::AnchorTypes WLayerSurface::ancher() const
{
    W_DC(WLayerSurface);
    return d->ancher;
}

int32_t WLayerSurface::leftMargin() const
{
    W_DC(WLayerSurface);
    return d->leftMargin;
}

int32_t WLayerSurface::rightMargin() const
{
    W_DC(WLayerSurface);
    return d->rightMargin;
}

int32_t WLayerSurface::topMargin() const
{
    W_DC(WLayerSurface);
    return d->topMargin;
}

int32_t WLayerSurface::bottomMargin() const
{
    W_DC(WLayerSurface);
    return d->bottomMargin;
}

int32_t WLayerSurface::exclusiveZone() const
{
    W_DC(WLayerSurface);
    return d->exclusiveZone;
}

WLayerSurface::KeyboardInteractivity WLayerSurface::keyboardInteractivity() const
{
    W_DC(WLayerSurface);
    return d->keyboardInteractivity;
}

WOutput *WLayerSurface::output() const
{
    W_DC(WLayerSurface);
    return d->output;
}

WLayerSurface::AnchorType WLayerSurface::getExclusiveZoneEdge() const
{
    W_DC(WLayerSurface);
    using enum WLayerSurface::AnchorType;

    auto ancher = d->ancher;
    if (ancher == Top || ancher == (Top|Left|Right))
        return Top;
    if (ancher == Bottom || ancher == (Bottom|Left|Right))
        return Bottom;
    if (ancher == Left || ancher == (Left|Top|Bottom))
        return Left;
    if (ancher == Right || ancher == (Right|Top|Bottom))
        return Right;
    return None;
}

uint32_t WLayerSurface::configureSize(const QSize &newSize)
{
    return handle()->configure(newSize.width(), newSize.height());
}

void WLayerSurface::closed()
{
    // 1. Notify the client that the surface has been closed
    // 2. Destroy the struct wlr_layer_surface_v1
    wlr_layer_surface_v1_destroy(nativeHandle());
}

void WLayerSurface::setActivate(bool on)
{
    W_D(WLayerSurface);
    if (d->activated != on) {
        d->activated = on;
        Q_EMIT activateChanged();
    }
}

bool WLayerSurface::checkNewSize(const QSize &size)
{
    W_D(WLayerSurface);

    if (size.width() <= 0 || size.height() <= 0) {
        return false;
    }
    return true;
}

WAYLIB_SERVER_END_NAMESPACE
