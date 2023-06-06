// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "woutputwindowitem_p.h"
#include "woutputwindow.h"
#include "woutput.h"

#include <private/qquickitem_p.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

class WOutputWindowItemPrivate : public QQuickItemPrivate
{
public:
    WOutputWindowItemPrivate()
    {

    }
    ~WOutputWindowItemPrivate() {

    }

    qreal getImplicitWidth() const override {
        auto output = window->output();
        return output ? output->effectiveSize().width() : 0;
    }
    qreal getImplicitHeight() const override {
        auto output = window->output();
        return output ? output->effectiveSize().height() : 0;
    }

    QQmlListProperty<QObject> data() {
        return QQuickWindowPrivate::get(window)->data();
    }

    void updateImplicitSize() {
        implicitWidthChanged();
        implicitHeightChanged();

        Q_Q(WOutputWindowItem);
        if (!widthValid())
            q->resetWidth();
        if (!heightValid())
            q->resetHeight();
    }

    void onOutputChanged(WOutput *newOutput) {
        if (output == newOutput)
            return;

        Q_Q(WOutputWindowItem);
        if (output)
            output->disconnect(q);

        output = newOutput;
        if (newOutput) {
            q->connect(newOutput, &WOutput::effectiveSizeChanged, q, [this] {
                updateImplicitSize();
            });
        }
    }

    Q_DECLARE_PUBLIC(WOutputWindowItem)
    WOutputWindow *window = nullptr;
    WOutput *output = nullptr;
};

WOutputWindowItem::WOutputWindowItem(QQuickItem *parent)
    : QQuickItem(*new WOutputWindowItemPrivate(), parent)
{

}

WOutputWindow *WOutputWindowItem::window() const
{
    Q_D(const WOutputWindowItem);
    return d->window;
}

void WOutputWindowItem::classBegin()
{
    Q_D(WOutputWindowItem);
    d->window = new WOutputWindow();

    connect(d->window, &WOutputWindow::outputChanged, this, [d, this] {
        d->onOutputChanged(d->window->output());
        Q_EMIT outputChanged();
    });

    QQuickItem::classBegin();
}

void WOutputWindowItem::componentComplete()
{
    Q_D(WOutputWindowItem);

    if (d->widthValid() && d->heightValid()) {
        d->window->setGeometry(QRect(position().toPoint(), size().toSize()));
    } else {
        d->window->setPosition(position().toPoint());
        d->updateImplicitSize();
    }

    QQuickItem::componentComplete();
}

void WOutputWindowItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(const WOutputWindowItem);

    if (d->window)
        d->window->setGeometry(newGeometry.toRect());

    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

WAYLIB_SERVER_END_NAMESPACE

#include "moc_woutputwindowitem_p.cpp"
