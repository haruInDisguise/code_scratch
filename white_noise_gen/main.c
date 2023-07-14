#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL.h>

#define SDL_ASSERT(condition)                                                                      \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "SDL error: %s\n", SDL_GetError());                                    \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

#define AMPLITUDE 28000
#define FREQUENCY 48000

typedef struct {
    Sint16 (*sample_func)(Uint32 index, Uint32 length);
    Uint32 index;
} AudioSampler;

static Sint16 white_noise_sample(Uint32 index, Uint32 length) {
    Uint8 bit_depth = 11;
    int rand_value = rand() & ((Uint64)(-1) >> (64 - bit_depth));
    int sign = (rand_value & 1) * 2 - 1;

    return rand_value * sign;
}

void audio_callback(void *userdata, Uint8 *raw_stream, int raw_length) {
    AudioSampler *sampler = (AudioSampler *)userdata;
    Sint16 *stream = (Sint16 *)raw_stream;
    int length = raw_length / sizeof(Sint16);

    for (Uint32 i = 0; i < length; i++, sampler->index++) {
        // int rand_value = FREQUENCY * (rand() % FREQUENCY) / FREQUENCY;
        // stream[i] = (Sint16)(AMPLITUDE * (sin(2.0f * M_PI * target_frequency * sample_index *
        // rand_value / 44100) ));
        stream[i] = sampler->sample_func(sampler->index, length);
    }
}

int main(int argc, char **argv) {
    SDL_Init(SDL_INIT_AUDIO);

    AudioSampler sampler = {
        .sample_func = white_noise_sample,
    };

    SDL_AudioSpec obtained_spec, desired_spec = {
                                     .channels = 1,
                                     .freq = FREQUENCY, /* Samples per second (pitch) */
                                     .format = AUDIO_S16,
                                     .samples = 1024,
                                     .callback = audio_callback,
                                     .userdata = &sampler,
                                 };


    const char *audio_device_name = SDL_GetAudioDeviceName(0, 0);
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(audio_device_name, 0, &desired_spec,
                                                   &obtained_spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    SDL_ASSERT(device > 0);
    srand(time(NULL));

    printf("Using audio device: %s\n", audio_device_name);
    printf("Buffer length: %u\n", obtained_spec.size);

    SDL_PauseAudioDevice(device, 0);

    // fgetc(stdin);
    SDL_Delay(1000);

    SDL_PauseAudioDevice(device, 1);

    SDL_CloseAudioDevice(device);

    printf("Index: %u\n", sampler.index);

    return 0;
}
