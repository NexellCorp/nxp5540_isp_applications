#ifndef NX_BUF_CONTROL_H
#define NX_BUF_CONTROL_H

#include "qobject.h"

class NxBufControl : public QObject
{
    Q_OBJECT
public:
    NxBufControl(struct nx_device *device);

signals:
    void bufIsReady(unsigned char*);

public slots:
    void stop();
    void run();

protected:
    struct nx_device *dev;
    bool stopFlag;
};

#endif // NX_BUF_CONTROL_H
