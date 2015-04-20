#include "dialog.h"
#include "ui_dialog.h"
#include <QTimer>
#include "player.h"


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
    ui->ST_SRV->setText("searching");
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()),  this, SLOT(timer_slot()));
    _timer->start(1000);
}

void Dialog::timer_slot()
{
    if(_server == false)
    {
        _ifs.clear();

        Udper::join(_ifs);
        if(_ifs.size() )
        {
            _server = true;
            ui->ST_SRV->setText(_ifs.begin()->c_str());
        }
    }
    else
    {
        if(_pthread==nullptr || __alive==false)
        {
            ui->ST_SRV->setText("restarting");
            if(_pthread)
            {
                __alive = false;
                _pthread->join();
                delete _pthread;
            }

            ui->ST_SRV->setText("running");
            _pthread =  new std::thread(&Dialog::_tmain,this);
            __alive=true;
        }
    }
    if(_pthread)
    {
        std::lock_guard<std::mutex> guard(_mut);
        QString t = "LEN=" + QString::number(_length);
        ui->ST_QL->setText(t);

        t = "MAX=" + QString::number(_maxl);
        ui->ST_QP->setText(t);

        t = "SQ=" + QString::number(_sque);
        ui->ST_QS->setText(t);

        t = "OPT=" + QString::number(_optim);
        ui->ST_QD->setText(t);
    }
}

Dialog::~Dialog()
{
    _timer->stop();
    delete _timer;
    delete ui;
}

void Dialog::update_ui(size_t len, size_t qoptim, int maxl, int qskew)
{
    std::lock_guard<std::mutex> guard(_mut);
    _length = len;
    _optim = qoptim;
    _maxl = maxl;
    _sque = qskew;
}

void Dialog::_tmain()
{
    Circle        qb;                   // play queue
    sclient       client(&qb, _ifs);     // client thread to queue
    player        player(&qb, &client); // player main thread from queue

    __alive = true;
    const size_t bl =  player.get_len_for(2/*seconds len buffer*/);
    if(bl)
    {
        const size_t chunks = (bl+TCP_SND_LEN) / TCP_SND_LEN;   // round it up
        const size_t tper_chunk = 1000/chunks;

        TERM_OUT(true,MAIN_L,"PLAY QUEUE: %d OF %d BYTES", chunks, TCP_SND_LEN);
        qb.reserve(chunks, TCP_SND_LEN);    // reserve the queue
        qb.set_optim_perc(CLI_SYNC_PERCENT);
        player.start();

        if(client.run(tper_chunk))          // tcp thread
        {
            player.play(client, this);            // main loop
            _server = false;
        }
    }
    __alive = false;
}

