#ifndef NX_MAIN_WINDOW_H
#define NX_MAIN_WINDOW_H

#include <QMainWindow>
#include "nx_common_enum.h"
#include "nx_regedit_dialog.h"

namespace Ui {
class NxMainWindow;
}

class NxMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit NxMainWindow(QWidget *parent = 0);
    ~NxMainWindow();

private:
    Ui::NxMainWindow *ui;
    int frame_width;
    int frame_height;
    int frame_size;
    unsigned char *frame_buffer;
    NxRegEditDialog *ispRegEdit;
protected:
    /* values */
    viewer_status viewerStatus;
    /* functions */
    void initValues();
    void createActions();
    void updateMenu();
    int sendCommand(int cmd, void* data);
    int checkStatus(int function, int cur_status);

signals:
    /* a signal to be sent from main window(UI) to func control thread */
    void userEvent(int cmd, void *data);
    void updateIspReg(struct nx_reg_control * control);

private slots:
    /* menu file */
    void openDevice();
    void closeDevice();
    void saveImage();
    void loadSensorRegSet();
    void loadISPRegSet();
    void saveCurrentRegSet();
    void exit();
    /* menu view */
    void previewStart();
    void previewStop();
    /* menu register control */
    void sensorRegControl();
    void ispRegControl();
    /* menu isp */
    void ispBlockOnOff();
    /* event handler to deal with the result of the user event */
    void recvResult(int cmd, int ret, void* data);
    void updateFrame(unsigned char *frame);
    void controlReg(struct nx_reg_control *reg);
};

#endif // NX_MAIN_WINDOW_H
