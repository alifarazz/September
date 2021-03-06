#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#ifdef __linux__
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#define HAVE_USLEEP 1
#define HAVE_UNISTD_H 1

#if HAVE_NANOSLEEP && HAVE_TIME_H
#include <time.h>
#include <errno.h>
#elif HAVE_USLEEP && HAVE_UNISTD_H
#include <unistd.h>
#elif HAVE_SLEEP && HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "report_error.h"

/* #define OAL_PLAYBACK_ENABLED */

#define SAMPLERATE (44100)
#define DURATION   (100)
#define FORMAT     (AL_FORMAT_MONO8)

typedef struct {
	ALCcontext   *context;
	ALCdevice    *playbackDevice;
	ALCdevice    *captureDevice;
} OALstate;

typedef struct {
  ALCbyte *data;
  ALCsizei len;

  ALCuint freq;
  ALCenum format;
  /* ALCuint resolution; */
} OALSampleSet;

static OALstate state;

/* static void */
/* initSampleSet(SampleSet *ss, ALCuint freq, ); */

ALboolean OALSleep (ALfloat duration)
{
  ALuint seconds = (ALuint) duration;
  ALfloat rest = duration - (ALfloat) seconds;

#if HAVE_NANOSLEEP && HAVE_TIME_H

  ALuint microSecs = (ALuint) (rest * 1000000);
  struct timespec t, remainingTime;
  t.tv_sec = (time_t) seconds;
  t.tv_nsec = ((long) microSecs) * 1000;

  /* At least the interaction of nanosleep and signals is specified! */
  while (nanosleep (&t, &remainingTime) < 0)
    {
      if (errno != EINTR)
        {
          return AL_FALSE;
        }
      /* If we received a signal, let's try again with the remaining time. */
      t.tv_sec = remainingTime.tv_sec;
      t.tv_nsec = remainingTime.tv_nsec;
    }

#elif HAVE_USLEEP && HAVE_UNISTD_H

  while (seconds > 0)
    {
      usleep (1000000);
      seconds--;
    }
  usleep ((unsigned int) (rest * 1000000));

#elif HAVE_SLEEP && HAVE_WINDOWS_H

  while (seconds > 0)
    {
      Sleep (1000);
      seconds--;
    }
  Sleep ((DWORD) (rest * 1000));

#endif

  return AL_TRUE;
}

ALCsizei calcBufLen (ALCuint sr, ALuint du, ALCenum frmt)
{
  ALuint res;

  /*GUIDE:********************************************/
	/*  AL format:        Resolution:                  */
	/*  8  bit  mono   -> 1 byte  per sample           */
	/*  16 bit  mono   -> 2 bytes per sample           */
	/*  8  bit  stereo -> 2 bytes per sample           */
  /*  16 bit  stereo -> 4 bytes per sample           */
	/***************************************************/
  if (frmt == AL_FORMAT_MONO8)
    res = 1;
  else if (frmt == AL_FORMAT_MONO16 || frmt == AL_FORMAT_STEREO8)
    res = 2;
  else /* if (frmt == AL_FORMAT_STEREO16) */
    res = 4;

  return sr * du * res;
}

void InitState (ALCuint freq, ALCenum format, ALCsizei RingBufLen)
{
#ifdef OAL_PLAYBACK_ENABLED
	if (NULL == (state.playbackDevice =
               alcOpenDevice(NULL/*, ALC_DEFAULT_DEVICE_SPECIFIER*/)))
		utils_alcReportError(NULL, "Could not Open Playback Device");

  if (NULL ==
      (state.context = alcCreateContext(state.playbackDevice, NULL)))
    utils_alcReportError(state.playbackDevice,
                         "Could not Create an OpenAL context");

  if (ALC_TRUE != alcMakeContextCurrent(state.context))
    pputils_alcReportError(state.playbackDevice,
                         "Could not Switch to context");

  /* No Need for OpenAL 3D Spatialization. */
  alDistanceModel(AL_NONE);
#endif

  if (NULL == (state.captureDevice =
							 alcCaptureOpenDevice(NULL, freq, format, RingBufLen)))
    utils_alcReportError(NULL, "Could not Open Capture Device");

  return ;
}

void Record (OALSampleSet *sset, ALuint duration)
{
  ALuint totalSamples = calcBufLen(SAMPLERATE, duration, sset->format);
  ALCint nCapdSamples = 0;

  printf("totalSamples: %d\n", totalSamples);

	alcCaptureStart(state.captureDevice);
	/* do { */
	/* 	OALSleep(0.02); */
  /*   alcGetIntegerv(state.captureDevice, */
  /*                  ALC_CAPTURE_SAMPLES, */
  /*                  sizeof(nCapdSamples), */
  /*                  &nCapdSamples); */
	/* 	/\* printf("%d \n", nCapdSamples); *\/ */
	/* } while (nCapdSamples <= totalSamples); /\* EDIT (ALCint < ALuint) *\/ */

  OALSleep(duration);
  alcGetIntegerv(state.captureDevice,
                 ALC_CAPTURE_SAMPLES,
                 sizeof(nCapdSamples),
                 &nCapdSamples);

  alcCaptureStop(state.captureDevice);
	sset->len = nCapdSamples; /* ALCsizei <- ALCint */

  printf("sset->len: %d\n", sset->len);
	sset->data = (ALCbyte*) malloc(sset->len * sizeof(ALCbyte));
	/* Retrieve Samples and Pour alBufferLen of Them Into alBuffer*/
	alcCaptureSamples(state.captureDevice, sset->data, sset->len);
  return ;
}

#ifdef OAL_PLAYBACK_ENABLED
void Play (OALSampleSet *sset)
{
  ALuint alBuffer, alSource;
  ALenum source_state;

  alGenSources(1, &alSource);
	alGenBuffers(1, &alBuffer);
	/* Pour Retrieved Samples to The Generated Buffer, 3rd arg is const */
	/* 4th arg(ALsizei <- ALCsizei), 5th arg(ALsizei <- ALCuint) */
	alBufferData(alBuffer, sset->format, sset->data, sset->len, sset->freq);
	alSourcei(alSource, AL_BUFFER, alBuffer); /* 3rd arg(ALint <- ALuint) */

  puts("start play");
	alSourcePlay(alSource);
  OALSleep(sset->len / calcBufLen(SAMPLERATE, 1, AL_FORMAT_MONO8));

/*alGetSourcei(alSource, AL_SOURCE_STATE, &source_state);
  while (source_state == AL_PLAYING) {
    OALSleep(.1);
    alGetSourcei(alSource, AL_SOURCE_STATE, &source_state);
  }*/

  alSourceStop(alSource);

  puts("end play");

	alDeleteSources(1, &alSource);
	alDeleteBuffers(1, &alBuffer);
  return ;
}
#endif

void ErrorReport ()
{
  puts("Here are The Error Reports:");
	utils_alReportError("alGeterror: ");
#ifdef OAL_PLAYBACK_ENABLED
	utils_alcReportError(state.playbackDevice, "alcGeterror: ");
#endif
	utils_alcReportError(state.captureDevice, "alcGeterror: ");
	puts("End of Error Reports");
}

void DestroyState ()
{
#ifdef OAL_PLAYBACK_ENABLED
	alcMakeContextCurrent(NULL);
	alcDestroyContext(state.context);
	alcCloseDevice(state.playbackDevice);
  alcDestroyContext(state.context);
	alcCloseDevice(state.playbackDevice);
#endif
	alcCloseDevice(state.captureDevice);
  return ;
}

void WriteFile (OALSampleSet *sset)
{
  FILE *fp = fopen("audio.char", "w");
  int i;
  for (i = 0; i < sset->len; i++)
    putc(sset->data[i], fp);
  fclose(fp);
  return ;
}

/* int main(int argc, char *argv[]) */
/* { */
/*   OALSampleSet sampleset; */

/*   sampleset.format = FORMAT; */
/*   sampleset.freq   = SAMPLERATE; */

/*   InitState(SAMPLERATE, */
/*             FORMAT, */
/*             ((calcBufLen(SAMPLERATE, DURATION, AL_FORMAT_MONO8) / 2) * 3)); */
/* #ifdef OAL_PLAYBACK_ENABLED */
/*   alGetError(); */
/* 	alcGetError(state.playbackDevice); */
/* #endif */
/* 	alcGetError(state.captureDevice); */

/*   Record(&sampleset, DURATION); */

/*   /\* ErrorReport(&globalstate); *\/ */
/* #ifdef OAL_PLAYBACK_ENABLED */
/*   Play(&sampleset); */
/* #endif */
/*   /\* WriteFile(&sampleset); *\/ */

/*   free(sampleset.data); */
/*   DestroyState(); */
/*   return 0; */
/* } */

