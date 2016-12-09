#ifndef NX_REGEDIT_DIALOG_H
#define NX_REGEDIT_DIALOG_H
#include "nx_common_enum.h"
#include <QDialog>

namespace Ui {
class NxRegEditDialog;
}

class NxRegEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NxRegEditDialog(QWidget *parent = 0);
    ~NxRegEditDialog();
    void setType(nx_reg_type reg_t);

private:
    Ui::NxRegEditDialog *ui;
    void closeEvent(QCloseEvent *);
    void initValues();
    void setText();
    void getText();
    void writeRegister(unsigned int addr, unsigned int *data);
    unsigned int    reg[16][4];
    nx_reg_type type;

signals:
    void controlReg(struct nx_reg_control *control);

private slots:
    void updateReg(struct nx_reg_control *control);
    void on_readButton_clicked();
    void on_writeButton_clicked();
    void keyPressEvent(QKeyEvent *event);
    void on_reg0_0_returnPressed();
    void on_reg0_4_returnPressed();
    void on_reg0_8_returnPressed();
    void on_reg0_c_returnPressed();
    void on_reg1_0_returnPressed();
    void on_reg1_4_returnPressed();
    void on_reg1_8_returnPressed();
    void on_reg1_c_returnPressed();
    void on_reg2_0_returnPressed();
    void on_reg2_4_returnPressed();
    void on_reg2_8_returnPressed();
    void on_reg2_c_returnPressed();
    void on_reg3_0_returnPressed();
    void on_reg3_4_returnPressed();
    void on_reg3_8_returnPressed();
    void on_reg3_c_returnPressed();
    void on_reg4_0_returnPressed();
    void on_reg4_4_returnPressed();
    void on_reg4_8_returnPressed();
    void on_reg4_c_returnPressed();
    void on_reg5_0_returnPressed();
    void on_reg5_4_returnPressed();
    void on_reg5_8_returnPressed();
    void on_reg5_c_returnPressed();
    void on_reg6_0_returnPressed();
    void on_reg6_4_returnPressed();
    void on_reg6_8_returnPressed();
    void on_reg6_c_returnPressed();
    void on_reg7_0_returnPressed();
    void on_reg7_4_returnPressed();
    void on_reg7_8_returnPressed();
    void on_reg7_c_returnPressed();
    void on_reg8_0_returnPressed();
    void on_reg8_4_returnPressed();
    void on_reg8_8_returnPressed();
    void on_reg8_c_returnPressed();
    void on_reg9_0_returnPressed();
    void on_reg9_4_returnPressed();
    void on_reg9_8_returnPressed();
    void on_reg9_c_returnPressed();
    void on_rega_0_returnPressed();
    void on_rega_4_returnPressed();
    void on_rega_8_returnPressed();
    void on_rega_c_returnPressed();
    void on_regb_0_returnPressed();
    void on_regb_4_returnPressed();
    void on_regb_8_returnPressed();
    void on_regb_c_returnPressed();
    void on_regc_0_returnPressed();
    void on_regc_4_returnPressed();
    void on_regc_8_returnPressed();
    void on_regc_c_returnPressed();
    void on_regd_0_returnPressed();
    void on_regd_4_returnPressed();
    void on_regd_8_returnPressed();
    void on_regd_c_returnPressed();
    void on_rege_0_returnPressed();
    void on_rege_4_returnPressed();
    void on_rege_8_returnPressed();
    void on_rege_c_returnPressed();
    void on_regf_0_returnPressed();
    void on_regf_4_returnPressed();
    void on_regf_8_returnPressed();
    void on_regf_c_returnPressed();
};

#endif // NX_REGEDIT_DIALOG_H
