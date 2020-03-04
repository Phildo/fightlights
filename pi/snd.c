#include "snd.h"

#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <stdio.h>

#define PCM_DEVICE "default"

volatile int snd_play = -1;

static const char *filename[] = {"../assets/test.wav","../assets/test.wav","../assets/test.wav"};
static SNDFILE **file;
static SF_INFO *sfinfo;

void snd_init()
{
  int n_files = sizeof(filename)/sizeof(const char *);
  file       = malloc(sizeof(SNDFILE *)*n_files);
  sfinfo     = malloc(sizeof(SF_INFO)  *n_files);

  for(int i = 0; i < n_files; i++)
  {
    if(!(file[i] = sf_open(filename[i], SFM_READ, &sfinfo[i])))
    {
      fprintf(stderr,"Failed opening sound file %s\n",filename[i]);
      exit(1);
    }
    /*
    fprintf(stderr,"Channels: %d\n", sfinfo[i].channels);
    fprintf(stderr,"Sample rate: %d\n", sfinfo[i].samplerate);
    fprintf(stderr,"Sections: %d\n", sfinfo[i].sections);
    fprintf(stderr,"Format: %d\n", sfinfo[i].format);
    */
  }
}

int snd_do()
{
  while(snd_play != -1)
  {
    int i = snd_play;
    snd_play = -1;

    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;

    /* Open the PCM device in playback mode */
    if(snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0) != 0)
    {
      fprintf(stderr,"Failed opening sound pcm device\n");
      exit(1);
    }

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    /* Set parameters */
    snd_pcm_hw_params_set_access(  pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(  pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, sfinfo[i].channels);
    snd_pcm_hw_params_set_rate(    pcm_handle, params, sfinfo[i].samplerate, 0);
    /* Write parameters */
    if(snd_pcm_hw_params(pcm_handle, params) != 0)
    {
      fprintf(stderr,"Failed setting pcm hw params\n");
      exit(1);
    }

    /* Allocate buffer to hold single period */
    int dir;
    snd_pcm_uframes_t frames;
    if(snd_pcm_hw_params_get_period_size(params, &frames, &dir) != 0)
    {
      fprintf(stderr,"Couldn't get exact period size\n");
      exit(1);
    }
    short* buff = NULL;
    int readcount;
    buff = malloc(frames * sfinfo[i].channels * sizeof(short));
    while((readcount = sf_readf_short(file[i], buff, frames)) > 0)
    {
      int pcmrc = snd_pcm_writei(pcm_handle, buff, readcount);
      if(pcmrc == -EPIPE)
      {
        fprintf(stderr, "Underrun!\n"); fflush(stderr);
        snd_pcm_prepare(pcm_handle);
      }
      else if(pcmrc < 0)
      {
        fprintf(stderr, "Error writing to PCM device: %s\n", snd_strerror(pcmrc));
        exit(1);
      }
      else if(pcmrc != readcount)
      {
        fprintf(stderr,"PCM write difffers from PCM read.\n");
        exit(1);
      }
    }
    sf_seek(file[i],0,0);

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    free(buff);
  }

  return 1;
}

void snd_kill()
{
  snd_play = -1;
}

