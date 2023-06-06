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

#include <WServer>
#include <WThreadUtils>
#include <WOutput>
#include <WBackend>
#include <WXCursorManager>
#include <WCursor>
#include <WSeat>
#include <WOutputLayout>
#include <WInputDevice>
#include <WXdgShell>
#include <WSurfaceLayout>
#include <WSurfaceItem>
#include <wquickbackend.h>
#include <wquickxdgshell.h>

#include <QQuickItem>
#include <QQmlComponent>
#include <QQmlContext>
#include <QTimer>
#include <QThread>
#include <QQuickView>
#include <QGuiApplication>
#include <QQuickRenderControl>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QApplication>

WAYLIB_SERVER_USE_NAMESPACE

int main(int argc, char *argv[]) {
//    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

    WServer::initializeQPA(false);
    std::unique_ptr<WServer> server(new WServer());

    QQuickStyle::setStyle("Material");

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts, false);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_UseOpenGLES);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QApplication app(argc, argv);

    QQmlApplicationEngine waylandEngine;
    waylandEngine.rootContext()->setContextProperty("waylandServer", server.get());
    waylandEngine.load(QUrl("qrc:/Main.qml"));

//    WXCursorManager xcursor_manager;
//    WCursor cursor;

    auto seat = server->attach<WSeat>();

//    cursor.setManager(&xcursor_manager);
//    cursor.setOutputLayout(backend->layout());
//    seat->attachCursor(&cursor);

    // TODO: attach to WBackend
    QObject::connect(server.get(), &WServer::inputAdded,
                     [&] (WBackend*, WInputDevice *device) {
        seat->attachInputDevice(device);
    });

    auto future = server->start();
    future.waitForFinished();
    if (!future.isFinished()) {
        qFatal() << "Failed on start wayland server";
    }
    server->initializeProxyQPA(argc, argv, {"wayland"});

    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     server.get(), &WServer::stop, Qt::DirectConnection);

    QQmlApplicationEngine *engine = new QQmlApplicationEngine;
    engine->load(QUrl("qrc:/ClientWindow.qml"));

    return app.exec();
}
