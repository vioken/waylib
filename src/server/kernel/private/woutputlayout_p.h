#include "woutputlayout.h"
#include <qwglobal.h>

QW_BEGIN_NAMESPACE
class QWOutputLayout;
QW_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WOutputLayoutPrivate : public WObjectPrivate
{
public:
    WOutputLayoutPrivate(WOutputLayout *qq);
    ~WOutputLayoutPrivate();

    // begin slot function
    void on_change(void*);
    // end slot function

    void connect();

    W_DECLARE_PUBLIC(WOutputLayout)

    QW_NAMESPACE::QWOutputLayout *handle;
    QVector<WOutput*> outputList;
    QVector<WCursor*> cursorList;
};

WAYLIB_SERVER_END_NAMESPACE
