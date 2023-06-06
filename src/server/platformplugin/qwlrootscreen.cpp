// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwlrootscreen.h"
#include "qwlrootscursor.h"
#include "qwlrootsintegration.h"
#include "woutput.h"
#include "woutputlayout.h"

#include <qwoutput.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>

extern "C" {
#include <wlr/types/wlr_output.h>
}

#include <drm_fourcc.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

QWlrootsScreen::QWlrootsScreen(WOutput *output)
    : m_output(output)
{

}

WOutput *QWlrootsScreen::output() const
{
    return m_output.get();
}

QRect QWlrootsScreen::geometry() const
{
    return QRect(m_output->position(), m_output->transformedSize());
}

void QWlrootsScreen::move(const QPoint &pos)
{
    auto layout = m_output->layout();
    if (layout)
        layout->move(m_output, pos);
}

int QWlrootsScreen::depth() const
{
    return QImage::toPixelFormat(format()).bitsPerPixel();
}

QImage::Format QWlrootsScreen::format() const
{
    if (m_format == QImage::Format_Invalid)
        m_format = getFormat();
    return m_format;
}

QSizeF QWlrootsScreen::physicalSize() const
{
    return QSizeF(handle()->phys_width, handle()->phys_height);
}

qreal QWlrootsScreen::devicePixelRatio() const
{
    return 1.0;
}

qreal QWlrootsScreen::refreshRate() const
{
    if (!handle()->current_mode)
        return 60;
    return handle()->current_mode->refresh;
}

QDpi QWlrootsScreen::logicalBaseDpi() const
{
    return QDpi(96, 96);
}

QDpi QWlrootsScreen::logicalDpi() const
{
    auto dpi = logicalBaseDpi();
    const qreal scale = handle()->scale;

    return QDpi(dpi.first * scale, dpi.second * scale);
}

Qt::ScreenOrientation QWlrootsScreen::nativeOrientation() const
{
    return handle()->phys_width > handle()->phys_height ?
               Qt::LandscapeOrientation : Qt::PortraitOrientation;
}

Qt::ScreenOrientation QWlrootsScreen::orientation() const
{
    bool isPortrait = nativeOrientation() == Qt::PortraitOrientation;
    switch (handle()->transform) {
    case WL_OUTPUT_TRANSFORM_NORMAL:
        return isPortrait ? Qt::PortraitOrientation : Qt::LandscapeOrientation;;
    case WL_OUTPUT_TRANSFORM_90:
        return isPortrait ? Qt::InvertedLandscapeOrientation : Qt::PortraitOrientation;
    case WL_OUTPUT_TRANSFORM_180:
        return isPortrait ? Qt::InvertedPortraitOrientation : Qt::InvertedLandscapeOrientation;
    case WL_OUTPUT_TRANSFORM_270:
        return isPortrait ? Qt::LandscapeOrientation : Qt::InvertedPortraitOrientation;
    default:
        break;
    }

    return Qt::PrimaryOrientation;
}

QWindow *QWlrootsScreen::topLevelAt(const QPoint &) const
{
    return nullptr;
}

QList<QPlatformScreen *> QWlrootsScreen::virtualSiblings() const
{
    QList<QPlatformScreen*> siblings;
    for (auto s : QWlrootsIntegration::instance()->m_screens) {
        if (s != this)
            siblings.append(s);
    }

    return siblings;
}

QString QWlrootsScreen::name() const
{
    return QString::fromUtf8(handle()->name);
}

QString QWlrootsScreen::manufacturer() const
{
    return QString::fromUtf8(handle()->make);
}

QString QWlrootsScreen::model() const
{
    return QString::fromUtf8(handle()->model);
}

QString QWlrootsScreen::serialNumber() const
{
    return QString::fromUtf8(handle()->serial);
}

QPlatformCursor *QWlrootsScreen::cursor() const
{
    if (!m_cursor)
        m_cursor.reset(new QWlrootsCursor());
    return m_cursor.get();
}

QPlatformScreen::SubpixelAntialiasingType QWlrootsScreen::subpixelAntialiasingTypeHint() const
{
    switch (handle()->subpixel) {
    case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
        return Subpixel_RGB;
    case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
        return Subpixel_BGR;
    case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
        return Subpixel_VRGB;
    case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
        return Subpixel_VBGR;
    default:
        break;
    }

    return Subpixel_None;
}

QPlatformScreen::PowerState QWlrootsScreen::powerState() const
{
    return handle()->enabled ? PowerStateOn : PowerStateOff;
}

void QWlrootsScreen::setPowerState(PowerState)
{

}

QVector<QPlatformScreen::Mode> QWlrootsScreen::modes() const
{
    QVector<Mode> modes;
    struct wlr_output_mode *mode;
    wl_list_for_each(mode, &handle()->modes, link) {
        modes << Mode {QSize(mode->width, mode->height), static_cast<qreal>(mode->refresh)};
    }

    return modes;
}

int QWlrootsScreen::currentMode() const
{
    int index = 0;
    struct wlr_output_mode *current = handle()->current_mode;
    struct wlr_output_mode *mode;
    wl_list_for_each(mode, &handle()->modes, link) {
        if (current == mode)
            return index;
        ++index;
    }

    return 0;
}

int QWlrootsScreen::preferredMode() const
{
    int index = 0;
    struct wlr_output_mode *mode;
    wl_list_for_each(mode, &handle()->modes, link) {
        if (mode->preferred)
            return index;
        ++index;
    }

    return 0;
}

void QWlrootsScreen::initialize()
{
    auto updateGeometry = [this] {
        const QRect newGeo = geometry();
        QWindowSystemInterface::handleScreenGeometryChange(screen(), newGeo, newGeo);
    };

    QObject::connect(m_output.get(), &WOutput::transformedSizeChanged, screen(), updateGeometry);

    auto updateDpi = [this] {
        const auto dpi = logicalDpi();
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(), dpi.first, dpi.second);
    };

    QObject::connect(m_output.get(), &WOutput::scaleChanged, screen(), updateDpi);
    updateDpi();
}

QImage::Format QWlrootsScreen::getFormat() const
{
    auto f = m_output->nativeInterface<QWOutput>()->preferredReadFormat();

    switch (f) {
    case DRM_FORMAT_C8:
        return QImage::Format_Indexed8;
    case DRM_FORMAT_XRGB4444:
        return QImage::Format_RGB444;
    case DRM_FORMAT_ARGB4444:
        return QImage::Format_ARGB4444_Premultiplied;
    case DRM_FORMAT_XRGB1555:
        return QImage::Format_RGB555;
    case DRM_FORMAT_ARGB1555:
        return QImage::Format_ARGB8555_Premultiplied;
    case DRM_FORMAT_RGB565:
        return QImage::Format_RGB16;
    case DRM_FORMAT_RGB888:
        return QImage::Format_RGB888;
    case DRM_FORMAT_BGR888:
        return QImage::Format_BGR888;
    case DRM_FORMAT_XRGB8888:
        return QImage::Format_RGB32;
    case DRM_FORMAT_RGBX8888:
        return QImage::Format_RGBX8888;
    case DRM_FORMAT_ARGB8888:
        return QImage::Format_ARGB32_Premultiplied;
    case DRM_FORMAT_RGBA8888:
        return QImage::Format_RGBA8888;
    case DRM_FORMAT_XRGB2101010:
        return QImage::Format_RGB30;
    case DRM_FORMAT_BGRX1010102:
        return QImage::Format_BGR30;
    case DRM_FORMAT_ARGB2101010:
        return QImage::Format_A2RGB30_Premultiplied;
    case DRM_FORMAT_BGRA1010102:
        return QImage::Format_A2BGR30_Premultiplied;
    default:
        return QImage::Format_Invalid;
    }
}

wlr_output *QWlrootsScreen::handle() const
{
    return m_output->nativeInterface<QWOutput>()->handle();
}

WAYLIB_SERVER_END_NAMESPACE