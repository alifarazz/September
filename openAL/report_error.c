#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#ifdef __linux__
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/alut.h>
#endif

void utils_alReportError(const char * errorString)
{
	ALenum error;
	if (AL_NO_ERROR != (error = alGetError())) {
		fprintf(stderr, "> %s: %s\n", errorString, alGetString(error));
	}
	return ;
}

void utils_alcReportError(ALCdevice *device, const char * errorString)
{
	ALenum error;
	if (AL_NO_ERROR != (error = alcGetError(device))) {
		fprintf(stderr, "> %s: %s\n", errorString, alcGetString(device, error));
	}
	return ;
}

