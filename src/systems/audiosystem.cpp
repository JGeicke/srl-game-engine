#include "audiosystem.h"
/**
* @brief Constructor of the audio system.
* @param audioManager - Audio manager to access the audio components.
*/
AudioSystem::AudioSystem(ComponentManager<Audio>* audioManager) {
	this->audioManager = audioManager;
	this->bgm = NULL;

	this->masterVolume = .5f;
	this->soundVolume = 1.0f;
	this->musicVolume = 1.0f;
}
/**
* @brief Destructor of the audio system. Frees the allocated space of the bgm.
*/
AudioSystem::~AudioSystem() {
	Mix_FreeMusic(bgm);
}

/**
* @brief Audio system update loop. Iterates the audio components to check if the audio clips should be played.
*/
void AudioSystem::update() {
	unsigned int componentCount = audioManager->getComponentCount();
	for (size_t i = 0; i < componentCount; i++)
	{
		Audio* audioComponent = audioManager->getComponentWithIndex(i);
		if (audioComponent->getPlayedAudioClipsCount() > 0) {
			AudioClip* nextClip = audioComponent->getNextAudioClip();
			Mix_PlayChannel(-1, nextClip->getAudioChunk(), 0);
		}
	}
}

/**
* @brief Initializes the audio system.
*/
void AudioSystem::init() {
	// initializes SDL_mixer with frequency of 44100, default sample format, 2 hardware channels and 2048 byte chunk size
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Mixer Error", "Could not initialize SDL Mixer!", NULL);
	}
}

/**
* @brief Adds background music from file.
* @param filePath - Path to file.
*/
void AudioSystem::addBGM(const char* filePath) {
	bgm = Mix_LoadMUS(filePath);
	if (bgm == NULL) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Mixer Error", "Could not load background music file!", NULL);
	}
}

/**
* @brief Plays the current background music and adjusts the volume.
*/
void AudioSystem::playBGM() {
	Mix_PlayMusic(bgm, -1);
	Mix_VolumeMusic(MIX_MAX_VOLUME * (masterVolume*musicVolume));
}

/**
* @brief Plays audio clip and adjusts the volume of the audio channel.
* @param audio - Audio clip to be played.
*/
void AudioSystem::playSound(AudioClip* audio) {
	int channel = Mix_PlayChannel(-1, audio->getAudioChunk(), 0);
	Mix_Volume(channel, MIX_MAX_VOLUME * (masterVolume * soundVolume));
}