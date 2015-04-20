#ifdef WITH_SDL

#include <unistd.h>
#include <QDebug>
#include "tinyauan.h"
#include <stdio.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h>

#define SL_BUFFERS  8

#define  TRY_(ex_) result=ex_; if(result!=SL_RESULT_SUCCESS) throw __LINE__;
#define  NTRY_(ex_)  do{	int k=8;								\
							while(k--){  							\
									result=ex_;						\
									if(result==SL_RESULT_SUCCESS)		\
										break;						\
								::usleep(0xFFFF);					\
							}										\
							if(result!=SL_RESULT_SUCCESS)				\
								throw __LINE__;						\
						}while(0);


void MaiBufferQueueCallback(
		SLBufferQueueItf queueItf,
		SLuint32 eventFlags,
		const void *pBuffer,
		SLuint32 bufferSize,
		SLuint32 dataUsed,
		void *pContext)
{
	Q_UNUSED(queueItf);
	Q_UNUSED(eventFlags);
	Q_UNUSED(pBuffer);
	Q_UNUSED(queueItf);
	Q_UNUSED(bufferSize);
	Q_UNUSED(dataUsed);
	Q_UNUSED(pContext);
	qDebug() << "buffer = " << bufferSize << " used " << dataUsed << "\n";
}

//int,int,int
//uint32_t _channels, uint32_t _sample_rate, uint32_t _bits_per_sample
AoCls::AoCls(int xbits_per_sam,int xsam_rate,int xnum_chn)
{
    _stopped = 0;
    _bits_per_sample=xbits_per_sam;
    _sample_rate=xsam_rate;
    _channels=xnum_chn;
    _one_sec_buff = (_bits_per_sample * _sample_rate * _channels) / 8; // per 1 second
    //create sbuf
}

AoCls::~AoCls()
{
	this->stop();

	//std::lock_guard<std::mutex> guard(_mut);
	(*_player)->Destroy(_player);
	(*_pout_mix)->Destroy(_pout_mix);
	(*_psl)->Destroy(_psl);
	delete [] sbuf;
}

void AoCls::stop()
{
	_stopped = 1;
	(*_pplay_itf)->SetPlayState( _pplay_itf, SL_PLAYSTATE_STOPPED );
}

int AoCls::init_ao(int time)
{
    Q_UNUSED(time);
    SLVolumeItf     volumeItf;
    SLEngineItf     engine_itf;
    SLresult        result = SL_RESULT_UNKNOWN_ERROR;
    SLEngineOption EngineOption[] = {
        {(SLuint32)SL_ENGINEOPTION_THREADSAFE,     (SLuint32)SL_BOOLEAN_TRUE},
        {(SLuint32)1, (SLuint32)1},
        { (SLuint32)0, (SLuint32)1}
    };
    SLboolean       required[3] = {SL_BOOLEAN_FALSE,SL_BOOLEAN_FALSE,SL_BOOLEAN_FALSE};
    SLInterfaceID   iidArray[3]={SL_IID_NULL,SL_IID_NULL,SL_IID_NULL};

    _buffers     = SL_BUFFERS;//(_one_sec_buff+TCP_SND_LEN) / TCP_SND_LEN;
    _buff_sz     = TCP_SND_LEN;
    sbuf         = new uint8_t[_buffers * _buff_sz];
    _sbuff_count = 0;

    try{
        result = slCreateEngine(&_psl, 2, EngineOption, 0, 0, 0);
        //TRY_ (slCreateEngine(&_psl, 2, EngineOption, 0, 0, 0));
        NTRY_((*_psl)->Realize(_psl, SL_BOOLEAN_FALSE));
        TRY_ ((*_psl)->GetInterface(_psl, SL_IID_ENGINE, (void*)&engine_itf));
        TRY_ ((*engine_itf)->CreateOutputMix(engine_itf, &_pout_mix, 0, iidArray, required));
        NTRY_((*_pout_mix)->Realize(_pout_mix, SL_BOOLEAN_FALSE));

        // NTRY_((*_pout_mix)->GetInterface(_pout_mix, SL_IID_VOLUME, (void*)&volumeItf));

        SLDataLocator_BufferQueue bufferQueue;
        bufferQueue.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
        bufferQueue.numBuffers = _buffers;

        SLDataFormat_PCM pcm;
        pcm.formatType      = SL_DATAFORMAT_PCM;
        pcm.numChannels     = _channels;
        pcm.samplesPerSec   = SL_SAMPLINGRATE_44_1;
        pcm.bitsPerSample   = _bits_per_sample;
        pcm.containerSize   = _bits_per_sample;
        pcm.channelMask     = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        pcm.endianness      = SL_BYTEORDER_LITTLEENDIAN;

        SLDataSource audioSource;
        audioSource.pFormat = (void *)&pcm;
        audioSource.pLocator = (void *)&bufferQueue;

        SLDataLocator_OutputMix locator_outputmix;
        SLDataSink audioSink;
        locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
        locator_outputmix.outputMix = _pout_mix;
        audioSink.pLocator = (void *)&locator_outputmix;
        audioSink.pFormat = 0;

        required[0] = SL_BOOLEAN_TRUE;
        iidArray[0] = SL_IID_BUFFERQUEUE;

        TRY_((*engine_itf)->CreateAudioPlayer(engine_itf, &_player, &audioSource, &audioSink, 1, iidArray, required));
        TRY_((*_player)->Realize(_player, SL_BOOLEAN_FALSE));
        TRY_((*_player)->GetInterface(_player, SL_IID_PLAY, (void*)&_pplay_itf));
        TRY_((*_player)->GetInterface(_player, SL_IID_BUFFERQUEUE, (void*)&_buff_qitf));
        (*_pplay_itf)->SetPlayState( _pplay_itf, SL_PLAYSTATE_PLAYING );

    }catch(int l)
    {
        qDebug() << "execption " << " result: " << result << " line: " << l ;
        return 0;
    }
    return _one_sec_buff;
}

void AoCls::play(const uint8_t* pb, int len)
{
	//std::lock_guard<std::mutex> guard(_mut);

	uint32_t buf_required = (len + _buff_sz - 1) / _buff_sz;
	SLBufferQueueState state;
	Q_UNUSED(buf_required);

	(*_buff_qitf)->GetState(_buff_qitf, &state);
	while(state.count!=0){
		(*_buff_qitf)->GetState(_buff_qitf, &state);
		::usleep(0xF);
	};
	uint32_t buf_loc = 0;
	while (len)//(buf_required)
	{
		(*_buff_qitf)->GetState(_buff_qitf, &state);
		if (_buffers - state.count)
		{
			uint32_t to_copy = len;
			if (to_copy > _buff_sz) to_copy = _buff_sz;
			::memcpy(sbuf + _sbuff_count, pb + buf_loc, to_copy);
			(*_buff_qitf)->Enqueue(_buff_qitf, (void*)(sbuf + _sbuff_count), to_copy);
			_sbuff_count += _buff_sz;
			_sbuff_count %= _buffers * _buff_sz;
			buf_loc += to_copy;
			len -= to_copy;
		}
	}
}

#endif //#ifdef WITH_TINY_ALSA
