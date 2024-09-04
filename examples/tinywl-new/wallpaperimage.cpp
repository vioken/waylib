// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wallpaperprovider.h"
#include "wallpaperimage.h"
#include "helper.h"
#include "output.h"
#include "woutput.h"
#include "workspace.h"

#include <QDir>

WallpaperImage::WallpaperImage(QQuickItem *parent)
    : QQuickImage(parent)
{
    auto provider = Helper::instance()->qmlEngine()->wallpaperImageProvider();
    connect(provider, &WallpaperImageProvider::wallpaperTextureUpdate, this, &WallpaperImage::updateWallpaperTexture);
}

WallpaperImage::~WallpaperImage()
{

}

int WallpaperImage::userId()
{
    return m_userId;
}

void WallpaperImage::setUserId(const int id)
{
    if (m_userId != id) {
        m_userId = id;
        Q_EMIT userIdChanged();
        updateSource();
    }
}

int WallpaperImage::workspace()
{
    return m_workspaceId;
}

void WallpaperImage::setWorkspace(const int id)
{
    if (m_workspaceId != id) {
        m_workspaceId = id;
        Q_EMIT workspaceChanged();
        updateSource();
    }
}

WOutput* WallpaperImage::output()
{
    return m_output;
}

void WallpaperImage::setOutput(WOutput* output)
{
    if (m_output != output) {
        m_output = output;
        Q_EMIT outputChanged();
        updateSource();
    }
}

void WallpaperImage::updateSource()
{
    if (m_userId == -1 ||
        !m_output ||
        m_workspaceId == -1) {
        return;
    }

    QStringList paras;
    paras << QString::number(m_userId) << m_output->name() << QString::number(m_workspaceId);
    QString source = "image://wallpaper/" + paras.join("/");
    setSource(source);
}

void WallpaperImage::updateWallpaperTexture(const QString& id, int size)
{
    QString item_id = source().toString().remove("image://wallpaper/");
    int item_size = sourceSize().width() * sourceSize().height();
    if (item_size < size && item_id == id) {
        load();
        update();
    }
}
