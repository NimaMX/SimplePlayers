#include <fstream>
#include <ios>
#include <iostream>
#include <stdio.h>
#include <stdint.h>

#include <alsa/asoundlib.h>

#define PCM_DEVICE "default"

using namespace std;

typedef struct WaveHeader {

    uint8_t riff[4];
    uint32_t chunkSize;
    uint8_t wave[4];
    uint8_t fmt[4];
    uint32_t subChunkSize;
    uint16_t audioFormat;
    uint16_t channelNum;
    uint32_t samplePerSec;
    uint32_t bytePerSec;
    uint16_t blockAgain;
    uint16_t bitsPerSample;
    uint8_t subChunkId[4];
    uint32_t subChunk2Size;

} wave_header_t;

static void printFileHeader(const WaveHeader& waveHeader) {

    cout << "======================================" << endl;
    cout << "RIFF Header : " << waveHeader.riff[0] << waveHeader.riff[1] << 
        waveHeader.riff[2] << waveHeader.riff[3] << endl;
    cout << "WAV Header : " << waveHeader.wave[0] <<  waveHeader.wave[1] << 
        waveHeader.wave[2] << waveHeader.wave[3] << endl; 
    cout << "FMT : " << waveHeader.fmt[0] << waveHeader.fmt[1] << waveHeader.fmt[2] << 
        waveHeader.fmt[3] << endl;
    cout << "Data Size : " << waveHeader.chunkSize << endl;
    cout << "Sampling Rate :" << waveHeader.samplePerSec << endl;
    cout << "Number of bits used :" << waveHeader.bitsPerSample << endl;
    cout << "Number of channels : " << waveHeader.channelNum << endl;
    cout << "Number of bytes per second : " << waveHeader.bytePerSec << endl;
    cout << "Data Length : " << waveHeader.subChunk2Size << endl;
    cout << "Audio Format : " << waveHeader.audioFormat << endl;
    cout << "Block Align : " << waveHeader.blockAgain << endl;
    cout << "Data String : " << waveHeader.subChunkId[0] << waveHeader.subChunkId[1] << 
        waveHeader.subChunkId[2] << waveHeader.subChunkId[3] << endl;
    cout << "=======================================" << endl;
}

int main(int argc, char *argv[]) {


    FILE *srcFile = nullptr;
    size_t readSize = 0;
    wave_header_t waveHeader;
   
    unsigned int tmp = 0; 
    int result = 0;
    snd_pcm_uframes_t frames;
    snd_pcm_t *playbackPcm;
    snd_pcm_hw_params_t *param;

    if(argc < 2) {
        cout << "Please enter the input fie path" << endl;
        return 1;
    }

    if((result = snd_pcm_open(&playbackPcm, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        cout << "opening pcm device failed : " << snd_strerror(result) << endl;
        return 1;
    } 

    srcFile = fopen(argv[1], "r");
    readSize = fread(&waveHeader, 1, sizeof(wave_header_t), srcFile);
    if(readSize > 0) {

        cout << readSize << " Header Byte has been read" << endl;
        printFileHeader(waveHeader);
       
       snd_pcm_hw_params_alloca(&param);
       snd_pcm_hw_params_any(playbackPcm, param);
       snd_pcm_hw_params_set_access(playbackPcm, param, SND_PCM_ACCESS_RW_INTERLEAVED);
       snd_pcm_hw_params_set_format(playbackPcm, param, SND_PCM_FORMAT_S16_LE);
       snd_pcm_hw_params_set_channels(playbackPcm, param, waveHeader.channelNum);
       snd_pcm_hw_params_set_rate_near(playbackPcm, param, &waveHeader.samplePerSec, 0);
       snd_pcm_hw_params(playbackPcm, param);

       cout << "PCM Name : " << snd_pcm_name(playbackPcm) << endl;
       cout << "PCM Status : " << snd_pcm_state_name(snd_pcm_state(playbackPcm)) << endl;

       snd_pcm_hw_params_get_channels(param, &tmp);
       cout << "PCM Channel : " << tmp << endl;

       snd_pcm_hw_params_get_rate(param, &tmp, 0);
       cout << "PCM Rate is : " << tmp << endl;

       snd_pcm_hw_params_get_period_size(param, &frames, 0); 

       const uint16_t size = (frames * waveHeader.channelNum * 2);
       cout << "PCM buffer size : " << size << endl;  
       uint8_t *buffer = new uint8_t[size];

       while((readSize = fread(buffer, sizeof(buffer[0]), size / sizeof(buffer[0]), srcFile)) > 0) { 
            
           if((result = snd_pcm_writei(playbackPcm, buffer, frames)) == -EPIPE) {
                cout << "the pcm is underrun" << endl;
                snd_pcm_prepare(playbackPcm);
           
           } else if (frames < 0) {
                cout << "cannot write to pcm device" << snd_strerror(result) << endl;
           }

           cout << "a new chunk of data read : " << readSize << endl;
           cout << frames << " bytes data wrote to pcm device " << endl;
       }

       if((result = snd_pcm_drain(playbackPcm)) < 0) {
            cout << "draning pcm device failed : " << snd_strerror(result) << endl;
            return 1;
       }

       delete [] buffer;
       buffer = nullptr;
    
    } else {
        cout << "Failed to open the file" << endl;
    }

    snd_pcm_close(playbackPcm);
    fclose(srcFile);
    return 0;
}
