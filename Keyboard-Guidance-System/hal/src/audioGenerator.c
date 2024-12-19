// Note: Generates low latency audio on BeagleBone Black; higher latency found on host.

#include "shutdown.h"
#include "hal/audioGenerator.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <alloca.h> // needed for mixer

static snd_pcm_t *handle;

#define DEFAULT_VOLUME 80

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short)) 			// bytes per sample
// Sample size note: This works for mono files because each sample ("frame') is 1 value.
// If using stereo files then a frame would be two samples.

static unsigned long playbackBufferSize = 0;
static short *playbackBuffer = NULL;

// Currently active (waiting to be played) sound bites
#define MAX_SOUND_BITES 100
typedef struct {
	// A pointer to a previously allocated sound bite (wavedata_t struct).
	// Note that many different sound-bite slots could share the same pointer
	// (overlapping cymbal crashes, for example)
	wavedata_t *pSound;

	// The offset into the pData of pSound. Indicates how much of the
	// sound has already been played (and hence where to start playing next).
	int location;
} playbackSound_t;
static playbackSound_t soundBites[MAX_SOUND_BITES];
//Keep track of the number of sound bites in sound bites buffer
static int currentSoundBite = 0;

// Playback threading
void* playbackThread(void * arg);
static bool stopping = false;
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

static int volume = 0;

void audioGenerator_init(void)
{
	// audioGenerator_setVolume(DEFAULT_VOLUME);
	audioGenerator_setVolume(100);

    for (int i = 0; i < MAX_SOUND_BITES; i++){
        soundBites[i].pSound = NULL;
        soundBites[i].location = 0;
    }

	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	// Configure parameters of PCM output
	err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			NUM_CHANNELS,
			SAMPLE_RATE,
			1,			// Allow software resampling
			50000);		// 0.05 seconds per buffer
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	// Allocate this software's playback buffer to be the same size as the
	// the hardware's playback buffers for efficient data transfers.
	// ..get info on the hardware buffers:
 	unsigned long unusedBufferSize = 0;
	snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);
	// ..allocate playback buffer:
	playbackBuffer = malloc(playbackBufferSize * sizeof(*playbackBuffer));

	// Launch playback thread:
	pthread_create(&playbackThreadId, NULL, &playbackThread, NULL);
}

// Client code must call audioGenerator_freeWaveFileData to free dynamically allocated data.
void audioGenerator_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound)
{
	assert(pSound);

	// The PCM data in a wave file starts after the header:
	const int PCM_DATA_OFFSET = 44;

	// Open the wave file
	FILE *file = fopen(fileName, "r");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
		exit(EXIT_FAILURE);
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	int sizeInBytes = ftell(file) - PCM_DATA_OFFSET;
	pSound->numSamples = sizeInBytes / SAMPLE_SIZE;

	// Search to the start of the data in the file
	fseek(file, PCM_DATA_OFFSET, SEEK_SET);

	// Allocate space to hold all PCM data
	pSound->pData = malloc(sizeInBytes);
	if (pSound->pData == 0) {
		fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
				sizeInBytes, fileName);
		exit(EXIT_FAILURE);
	}

	// Read PCM data from wave file into memory
	int samplesRead = fread(pSound->pData, SAMPLE_SIZE, pSound->numSamples, file);
	if (samplesRead != pSound->numSamples) {
		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
				pSound->numSamples, fileName, samplesRead);
		exit(EXIT_FAILURE);
	}
}

void audioGenerator_freeWaveFileData(wavedata_t *pSound)
{
	pSound->numSamples = 0;
	free(pSound->pData);
	pSound->pData = NULL;
}

void audioGenerator_queueSound(wavedata_t *pSound)
{
	assert(pSound->numSamples > 0);
	assert(pSound->pData);

	pthread_mutex_lock(&audioMutex);

	bool freeSlots = false;

	// search thru soundBites array looking for free slot
	for (int i = 0; i < MAX_SOUND_BITES; i++){
		// if there is free slot, place new sound file in slot
		if (soundBites[i].pSound == NULL){
			soundBites[i].pSound = pSound;
			soundBites[i].location = 0;
			// Indicate that we found a free spot
			freeSlots = true;
			break;
		}
	}

	pthread_mutex_unlock(&audioMutex);

	if (freeSlots == false){
		printf("No free slots in soundBites, could not queue sound. \n");
		return;
	}
}

void audioGenerator_cleanup(void)
{
	// Stop the PCM generation thread
	stopping = true;
	pthread_join(playbackThreadId, NULL);

	// Shutdown the PCM output, allowing any pending sound to play out (drain)
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	// Free playback buffer
	// (note that any wave files read into wavedata_t records must be freed
	//  in addition to this by calling audioGenerator_freeWaveFileData() on that struct.)
	// Running through each sound bites to clear it 
	 for (int i = 0; i < currentSoundBite; i++) {
		audioGenerator_freeWaveFileData(soundBites[i].pSound);
        soundBites[i].pSound = NULL;
		soundBites[i].location = 0;
    }
	currentSoundBite = 0;
	free(playbackBuffer);
	playbackBuffer = NULL;

	fflush(stdout);
	printf("Audio Mixer cleanup done!\n");
	
}


int audioGenerator_getVolume()
{
	// Return the cached volume; good enough unless someone is changing
	// the volume through other means and the cached value is out of date.
	return volume;
}

void audioGenerator_increaseVolumeFive()
{
	if (volume + 5 > MAX_AUDIO_VOLUME) {
		return;
	} else {
		volume += 5;
		audioGenerator_setVolume(volume);
	}
}

void audioGenerator_decreaseVolumeFive()
{
	if (volume - 5 < 0) {
		return;
	} else {
		volume -= 5;
		audioGenerator_setVolume(volume);
	}
}

// Function copied from:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
// Written by user "trenki".
void audioGenerator_setVolume(int newVolume)
{
	// Ensure volume is reasonable; If so, cache it for later getVolume() calls.
	if (newVolume < 0 || newVolume > MAX_AUDIO_VOLUME) {
		printf("ERROR: Volume must be between 0 and 100.\n");
		return;
	}
	volume = newVolume;
 
    long min, max;
    snd_mixer_t *volHandle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "PCM";

    snd_mixer_open(&volHandle, 0);
    snd_mixer_attach(volHandle, card);
    snd_mixer_selem_register(volHandle, NULL, NULL);
    snd_mixer_load(volHandle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(volHandle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(volHandle);
}


// Fill the `buff` array with new PCM values to output.
//    `buff`: buffer to fill with new PCM data from sound bites.
//    `size`: the number of values to store into playbackBuffer
static void fillPlaybackBuffer(short *buff, int size)
{
	// wipe playback buffer
	memset(buff, 0, size * sizeof((short)*buff));
	// make sure soundBites array is threadsafe
	pthread_mutex_lock(&audioMutex);

	// loop thru soundBites arr
	for (int i = 0; i < MAX_SOUND_BITES; i++){
		// if slot is unused, do nothing
		if(soundBites[i].pSound == NULL){
			continue;
		}

		// otherwise add sound bite's data to buffer if there is a sound in the soundBites slot
		// load current sound bite into local variable
		wavedata_t *currSoundBite = soundBites[i].pSound;
		// get the current position of offset, where to start playing next
		int offset = soundBites[i].location;

		// loop thru the current sound bite
		// size is the number of values to store in the current buffer
		for(int j = 0; j < size; j++){
			// make sure to not overflow playbackBuffer

			// offset is supposed to be the current location of the sound on the soundBite
			// goal is to use offset location to look at where to place the next 'bite'
			// size is the size of the buffer to be filled in, always seems to be 735 bites
			// so if we do comparison if (offset >= size), it will be true very early, since offset seems to be multiplse of 735
			if (offset >= currSoundBite->numSamples){
				break;
			}

			// get the current value of the soundBite at offset location and check to see if we can add it to buffer
			// using pData[offset] since offset shows location of soundbite 
			int tempValue = ((int)buff[j] + currSoundBite->pData[offset]);

			// check for overflow
			if (tempValue > SHRT_MAX){
				tempValue = SHRT_MAX;
			}
			else if (tempValue < SHRT_MIN){
				tempValue = SHRT_MIN;
			}

			// add new soundbite to playbackBuffer and increment offset
			buff[j] = tempValue;
			offset++;
		}
		// update new location to show this portion of the sound bite has been played back 
		soundBites[i].location = offset;

		// free slot in soundBites arr if whole sample played
		if (soundBites[i].location >= currSoundBite->numSamples){
			// printf("freeing slot in soundBites\n");
			soundBites[i].pSound = NULL;
		}
	}

	pthread_mutex_unlock(&audioMutex);
}

void audioGenerator_clearSound(void) {
    pthread_mutex_lock(&audioMutex); 

    // Iterate through the soundBites array and set each element to NULL
    for (int i = 0; i < currentSoundBite; i++) {
        soundBites[i].pSound = NULL;
		soundBites[i].location = 0;
    }
	currentSoundBite = 0;
    pthread_mutex_unlock(&audioMutex); 
}


void* playbackThread(void * arg)
{

	while(!Shutdown_isShutdown()) {
		// Generate next block of audio
		fillPlaybackBuffer(playbackBuffer, playbackBufferSize);

		// Output the audio
		snd_pcm_sframes_t frames = snd_pcm_writei(handle,
				playbackBuffer, playbackBufferSize);

		// Check for (and handle) possible error conditions on output
		if (frames < 0) {
			fprintf(stderr, "audioGenerator: writei() returned %li\n", frames);
			frames = snd_pcm_recover(handle, frames, 1);
		}
		if (frames < 0) {
			fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n",
					frames);
			exit(EXIT_FAILURE);
		}
		if (frames > 0 && frames < (long int) playbackBufferSize) {
			printf("Short write (expected %li, wrote %li)\n",
					playbackBufferSize, frames);
		}

	}

	return arg;
}

