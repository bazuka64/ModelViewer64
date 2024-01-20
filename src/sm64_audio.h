#pragma once
#include "config.h"
#include "libsm64/libsm64.h"

using namespace std::chrono_literals;
SDL_AudioDeviceID dev;
std::thread* thread;

void audio_thread()
{
	double currentTime = glfwGetTime();
	double targetTime = 0;
	while (1)
	{
		int16_t audioBuffer[544 * 2 * 2];
		uint32_t numSamples = sm64_audio_tick(SDL_GetQueuedAudioSize(dev) / 4, 1100, audioBuffer);
		if (SDL_GetQueuedAudioSize(dev) / 4 < 6000)
			SDL_QueueAudio(dev, audioBuffer, numSamples * 2 * 4);

		targetTime = currentTime + 1.f / 30;
		while (glfwGetTime() < targetTime)
		{
			std::this_thread::sleep_for(100us);
		}
		currentTime = glfwGetTime();
	}
}

void audio_init()
{
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = 32000;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = 512;
	want.callback = NULL;
	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	SDL_PauseAudioDevice(dev, 0);

	thread = new std::thread(audio_thread);
}