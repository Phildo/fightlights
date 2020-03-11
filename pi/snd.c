#include "snd.h"

#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdarg.h>

#define PCM_DEVICE "default"

int snd_killed;
static int snd_inited = 0;

static const char *assets_folder = "/home/phildo/projects/fightlights/assets";
static int n_files;
static const char *filename[] = {"test.wav","test.wav","test.wav"};
static int running[]          = {    0,         0,          0    };
static SNDFILE **file;
static SF_INFO *sfinfo;
static snd_pcm_t *pcm_handle;
static snd_pcm_hw_params_t *params;
static int bufflen;
static short* read_buff;
static short* snd_buff;
static snd_pcm_uframes_t frames;

void snd_debug(char *fmt, ...)
{
  printf("SND: ");
  va_list myargs;
  va_start(myargs, fmt);
  vprintf(fmt, myargs);
  va_end(myargs);
  fflush(stdout);
}

void snd_init()
{
  n_files = sizeof(filename)/sizeof(const char *);
  file   = malloc(sizeof(SNDFILE *)*n_files);
  sfinfo = malloc(sizeof(SF_INFO)  *n_files);
  char filepath[255];

  int assets_len = strlen(assets_folder);
  for(int i = 0; i < n_files; i++)
  {
    strcpy(filepath,assets_folder);
    filepath[assets_len] = '/';
    strcpy(filepath+assets_len+1,filename[i]);
    if(!(file[i] = sf_open(filepath, SFM_READ, &sfinfo[i])))
    {
      snd_debug("Failed opening sound file %s\n",filepath);
      exit(1);
    }
    /*
    snd_debug("Channels: %d\n", sfinfo[i].channels);
    snd_debug("Sample rate: %d\n", sfinfo[i].samplerate);
    snd_debug("Sections: %d\n", sfinfo[i].sections);
    snd_debug("Format: %d\n", sfinfo[i].format);
    */
  }

  /* Open the PCM device in playback mode */
  if(snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0) != 0)
  {
    snd_debug("Failed opening sound pcm device\n");
    exit(1);
  }

  /* Allocate parameters object and fill it with default values*/
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(pcm_handle, params);
  /* Set parameters */
  snd_pcm_hw_params_set_access(  pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(  pcm_handle, params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(pcm_handle, params, sfinfo[0].channels);
  snd_pcm_hw_params_set_rate(    pcm_handle, params, sfinfo[0].samplerate, 0);
  /* Write parameters */
  if(snd_pcm_hw_params(pcm_handle, params) != 0)
  {
    snd_debug("Failed setting pcm hw params\n");
    exit(1);
  }

  /* Allocate buffer to hold single period */
  int dir;
  if(snd_pcm_hw_params_get_period_size(params, &frames, &dir) != 0)
  {
    snd_debug("Couldn't get exact period size\n");
    exit(1);
  }
  bufflen = frames * sfinfo[0].channels;
  read_buff = malloc(bufflen * sizeof(short));
  snd_buff  = malloc(bufflen * sizeof(short));

  snd_killed = 0;
  snd_inited = 1;
}

void snd_die();

void snd_play(int i)
{
  if(snd_inited) sf_seek(file[i],0,0);
  running[i] = 1;
}

int snd_do()
{
  if(snd_killed) { snd_die(); return 0; }

  memset(snd_buff,0,bufflen*sizeof(short));
  int feed = 0;
  for(int i = 0; i < n_files; i++)
  {
    if(running[i])
    {
      int readcount = sf_readf_short(file[i], read_buff, frames);
      if(readcount)
      {
        for(int j = 0; j < readcount*sfinfo[0].channels; j++)
        {
          snd_buff[j] += read_buff[j]; //TODO: actually mix!
        }
        if(readcount > feed) feed = readcount;
      }
      else running[i] = 0;
    }
  }
  if(feed)
  {
    int pcmrc = snd_pcm_writei(pcm_handle, snd_buff, feed);
    if(pcmrc == -EPIPE)
    {
      //snd_debug("Underrun!\n"); //this is fine- there's no way to know if the sound is "finished playing" until it's too late (sf_readf_short returns 0)
      snd_pcm_prepare(pcm_handle);
    }
    else if(pcmrc < 0)
    {
      snd_debug("Error writing to PCM device: %s\n", snd_strerror(pcmrc));
      exit(1);
    }
    else if(pcmrc != feed)
    {
      snd_debug("PCM write difffers from PCM read.\n");
      exit(1);
    }
  }

  return 1;
}

void snd_kill()
{
  snd_debug("killed\n");
  snd_killed = 1;
  for(int i = 0; i < n_files; i++)
    running[i] = 0;
}

void snd_die()
{
  snd_pcm_drain(pcm_handle);
  snd_pcm_close(pcm_handle);
  free(read_buff);
  free(snd_buff);
  for(int i = 0; i < n_files; i++)
    sf_close(file[i]);
  free(file);
  free(sfinfo);
}

