#pragma once
#include "SDL_ttf.h"
#include "SDL.h"
#include "util/ui/label.h"
#include "util/ui/panel.h"
#include "util/ui/progressbar.h"

class UIManager {
public:
	UIManager(SDL_Renderer* renderer);
	void addFont(const char* path, int fontSize);

	size_t addLabel(int x, int y, std::string text, SDL_Color color, size_t fontIndex);
	Label* getLabel(size_t index) {
		if (index < currentLabelIndex) {
			return &uiLabels[index];
		}
	}
	size_t getCurrentLabelIndex() {
		return currentLabelIndex;
	}


	size_t addPanel(const char* filePath, int x, int y, int w, int h, SDL_Color panelColor);
	size_t addPanel(int x, int y, int w, int h, SDL_Color panelColor);
	Panel* getPanel(size_t panelIndex) {
		if (panelIndex < currentPanelIndex) {
			return &uiPanels[panelIndex];
		}
	}
	size_t getCurrentPanelIndex() {
		return currentPanelIndex;
	}

	size_t addProgressBar(const char* bgFilePath, const char* progressFilePath, int x, int y, int w, int h, SDL_Color bgColor, SDL_Color progressColor);
	size_t addProgressBar(int x, int y, int w, int h, SDL_Color bgColor, SDL_Color progressColor);
	ProgressBar* getProgressBar(size_t progressBarIndex) {
		if (progressBarIndex < currentProgressBarIndex) {
			return &uiProgressBars[progressBarIndex];
		}
	}
	size_t getCurrentProgressBarIndex() {
		return currentProgressBarIndex;
	}
private:
	SDL_Renderer* renderer;

	TTF_Font* fonts[3];
	size_t currentFontIndex;

	Label uiLabels[32];
	size_t currentLabelIndex;

	Panel uiPanels[32];
	size_t currentPanelIndex;

	ProgressBar uiProgressBars[32];
	size_t currentProgressBarIndex;
};