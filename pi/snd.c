#include "snd.h"

#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <stdio.h>

#define PCM_DEVICE "default"

void snd_init()
{
  const char *filename = "/home/pi/shortsample.wav";
  SNDFILE *file = NULL;
  SF_INFO sfinfo;

  if(!(file = sf_open(filename, SFM_READ, &sfinfo)))
  {
    fprintf(stderr,"Failed opening sound file %s\n",filename);
    exit(1);
  }
  /*
  fprintf(stderr,"Channels: %d\n", sfinfo.channels);
  fprintf(stderr,"Sample rate: %d\n", sfinfo.samplerate);
  fprintf(stderr,"Sections: %d\n", sfinfo.sections);
  fprintf(stderr,"Format: %d\n", sfinfo.format);
  */

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
  snd_pcm_hw_params_set_channels(pcm_handle, params, sfinfo.channels);
  snd_pcm_hw_params_set_rate(    pcm_handle, params, sfinfo.samplerate, 0);
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
  int* buff = NULL;
  int readcount;
  buff = malloc(frames * sfinfo.channels * sizeof(short));
  while((readcount = sf_readf_short(file, buff, frames)) > 0)
  {
    int pcmrc = snd_pcm_writei(pcm_handle, buff, readcount);
    if(pcmrc == -EPIPE)
    {
      fprintf(stderr, "Underrun!\n");
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

  snd_pcm_drain(pcm_handle);
  snd_pcm_close(pcm_handle);
  free(buff);
}

int snd_do()
{
  return 1;
}

void snd_kill()
{

}

