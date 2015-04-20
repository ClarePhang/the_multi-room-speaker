#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QThread>
#include "udpdisc.h"
#include "srvq.h"
#include "sclient.h"
#include <thread>

namespace Ui {
class Dialog;
}

class player;
class sclient;
class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();
    void update_ui(size_t len, size_t qoptim, int qerc, int qskew);
public slots:
    void timer_slot();
private:
    void _tmain();
private:
    Ui::Dialog   *ui;
    QTimer*      _timer;
    bool         _server=false;
    std::thread* _pthread = nullptr;
    std::set<std::string> _ifs;
    size_t       _optim = 0;
    size_t       _length = 0;
    int          _maxl = 0;
    int          _sque = 0;
    std::mutex   _mut;
};

#endif // DIALOG_H
