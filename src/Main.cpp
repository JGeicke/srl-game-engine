#include "SDL.h"
#include "entitymanager.h"
#include "componentmanager.h"
#include "components/components.h"
#include "systems/rendersystem.h"
#include "systems/physicsystem.h"
#include "inputmanager.h"

int main(int argc, char* argv[]) {
	int framerate = 60;
	float frameDelay = (float)(1000 / framerate);

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window = SDL_CreateWindow("Testwindow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

	// TODO: Singleton Desingpattern for all managers and systems
	EntityManager* entityManager = new EntityManager();
	InputManager* inputManager = new InputManager();
	ComponentManager<Sprite>* spriteManager = new ComponentManager<Sprite>();
	ComponentManager<Position>* posManager = new ComponentManager<Position>();
	ComponentManager<Movement>* movementManager = new ComponentManager<Movement>();
	// TODO: special manager for single components in scene
	ComponentManager<CameraFollow>* cameraFollowManager = new ComponentManager<CameraFollow>();
	ComponentManager<Animator>* animatorManager = new ComponentManager<Animator>();

	RenderSystem* renderSystem = new RenderSystem(spriteManager, posManager, renderer, cameraFollowManager, animatorManager);
	renderSystem->setMap("../TestTextures/test_level_1.png", "../TestTextures/test_level_1.json", 2);
	// camera matches viewport
	renderSystem->initCamera(1280, 720);
	PhysicSystem* physicSystem = new PhysicSystem(inputManager, movementManager, posManager, spriteManager, animatorManager);

	Entity entity = entityManager->createEntity();

	Sprite* spriteComponent = spriteManager->addComponent(entity);
	spriteComponent->setEntity(entity);
	spriteComponent->init("../TestTextures/char_idle.png", 64, 64, 1.5f);
	//spriteComponent->init("../TestTextures/char_atk_side.png", 64, 64, 1.5f);
	//spriteComponent->init("../TestTextures/char_walk.png", 64, 64, 1.5f);

	Position* positionComponent = posManager->addComponent(entity);
	positionComponent->setEntity(entity);
	positionComponent->x = 250;
	positionComponent->y = 250;

	Movement* movementComponent = movementManager->addComponent(entity);
	movementComponent->setEntity(entity);
	movementComponent->setMovementSpeed(5);

	// camera follow target = player
	CameraFollow* cameraFollow = cameraFollowManager->addComponent(entity);
	cameraFollow->setEntity(entity);

	Animator* animator = animatorManager->addComponent(entity);
	animator->addAnimation("idle_side", 5, 160, FileLoader::loadTexture("../TestTextures/char_idle_side.png", renderer));
	animator->addAnimation("walk_side", 6, 100, FileLoader::loadTexture("../TestTextures/char_walk_side.png", renderer));
	animator->addAnimation("idle_up", 5, 160, FileLoader::loadTexture("../TestTextures/char_idle_up.png", renderer));
	animator->addAnimation("walk_up", 6, 100, FileLoader::loadTexture("../TestTextures/char_walk_up.png", renderer));
	animator->addAnimation("idle_down", 5, 160, FileLoader::loadTexture("../TestTextures/char_idle_down.png", renderer));
	animator->addAnimation("walk_down", 6, 100, FileLoader::loadTexture("../TestTextures/char_walk_down.png", renderer));
	//animator->addAnimation("idle", 6, 100);
	//animator->play("idle");
	//animator->play("attack");
	// test enemy
	Entity wizard = entityManager->createEntity();
	Sprite* wizardSprite = spriteManager->addComponent(wizard);
	wizardSprite->setEntity(wizard);
	wizardSprite->init("../TestTextures/wizard_idle.png", 64, 64, 1.0f);

	Position* wizardPosition = posManager->addComponent(wizard);
	wizardPosition->setEntity(wizard);
	wizardPosition->x = 240;
	wizardPosition->y = 250;

	Animator* wizardAnimator = animatorManager->addComponent(wizard);
	wizardAnimator->addAnimation("testing_name", 10, 150);

	while (!inputManager->interrupted)
	{
		Uint32 startTimestamp = SDL_GetTicks();

		inputManager->update();
		if (inputManager->interrupted) break;

		physicSystem->update();
		renderSystem->update();

		Uint32 endTimestamp = SDL_GetTicks();
		Uint32 delay = frameDelay - (endTimestamp - startTimestamp);
		SDL_Delay(delay);
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
