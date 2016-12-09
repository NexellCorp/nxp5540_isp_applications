/* qt */
#include "QCloseEvent"
/* nexell isp viewer */
#include "nx_common_enum.h"
#include "nx_debug.h"
#include "ui_nx_regedit_dialog.h"
#include "nx_regedit_dialog.h"

NxRegEditDialog::NxRegEditDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::NxRegEditDialog)
{
    NxDebug("[%s]", __func__);
    ui->setupUi(this);
    initValues();
    type = nx_reg_type_isp;
}

void NxRegEditDialog::setType(nx_reg_type reg_t)
{
    QString text = NULL;
    if (reg_t == nx_reg_type_sensor)
        text.append("Sensor");
    else
        text.append("Isp");

    type = reg_t;
    text.append(" Register Control Dialog");
    this->setWindowTitle(text);
}

void NxRegEditDialog::setText()
{
    ui->reg0_0->setText(QString::number(reg[0][0], 16));
    ui->reg0_4->setText(QString::number(reg[0][1], 16));
    ui->reg0_8->setText(QString::number(reg[0][2], 16));
    ui->reg0_c->setText(QString::number(reg[0][3], 16));
    ui->reg1_0->setText(QString::number(reg[1][0], 16));
    ui->reg1_4->setText(QString::number(reg[1][1], 16));
    ui->reg1_8->setText(QString::number(reg[1][2], 16));
    ui->reg1_c->setText(QString::number(reg[1][3], 16));
    ui->reg2_0->setText(QString::number(reg[2][0], 16));
    ui->reg2_4->setText(QString::number(reg[2][1], 16));
    ui->reg2_8->setText(QString::number(reg[2][2], 16));
    ui->reg2_c->setText(QString::number(reg[2][3], 16));
    ui->reg3_0->setText(QString::number(reg[3][0], 16));
    ui->reg3_4->setText(QString::number(reg[3][1], 16));
    ui->reg3_8->setText(QString::number(reg[3][2], 16));
    ui->reg3_c->setText(QString::number(reg[3][3], 16));
    ui->reg4_0->setText(QString::number(reg[4][0], 16));
    ui->reg4_4->setText(QString::number(reg[4][1], 16));
    ui->reg4_8->setText(QString::number(reg[4][2], 16));
    ui->reg4_c->setText(QString::number(reg[4][3], 16));
    ui->reg5_0->setText(QString::number(reg[5][0], 16));
    ui->reg5_4->setText(QString::number(reg[5][1], 16));
    ui->reg5_8->setText(QString::number(reg[5][2], 16));
    ui->reg5_c->setText(QString::number(reg[5][3], 16));
    ui->reg6_0->setText(QString::number(reg[6][0], 16));
    ui->reg6_4->setText(QString::number(reg[6][1], 16));
    ui->reg6_8->setText(QString::number(reg[6][2], 16));
    ui->reg6_c->setText(QString::number(reg[6][3], 16));
    ui->reg7_0->setText(QString::number(reg[7][0], 16));
    ui->reg7_4->setText(QString::number(reg[7][1], 16));
    ui->reg7_8->setText(QString::number(reg[7][2], 16));
    ui->reg7_c->setText(QString::number(reg[7][3], 16));
    ui->reg8_0->setText(QString::number(reg[8][0], 16));
    ui->reg8_4->setText(QString::number(reg[8][1], 16));
    ui->reg8_8->setText(QString::number(reg[8][2], 16));
    ui->reg8_c->setText(QString::number(reg[8][3], 16));
    ui->reg9_0->setText(QString::number(reg[9][0], 16));
    ui->reg9_4->setText(QString::number(reg[9][1], 16));
    ui->reg9_8->setText(QString::number(reg[9][2], 16));
    ui->reg9_c->setText(QString::number(reg[9][3], 16));
    ui->rega_0->setText(QString::number(reg[10][0], 16));
    ui->rega_4->setText(QString::number(reg[10][1], 16));
    ui->rega_8->setText(QString::number(reg[10][2], 16));
    ui->rega_c->setText(QString::number(reg[10][3], 16));
    ui->regb_0->setText(QString::number(reg[11][0], 16));
    ui->regb_4->setText(QString::number(reg[11][1], 16));
    ui->regb_8->setText(QString::number(reg[11][2], 16));
    ui->regb_c->setText(QString::number(reg[11][3], 16));
    ui->regc_0->setText(QString::number(reg[12][0], 16));
    ui->regc_8->setText(QString::number(reg[12][2], 16));
    ui->regc_c->setText(QString::number(reg[12][3], 16));
    ui->regd_0->setText(QString::number(reg[13][0], 16));
    ui->regd_4->setText(QString::number(reg[13][1], 16));
    ui->regd_8->setText(QString::number(reg[13][2], 16));
    ui->regd_c->setText(QString::number(reg[13][3], 16));
    ui->rege_0->setText(QString::number(reg[14][0], 16));
    ui->rege_4->setText(QString::number(reg[14][1], 16));
    ui->rege_8->setText(QString::number(reg[14][2], 16));
    ui->rege_c->setText(QString::number(reg[14][3], 16));
    ui->regf_0->setText(QString::number(reg[15][0], 16));
    ui->regf_4->setText(QString::number(reg[15][1], 16));
    ui->regf_8->setText(QString::number(reg[15][2], 16));
    ui->regf_c->setText(QString::number(reg[15][3], 16));
}

void NxRegEditDialog::initValues()
{
    int i, j;
    for(i = 0; i< 16; i++) {
        for (j = 0; j < 4; j++)
            reg[i][j] = 0x00000000;
    }
    ui->lineEdit->setText("00");
    setText();
}

void NxRegEditDialog::closeEvent(QCloseEvent *event)
{
    NxDebug("[%s]", __func__);
    event->accept();
    this->hide();
}

void NxRegEditDialog::updateReg(struct nx_reg_control *control)
{
    int i, j = 0;
    struct nx_reg_control data = *(control);

    NxDebug("[%s] flag is %d ", __func__, data.flag);

    if (data.flag == nx_reg_flag_read) {
        i = (data.addr & 0x0010) >> 1;
        j = (data.addr & 0x0001);
        reg[i][j] = *(unsigned int*)data.data;
        NxDebug("[%d][%d] data = 0x%8x \n", i, j, reg[i][j]);
    } else if (data.flag == nx_reg_flag_read_page) {
        memcpy(reg, data.data, sizeof(reg));
    }
    setText();
}

NxRegEditDialog::~NxRegEditDialog()
{
    NxDebug("[%s]", __func__);
    delete ui;
}

void NxRegEditDialog::on_readButton_clicked()
{
    unsigned int data = ui->lineEdit->text().toInt(NULL, 16);
    struct nx_reg_control control;
    NxDebug("[%s] 0x%02x", __func__, data);
    data = ((data << 8) & 0xFF00);
    NxDebug("addr = 0x%04x", data);
    control.type = type;
    control.flag = nx_reg_flag_read_page;
    control.addr = data;
    control.data = reg;
    emit controlReg(&control);
}

void NxRegEditDialog::getText()
{
    reg[0][0] = ui->reg0_0->text().toInt(NULL, 16);
    reg[0][1] = ui->reg0_4->text().toInt(NULL, 16);
    reg[0][2] = ui->reg0_8->text().toInt(NULL, 16);
    reg[0][3] = ui->reg0_c->text().toInt(NULL, 16);
    reg[1][0] = ui->reg1_0->text().toInt(NULL, 16);
    reg[1][1] = ui->reg1_4->text().toInt(NULL, 16);
    reg[1][2] = ui->reg1_8->text().toInt(NULL, 16);
    reg[1][3] = ui->reg1_c->text().toInt(NULL, 16);
    reg[2][0] = ui->reg2_0->text().toInt(NULL, 16);
    reg[2][1] = ui->reg2_4->text().toInt(NULL, 16);
    reg[2][2] = ui->reg2_8->text().toInt(NULL, 16);
    reg[2][3] = ui->reg2_c->text().toInt(NULL, 16);
    reg[3][0] = ui->reg3_0->text().toInt(NULL, 16);
    reg[3][1] = ui->reg3_4->text().toInt(NULL, 16);
    reg[3][2] = ui->reg3_8->text().toInt(NULL, 16);
    reg[3][3] = ui->reg3_c->text().toInt(NULL, 16);
    reg[4][0] = ui->reg4_0->text().toInt(NULL, 16);
    reg[4][1] = ui->reg4_4->text().toInt(NULL, 16);
    reg[4][2] = ui->reg4_8->text().toInt(NULL, 16);
    reg[4][3] = ui->reg4_c->text().toInt(NULL, 16);
    reg[5][0] = ui->reg5_0->text().toInt(NULL, 16);
    reg[5][1] = ui->reg5_4->text().toInt(NULL, 16);
    reg[5][2] = ui->reg5_8->text().toInt(NULL, 16);
    reg[5][3] = ui->reg5_c->text().toInt(NULL, 16);
    reg[6][0] = ui->reg6_0->text().toInt(NULL, 16);
    reg[6][1] = ui->reg6_4->text().toInt(NULL, 16);
    reg[6][2] = ui->reg6_8->text().toInt(NULL, 16);
    reg[6][3] = ui->reg6_c->text().toInt(NULL, 16);
    reg[7][0] = ui->reg7_0->text().toInt(NULL, 16);
    reg[7][1] = ui->reg7_4->text().toInt(NULL, 16);
    reg[7][2] = ui->reg7_8->text().toInt(NULL, 16);
    reg[7][3] = ui->reg7_c->text().toInt(NULL, 16);
    reg[8][0] = ui->reg8_0->text().toInt(NULL, 16);
    reg[8][1] = ui->reg8_4->text().toInt(NULL, 16);
    reg[8][2] = ui->reg8_8->text().toInt(NULL, 16);
    reg[8][3] = ui->reg8_c->text().toInt(NULL, 16);
    reg[9][0] = ui->reg9_0->text().toInt(NULL, 16);
    reg[9][1] = ui->reg9_4->text().toInt(NULL, 16);
    reg[9][2] = ui->reg9_8->text().toInt(NULL, 16);
    reg[9][3] = ui->reg9_c->text().toInt(NULL, 16);
    reg[10][0] = ui->rega_0->text().toInt(NULL, 16);
    reg[10][1] = ui->rega_4->text().toInt(NULL, 16);
    reg[10][2] = ui->rega_8->text().toInt(NULL, 16);
    reg[10][3] = ui->rega_c->text().toInt(NULL, 16);
    reg[11][0] = ui->regb_0->text().toInt(NULL, 16);
    reg[11][1] = ui->regb_4->text().toInt(NULL, 16);
    reg[11][2] = ui->regb_8->text().toInt(NULL, 16);
    reg[11][3] = ui->regb_c->text().toInt(NULL, 16);
    reg[12][0] = ui->regc_0->text().toInt(NULL, 16);
    reg[12][1] = ui->regc_4->text().toInt(NULL, 16);
    reg[12][2] = ui->regc_8->text().toInt(NULL, 16);
    reg[12][3] = ui->regc_c->text().toInt(NULL, 16);
    reg[13][0] = ui->regd_0->text().toInt(NULL, 16);
    reg[13][1] = ui->regd_4->text().toInt(NULL, 16);
    reg[13][2] = ui->regd_8->text().toInt(NULL, 16);
    reg[13][3] = ui->regd_c->text().toInt(NULL, 16);
    reg[14][0] = ui->rege_0->text().toInt(NULL, 16);
    reg[14][1] = ui->rege_4->text().toInt(NULL, 16);
    reg[14][2] = ui->rege_8->text().toInt(NULL, 16);
    reg[14][3] = ui->rege_c->text().toInt(NULL, 16);
    reg[15][0] = ui->regf_0->text().toInt(NULL, 16);
    reg[15][1] = ui->regf_4->text().toInt(NULL, 16);
    reg[15][2] = ui->regf_8->text().toInt(NULL, 16);
    reg[15][3] = ui->regf_c->text().toInt(NULL, 16);
}

void NxRegEditDialog::on_writeButton_clicked()
{
    unsigned int data = ui->lineEdit->text().toInt(NULL, 16);
    struct nx_reg_control control;
    NxDebug("[%s] 0x%02x", __func__, data);
    data = ((data << 8) & 0xFF00);
    NxDebug("addr = 0x%04x", data);
    getText();
    control.type = type;
    control.flag = nx_reg_flag_write_page;
    control.addr = data;
    control.data = reg;
    emit controlReg(&control);
}

void NxRegEditDialog::keyPressEvent(QKeyEvent *event){
    event->ignore();
    /*
    if(event->key()==Qt::Key_Return){
    }
    else event->ignore();
    */
}

void NxRegEditDialog::writeRegister
(unsigned int addr, unsigned int *data)
{
    struct nx_reg_control control;
    unsigned int page = ui->lineEdit->text().toInt(NULL, 16);
    page = ((page << 8) & 0xFF00);
    addr = addr | page;

    NxDebug("[%s] 0x%04x, 0x%08x ", __func__, addr, *(unsigned int*)data);
    control.type = type;
    control.flag = nx_reg_flag_write;
    control.addr = addr;
    control.data = data;
    emit controlReg(&control);
}

void NxRegEditDialog::on_reg0_0_returnPressed()
{
    reg[0][0] = ui->reg0_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[0][0]);
    writeRegister(0x00, &reg[0][0]);
}

void NxRegEditDialog::on_reg0_4_returnPressed()
{
    reg[0][1] = ui->reg0_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[0][1]);
    writeRegister(0x04, &reg[0][1]);
}

void NxRegEditDialog::on_reg0_8_returnPressed()
{
    reg[0][2] = ui->reg0_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[0][2]);
    writeRegister(0x08, &reg[0][2]);
}

void NxRegEditDialog::on_reg0_c_returnPressed()
{
    reg[0][3] = ui->reg0_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[0][3]);
    writeRegister(0x0c, &reg[0][3]);
}

void NxRegEditDialog::on_reg1_0_returnPressed()
{
    reg[1][0] = ui->reg1_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[1][0]);
    writeRegister(0x10, &reg[1][0]);
}

void NxRegEditDialog::on_reg1_4_returnPressed()
{
    reg[1][1] = ui->reg1_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[1][1]);
    writeRegister(0x14, &reg[1][1]);
}

void NxRegEditDialog::on_reg1_8_returnPressed()
{
    reg[1][2] = ui->reg1_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[1][2]);
    writeRegister(0x18, &reg[1][2]);
}

void NxRegEditDialog::on_reg1_c_returnPressed()
{
    reg[1][3] = ui->reg1_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[1][3]);
    writeRegister(0x1c, &reg[1][3]);
}

void NxRegEditDialog::on_reg2_0_returnPressed()
{
    reg[2][0] = ui->reg2_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[2][0]);
    writeRegister(0x20, &reg[2][0]);
}

void NxRegEditDialog::on_reg2_4_returnPressed()
{
    reg[2][1] = ui->reg2_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[2][1]);
    writeRegister(0x24, &reg[2][1]);
}

void NxRegEditDialog::on_reg2_8_returnPressed()
{
    reg[2][2] = ui->reg2_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[2][2]);
    writeRegister(0x28, &reg[2][2]);
}

void NxRegEditDialog::on_reg2_c_returnPressed()
{
    reg[2][3] = ui->reg2_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[2][3]);
    writeRegister(0x21c, &reg[2][3]);
}

void NxRegEditDialog::on_reg3_0_returnPressed()
{
    reg[3][0] = ui->reg3_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[3][0]);
    writeRegister(0x30, &reg[3][0]);
}

void NxRegEditDialog::on_reg3_4_returnPressed()
{
    reg[3][1] = ui->reg3_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[3][1]);
    writeRegister(0x34, &reg[3][1]);
}

void NxRegEditDialog::on_reg3_8_returnPressed()
{
    reg[3][2] = ui->reg3_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[3][2]);
    writeRegister(0x38, &reg[3][2]);
}

void NxRegEditDialog::on_reg3_c_returnPressed()
{
    reg[3][3] = ui->reg3_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[3][3]);
    writeRegister(0x3c, &reg[3][3]);
}

void NxRegEditDialog::on_reg4_0_returnPressed()
{
    reg[4][0] = ui->reg4_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[4][0]);
    writeRegister(0x40, &reg[4][0]);
}

void NxRegEditDialog::on_reg4_4_returnPressed()
{
    reg[4][1] = ui->reg4_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[4][1]);
    writeRegister(0x43, &reg[4][1]);
}

void NxRegEditDialog::on_reg4_8_returnPressed()
{
    reg[4][2] = ui->reg4_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[4][2]);
    writeRegister(0x48, &reg[4][2]);
}

void NxRegEditDialog::on_reg4_c_returnPressed()
{
    reg[4][3] = ui->reg4_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[4][3]);
    writeRegister(0x4c, &reg[4][3]);
}

void NxRegEditDialog::on_reg5_0_returnPressed()
{
    reg[5][0] = ui->reg5_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[5][0]);
    writeRegister(0x50, &reg[5][0]);
}

void NxRegEditDialog::on_reg5_4_returnPressed()
{
    reg[5][1] = ui->reg5_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[5][1]);
    writeRegister(0x54, &reg[5][1]);
}

void NxRegEditDialog::on_reg5_8_returnPressed()
{
    reg[5][2] = ui->reg5_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[5][2]);
    writeRegister(0x58, &reg[5][2]);
}

void NxRegEditDialog::on_reg5_c_returnPressed()
{
    reg[5][3] = ui->reg5_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[5][3]);
    writeRegister(0x5c, &reg[5][3]);
}

void NxRegEditDialog::on_reg6_0_returnPressed()
{
    reg[6][0] = ui->reg6_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[6][0]);
    writeRegister(0x60, &reg[6][0]);
}

void NxRegEditDialog::on_reg6_4_returnPressed()
{
    reg[6][1] = ui->reg6_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[6][1]);
    writeRegister(0x64, &reg[6][1]);
}


void NxRegEditDialog::on_reg6_8_returnPressed()
{
    reg[6][2] = ui->reg6_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[6][2]);
    writeRegister(0x68, &reg[6][2]);
}

void NxRegEditDialog::on_reg6_c_returnPressed()
{
    reg[6][3] = ui->reg6_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[6][3]);
    writeRegister(0x6c, &reg[6][3]);
}

void NxRegEditDialog::on_reg7_0_returnPressed()
{
    reg[7][0] = ui->reg7_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[7][0]);
    writeRegister(0x70, &reg[7][0]);
}

void NxRegEditDialog::on_reg7_4_returnPressed()
{
    reg[7][1] = ui->reg7_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[7][1]);
    writeRegister(0x74, &reg[7][1]);
}

void NxRegEditDialog::on_reg7_8_returnPressed()
{
    reg[7][2] = ui->reg7_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[7][2]);
    writeRegister(0x78, &reg[7][2]);
}

void NxRegEditDialog::on_reg7_c_returnPressed()
{
    reg[7][3] = ui->reg7_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[7][3]);
    writeRegister(0x7c, &reg[7][3]);
}

void NxRegEditDialog::on_reg8_0_returnPressed()
{
    reg[8][0] = ui->reg8_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[8][0]);
    writeRegister(0x80, &reg[8][0]);
}

void NxRegEditDialog::on_reg8_4_returnPressed()
{
    reg[8][1] = ui->reg8_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[8][1]);
    writeRegister(0x84, &reg[8][1]);
}

void NxRegEditDialog::on_reg8_8_returnPressed()
{
    reg[8][2] = ui->reg8_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[8][2]);
    writeRegister(0x88, &reg[8][2]);
}

void NxRegEditDialog::on_reg8_c_returnPressed()
{
    reg[8][3] = ui->reg8_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[8][3]);
    writeRegister(0x8c, &reg[8][3]);
}

void NxRegEditDialog::on_reg9_0_returnPressed()
{
    reg[9][0] = ui->reg9_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[9][0]);
    writeRegister(0x90, &reg[9][0]);
}

void NxRegEditDialog::on_reg9_4_returnPressed()
{
    reg[9][1] = ui->reg9_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[9][1]);
    writeRegister(0x94, &reg[9][1]);
}

void NxRegEditDialog::on_reg9_8_returnPressed()
{
    reg[9][2] = ui->reg9_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[9][2]);
    writeRegister(0x98, &reg[9][2]);
}

void NxRegEditDialog::on_reg9_c_returnPressed()
{
    reg[9][3] = ui->reg9_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[9][3]);
    writeRegister(0x9c, &reg[9][3]);
}

void NxRegEditDialog::on_rega_0_returnPressed()
{
    reg[10][0] = ui->rega_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[10][0]);
    writeRegister(0xa0, &reg[10][0]);
}

void NxRegEditDialog::on_rega_4_returnPressed()
{
    reg[10][1] = ui->rega_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[10][1]);
    writeRegister(0xa4, &reg[10][1]);
}

void NxRegEditDialog::on_rega_8_returnPressed()
{
    reg[10][2] = ui->rega_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[10][2]);
    writeRegister(0xa8, &reg[10][2]);
}

void NxRegEditDialog::on_rega_c_returnPressed()
{
    reg[10][3] = ui->rega_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[10][3]);
    writeRegister(0xac, &reg[10][3]);
}

void NxRegEditDialog::on_regb_0_returnPressed()
{
    reg[11][0] = ui->regb_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[11][0]);
    writeRegister(0xb0, &reg[11][0]);
}

void NxRegEditDialog::on_regb_4_returnPressed()
{
    reg[11][1] = ui->regb_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[11][1]);
    writeRegister(0xb4, &reg[11][1]);
}

void NxRegEditDialog::on_regb_8_returnPressed()
{
    reg[11][2] = ui->regb_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[11][2]);
    writeRegister(0xb8, &reg[11][2]);
}

void NxRegEditDialog::on_regb_c_returnPressed()
{
    reg[11][3] = ui->regb_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[11][3]);
    writeRegister(0xbc, &reg[11][3]);
}

void NxRegEditDialog::on_regc_0_returnPressed()
{
    reg[12][0] = ui->regc_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[12][0]);
    writeRegister(0xc0, &reg[12][0]);
}

void NxRegEditDialog::on_regc_4_returnPressed()
{
    reg[12][1] = ui->regc_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[12][1]);
    writeRegister(0xc4, &reg[12][1]);
}

void NxRegEditDialog::on_regc_8_returnPressed()
{
    reg[12][2] = ui->regc_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[12][2]);
    writeRegister(0xc8, &reg[12][2]);
}

void NxRegEditDialog::on_regc_c_returnPressed()
{
    reg[12][3] = ui->regc_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[12][3]);
    writeRegister(0xcc, &reg[12][3]);
}

void NxRegEditDialog::on_regd_0_returnPressed()
{
    reg[13][0] = ui->regd_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[13][0]);
    writeRegister(0xd0, &reg[13][0]);
}

void NxRegEditDialog::on_regd_4_returnPressed()
{
    reg[13][1] = ui->regd_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[13][10]);
    writeRegister(0xd4, &reg[13][1]);
}

void NxRegEditDialog::on_regd_8_returnPressed()
{
    reg[13][2] = ui->regd_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[13][2]);
    writeRegister(0xd8, &reg[13][2]);
}

void NxRegEditDialog::on_regd_c_returnPressed()
{
    reg[13][3] = ui->regd_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[13][3]);
    writeRegister(0xdc, &reg[13][3]);
}

void NxRegEditDialog::on_rege_0_returnPressed()
{
    reg[14][0] = ui->rege_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[14][0]);
    writeRegister(0xe0, &reg[14][0]);
}

void NxRegEditDialog::on_rege_4_returnPressed()
{
    reg[14][1] = ui->rege_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[14][1]);
    writeRegister(0xe4, &reg[14][1]);
}

void NxRegEditDialog::on_rege_8_returnPressed()
{
    reg[14][2] = ui->rege_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[14][2]);
    writeRegister(0xe8, &reg[14][2]);
}

void NxRegEditDialog::on_rege_c_returnPressed()
{
    reg[14][3] = ui->rege_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[14][3]);
    writeRegister(0xec, &reg[14][3]);
}

void NxRegEditDialog::on_regf_0_returnPressed()
{
    reg[15][0] = ui->regf_0->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[15][0]);
    writeRegister(0xf0, &reg[15][0]);
}

void NxRegEditDialog::on_regf_4_returnPressed()
{
    reg[15][1] = ui->regf_4->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[15][1]);
    writeRegister(0xf4, &reg[15][1]);
}

void NxRegEditDialog::on_regf_8_returnPressed()
{
    reg[15][2] = ui->regf_8->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[15][2]);
    writeRegister(0xf8, &reg[15][2]);
}

void NxRegEditDialog::on_regf_c_returnPressed()
{
    reg[15][3] = ui->regf_c->text().toInt(NULL, 16);
    NxDebug("[%s] 0x%08x", __func__, reg[15][3]);
    writeRegister(0xfc, &reg[15][3]);
}
