#ifndef OPEN_AL___H_INCLUDED
#define OPEN_AL___H_INCLUDED


#define SAMPLERATE (44100)
#define DURATION   (100)
#define FORMAT     (AL_FORMAT_MONO8)


typedef struct {
  ALCbyte *data;
  ALCsizei len;

  ALCuint freq;
  ALCenum format;
  /* ALCuint resolution; */
} OALSampleSet;


ALCsizei calcBufLen (ALCuint, ALuint, ALCenum);
void InitState (ALCuint, ALCenum, ALCsizei);
void Record (OALSampleSet*, ALuint);
void Play (OALSampleSet*);
void ErrorReport ();
void DestroyState ();
void WriteFile (OALSampleSet*);

#endif
