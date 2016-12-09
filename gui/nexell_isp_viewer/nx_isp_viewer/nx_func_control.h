#ifndef NX_FUNC_CONTROL_H
#define NX_FUNC_CONTROL_H

#include "qthread.h"
#include "nx_buf_control.h"

class NxFuncControl : public QObject
{
    Q_OBJECT

public:
    NxFuncControl();

protected:
    /* values */
    struct nx_device *dev;
    QThread *dqthread;
    NxBufControl *bufcontrol;
    void initDevValues(struct nx_device *dev, int width, int height);
    void deinitBuf(struct nx_device *device);
    int initBuf(struct nx_device *device);

signals:
    /* a signal to be sent from func control thread to main window(UI) */
    void uiNeedUpdate(int cmd, int ret, void* data);
    void bufIsDone(unsigned char *buf);
    void stopPreview();

private slots:
    /* event handler to deal with the cmd from ui */
    void recvCommand(int cmd, void* data);
    void callDisplay(unsigned char *dst);
private:
    void cmdProcessing(int cmd, void *data);
    int openDevice(void *data);
    int closeDevice();
    int exit();
    int previewStart();
    int previewStop();
    int controlReg(struct nx_reg_control *reg);
    int loadRegSet(int type, char *data);
};

#endif // NX_FUNC_CONTROL_H
