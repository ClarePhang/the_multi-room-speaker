#include "dialog.h"
#include "ui_dialog.h"
#include "screenxy.h"
#include "clithread.h"
#include "main.h"
#include "tinyauan.h"
#include <QTimer>

Dialog* PCTX;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    PCTX = this;
    ui->setupUi(this);
    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
    ui->ST_SRV->setText("searching");
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()),  this, SLOT(timer_slot()));
    _pthread =  new std::thread(&Dialog::_tmain,this);
}

int  Dialog::cli_frame()const
{
    std::lock_guard<std::mutex> guard(_mut);
    return _frame;
}


void Dialog::timer_slot()
{
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


size_t Dialog::fill_q(Qbuff& qb, bool& filling)
{
    size_t qoptim = qb.optim();
    if(qb.length()==0)
    {
        filling = true;
    }
    if(filling)
    {
        if(qb.length()>qoptim)
        {
            filling = false;
        }
        else
        {
            ::usleep(0xFFF);
            TERM_OUT(true,QUE_L,"NO STREAM");
        }
    }
    return qoptim;
}

void Dialog::_tmain()
{
    int           buffsz = 4096;
#if defined(AO_LIB)
    AoCls         ao(BITS_AO, SAMPLE_AO, CHANNELS_AO, UDP_SOUND_BUFF);
#elif defined(PULSE_AUDIO)
    pulsecls      pu(BITS_AO, SAMPLE_AO, CHANNELS_AO, UDP_SOUND_BUFF);
#elif defined(ALSA_AUDIO)
    Alsacls       alsa;
    buffsz = alsa.open(SAMPLE_AO, CHANNELS_AO, 1, UDP_SOUND_BUFF);
#elif defined(WITH_SDL)
    AoCls         ao(BITS_AO, SAMPLE_AO, CHANNELS_AO);
#endif

    int           onekchunk = buffsz/1024;
    Qbuff         qb(CHUNKS, UDP_SOUND_BUFF);
    Qbuff::Chunk* pk;
    CliThread     t(&qb);
    uint64_t      ct = tick_count();
    uint64_t      now = ct;
    size_t        qlen,bytesec = 0,qoptim=OPTIM_QUEUE_LEN_H;
    bool          filling = true;
    size_t        fillingsz = Q_ALMOST_EMPTY;
    size_t        sleept = (1000*UDP_SOUND_BUFF) / (SAMPLE_AO * CHANNELS_AO * (BITS_AO/8));
    int           timebuff = ao.init_ao(2);

    qb.set_optim(PER_CENT(qoptim));
    TERM_OUT(true,VER_L, "VERSION: %s",_VERSION);

    t.run(0);
    while(__alive)
    {

        qlen = qb.length();
        if(qlen < fillingsz || filling)     // filling empty q
        {
            TERM_OUT(true,QUE_L,"NO STREAM");
            qb.set_optim(PER_CENT(qoptim));
            fillingsz = fill_q(qb, filling);
            continue;
        }
        fillingsz = Q_ALMOST_EMPTY;

        pk =  qb.q_consume();
        if(pk != nullptr)
        {
            if(pk->_hdr->n_bytes && pk->_hdr->pred==ALIVE_DATA)
            {
#ifdef AO_LIB
                ao.play(pk->payload(),pk->len(), &qb, t.is_pilot() );
#elif defined(PULSE_AUDIO)
                optim = pu.play(pk->payload(),pk->_hdr->n_bytes, qb);
#elif defined(ALSA_AUDIO)
                optim = alsa.playbuffer(pk->payload(),pk->_hdr->n_bytes, qb);
#elif defined(WITH_SDL)
                ao.play(pk->payload(),pk->len());
#endif
                bytesec += pk->_hdr->n_bytes;
                pk->_hdr->n_bytes = 0;
                now = tick_count();
                if(now-ct>=QUART_SEC)
                {
                    TERM_OUT(true, QUE_L ,"SEQ:%05d RATE:%d Kby/s QL-[%05d] QO-[%05d] C_SKEW %05d S_SKE %05d",
                                        pk->_hdr->cli_frame,
                                        bytesec / QUART_SEC,
                                        qb.length() , qb.optim(),
                                        int(qb.optim() - qb.length()),
                                        t.srv_skew());
                    ct      = now;
                    bytesec = 0;
                }
            }
            pk->_hdr->n_bytes=0;
            do{
                std::lock_guard<std::mutex> guard(_mut);
                _frame = pk->_hdr->cli_frame;
            }while(0);
            qb.release(pk);
        }else {
            ::usleep(0x1FF);
        }
    }
    ao.stop();
    __alive=false;
}

