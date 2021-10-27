#include "SDL.h"
#include "entitymanager.h"

int main(int argc, char* argv[]) {
	/*
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *window = SDL_CreateWindow("Testwindow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

	SDL_RenderClear(renderer);

	SDL_RenderPresent(renderer);

	SDL_Delay(3000);
	*/
	EntityManager *entityManager = new EntityManager();
	Entity e = entityManager->createEntity();
	entityManager->createEntity();
	entityManager->debugListEntities();
	entityManager->destroyEntity(e);
	entityManager->debugListEntities();
	return 0;
}