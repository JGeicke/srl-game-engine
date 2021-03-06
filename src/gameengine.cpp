#include "gameengine.h"
#pragma region Lifecycle
/**
* @brief Initializes the game engine.
* @param fps - Target frames per seconds of game loop.
* @param windowTitle - Title of game window.
* @param windowWidth - Width of game window.
* @param windowHeight - Height of game window.
* @param cameraWidth - Width of camera.
* @param cameraHeight - Height of camera.
* @param iconFilePath - Filepath to icon.
* @param debug- Display position, collider and paths for debugging.
*/
void GameEngine::init(int fps, std::string windowTitle, int width, int height, int cameraWidth, int cameraHeight, const char* iconFilePath, bool debug) {
	if (fps > 0) {
		this->frameDelay = 1000 / fps;
	}
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization error", "Could not initialize game engine.", NULL);
		return;
	}
	if (TTF_Init() != 0) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization error", "Could not initialize SDL_TTF.", NULL);
		return;
	}
	this->window = new Window(windowTitle, width, height);
	if(window->initWindow()){
		// set window icon
		window->setWindowIcon(iconFilePath);

		this->initManagers();
		this->initComponentManagers();
		this->initUniqueComponents();
		this->initObjectPools();
		this->initSystems(cameraWidth, cameraHeight, debug);
	}
}

/**
* @brief Runs the game engine. Starts the game loop.
*/
void GameEngine::run() {
	if (window == nullptr) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Runtime error", "Please initialize engine before executing run.", NULL);
		return;
	}

	while (true)
	{
		Uint32 startTimestamp = SDL_GetTicks();

		inputManager->update();
		if (inputManager->interrupted) break;

		uiManager->update();

		physicSystem->update();

		audioSystem->update();
		renderSystem->update();

		collectObjects();
		Uint32 endTimestamp = SDL_GetTicks();
		Uint32 delay = (Uint32)(frameDelay - (endTimestamp - startTimestamp));
		if (delay < frameDelay) {
			SDL_Delay(delay);
		}
	}
}

/**
* @brief Quits the game engine and used libraries.
*/
void GameEngine::quit() {
	inputManager->interrupted = true;
	TTF_Quit();
	SDL_Quit();
}

/**
* @brief Calculates the angle of the vector a->b in the unit circle.
* @param a - Point a
* @param b - Point b
* @return Angle of vector a->b
*/
double GameEngine::calcAngle(SDL_Point a, SDL_Point b) {
	int deltaX = b.x - a.x;
	int deltaY = b.y - a.y;

	double angle = (std::atan2(deltaY, deltaX) * 180) / M_PI;
	return angle;
}

#pragma endregion Lifecycle
/**
* @brief Adds an entity.
* @param tag - Tag of entity
* @param isPreserved -  Whether the entity is preserved across scenes.
* @param position - Position of the entity.
* @return Added entity.
*/
Entity GameEngine::addEntity(const char* tag, bool isPreserved, SDL_Point position) {
	Entity entity = entityManager->createEntity(tag, isPreserved);
	Position* pos = this->posManager->addComponent(entity);
	if (pos != nullptr) {
		pos->setEntity(entity);
		pos->setPosition(position.x*renderSystem->getCameraZoomFactorX(), position.y*renderSystem->getCameraZoomFactorY());
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Entity Initialization error", "Could not add position component to entity.", NULL);
	}
	return entity;
}

/**
* @brief Sets the position of the entity.
* @param e - Entity to set position of.
* @param pos - New position of entity.
*/
void GameEngine::setPosition(Entity e, SDL_Point pos) {
	Position* component = this->posManager->getComponent(e);
	if (component != nullptr) {
		component->setPosition(pos.x * renderSystem->getCameraZoomFactorX(), pos.y * renderSystem->getCameraZoomFactorY());
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Entity Initialization error", "Could not add position component to entity.", NULL);
	}
}

/**
* @brief Destroys the entity and its component.
* @param e - Entity to destroy.
*/
void GameEngine::destroyEntity(Entity e) {
	this->removeEntityComponents(e);
	this->entityManager->destroyEntity(e);
}

/**
* @brief Sets the entity that the camera follows.
* @param e - Entity to follow.
*/
void GameEngine::setCameraFollowTarget(Entity e) {
	this->cameraFollow->setEntity(e);
	this->renderSystem->setCameraFollowTarget(cameraFollow);
}

/**
* @brief Resets the entity that the camera follows.
*/
void GameEngine::resetCameraFollowTarget() {
	this->setCameraFollowTarget({ 0 });
}

/**
* @brief Adds player entity. The player entity acts like a regular entity but is moveable.
* @param tag - Tag of entity
* @param isPreserved -  Whether the entity is preserved across scenes.
* @param position - Position of the player entity.
* @param movementSpeed - Movement speed of the player entity.
* @return The player entity.
*/
Entity GameEngine::addPlayer(const char* tag, bool isPreserved, SDL_Point position, unsigned int movementSpeed) {
	Entity player = addEntity(tag, isPreserved, position);
	this->playerMovement->setEntity(player);
	this->playerMovement->setMovementSpeed(movementSpeed);
	return player;
}

/**
* @brief Creates new projectile from projectile object pool.
* @param spritePath - File path to sprite.
* @param size - Size of sprite.
* @param scale - Scale of sprite.
* @param start  - Start position.
* @param target - Target position.
* @param projectileSpeed - Projectile speed.
* @param isCursorTarget - Whether the cursor position is the target position.
* @return Projectile entity.
*/
Entity GameEngine::createProjectile(const char* spritePath, SDL_Point size, float scale, SDL_Point start, SDL_Point target, float projectileSpeed, bool isCursorTarget) {
	Entity result = this->projectilePool->getNext();
	if (result.uid != 0) {
		Position* position = this->posManager->getComponent(result);
		position->setPosition(start.x, start.y);

		// adjust or create sprite component
		if (!this->spriteManager->hasComponent(result)) {
			this->addSpriteComponent(result, spritePath, size, scale);
		}
		else {
			Sprite* sprite = this->spriteManager->getComponent(result);
			sprite->init(spritePath, size.x, size.y, scale);
			sprite->setActive(true);
		}
		// adjust or create collider component
		if (!this->colliderManager->hasComponent(result)) {
			this->addColliderComponent(result, { 0,0 }, size, true);
		}
		else {
			Collider* collider = this->colliderManager->getComponent(result);
			collider->init(start.x, start.y, 0, 0, size.x, size.y, true);
			collider->resetLastCollision();
			collider->setActive(true);
		}

		// adjust or create projectile movement component
		if (!this->projectileMovementManager->hasComponent(result)) {
			this->addProjectileMovementComponent(result, start, target, projectileSpeed, isCursorTarget);
		}
		else {
			ProjectileMovement* projectileMovement = this->projectileMovementManager->getComponent(result);
			double angle = (isCursorTarget) ? this->calcAngle({ start.x - renderSystem->getCameraX(),start.y - renderSystem->getCameraY() }, target) : this->calcAngle(start, target);
			projectileMovement->init(angle, projectileSpeed);
			projectileMovement->setActive(true);
		}
	}
	return result;
}

/**
* @brief Destroys projectile of projectile object pool.
* @param e - Entity to destroy.
*/
void GameEngine::destroyProjectile(Entity e) {
	if (this->projectilePool->collect(e)) {
		Sprite* sprite = this->spriteManager->getComponent(e);
		sprite->setActive(false);

		ProjectileMovement* projectileMovement = this->projectileMovementManager->getComponent(e);
		projectileMovement->setActive(false);

		Collider* collider = this->colliderManager->getComponent(e);
		collider->setActive(false);
		collider->resetLastCollision();
	}
}

/**
* @brief Sets the current tilemap.
* @param tilesetFilePath - File path to the used tileset.
* @param tilemapDataFilePath - File path to the data of the tilemap.
* @param layerCount - Number of layers in the tilemap.
*/
void GameEngine::setTilemap(const char* tilesetFilePath, const char* tilemapDataFilePath, size_t layerCount) {
	if (tilemapDataFilePath == nullptr) {
		return;
	}

	Tilemap* tilemap = renderSystem->setMap(tilesetFilePath, tilemapDataFilePath, layerCount);

	// create collision objects
	if (tilemap->hasCollisionLayer()) {
		std::vector<SDL_Rect> colLayer = tilemap->getTilemapCollider();


		for (size_t i = 0; i < colLayer.size(); i++) {
			// checks if current x-position is greater or equals to max tilecount in row
			int newX = colLayer[i].x + (colLayer[i].w / 2);
			int newY = colLayer[i].y + (colLayer[i].h / 2);

			Entity e = this->addEntity("", false,{ newX, newY });
			this->addColliderComponent(e, { 0,0 }, { colLayer[i].w, colLayer[i].h}, false);
		}
	}

	// create tilemap objects
	if (tilemap->hasTilemapObjectLayer()) {
		std::vector<SDL_Rect> objLayer = tilemap->getTilemapObjects();

		int tileHeight = tilemap->getTileHeight();
		int tileWidth = tilemap->getTileWidth();
		Tileset* tileset = this->renderSystem->getTileset();
		unsigned int tilesetWidth = tileset->getTexture().textureWidth;

		for (size_t i = 0; i < objLayer.size(); i++) {
			// checks if current x-position is greater or equals to max tilecount in row
			int newX = objLayer[i].x  + (objLayer[i].w / 2);
			int newY = objLayer[i].y + (objLayer[i].h / 2);

			int idxOffsetX = objLayer[i].x / tileWidth;

			int dataIndex = (objLayer[i].y / tileHeight)*tilemap->getTilesPerRow() + idxOffsetX;
			int tilemapData = tilemap->getLayer(tilemap->getLayerCount() - 1)[dataIndex];

			int srcX = 0;
			int srcY = 0;
			// check if first row tileWidth * maxWidth
			if ((tilemapData * tileWidth) <= (tilesetWidth)) {
				srcX = (tilemapData == 0) ? tilemapData * tileWidth : (tilemapData - 1) * tileWidth;
			}
			else {
				// calculate row
				unsigned int yoffset = (tilemapData * tileWidth) / (tilesetWidth);
				unsigned int xoffset = ((tilemapData * tileWidth) % (tilesetWidth)) / tileWidth;

				srcX = (xoffset == 0) ? xoffset * tileWidth : (xoffset - 1) * tileWidth;
				srcY = yoffset * tileHeight;
			}

			Entity e = this->addEntity("", false, { newX, newY });
			Sprite* sprite = this->addSpriteComponent(e, "", {srcX, srcY}, { objLayer[i].w, objLayer[i].h }, 1.0f);
			sprite->setTexture(tileset->getTexture());
		}
	}
}

/**
* @brief Sets the current background music.
* @param bgmFilePath - File path to the background music.
* @param loopBGM - Whether bgm should be looped.
*/
void GameEngine::setBGM(const char* bgmFilePath, bool loopBGM) {
	if (bgmFilePath == nullptr) {
		return;
	}

	audioSystem->addBGM(bgmFilePath);
	audioSystem->playBGM(loopBGM);
}

/**
* @brief Plays audio clip with index of entity.
* @param e - Entity to play audio clip of.
* @param index - Index of audio clip.
*/
void GameEngine::playAudioClip(Entity e, size_t index) {
	Audio* audio = this->getAudioComponent(e);
	if (audio != nullptr) {
		AudioClip* clip = nullptr;
		clip = audio->getAudioClip(index);
		if (clip != nullptr) {
			this->audioSystem->playSound(clip);
		}
	}
}

/**
* @brief Plays audio file. This is needed for elements outside of the ecs to be able to use the audio system.
* @param path - Path to audio file.
*/
void GameEngine::playAudioFile(const char* path) {
	AudioClip* clip = new AudioClip(path);
	if (clip != nullptr) {
		this->audioSystem->playSound(clip);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Audio error", "Could not play audio clip.", NULL);
	}
}

/**
* @brief Sets the destination of the enemy.
* @param e - Entity to set the destination from.
* @param pos - Destination position.
*/
void GameEngine::setEnemyDestination(Entity e, Position* pos) {
	Node* dest = this->physicSystem->getCurrentNode(pos);
	if (dest != nullptr) {
		EnemyMovement* component = enemyMovementManager->getComponent(e);
		if (component != nullptr) {
			component->setDestination(dest);
			component->flag(true);
		}
	}
}

/**
* @brief Adds a sprite component to the entity.
* @param e - Entity to add component to
* @param filePath - File path to the sprite file.
* @param size - SDL_Point(width, height) representing the texture size.
* @param scale - Scale of the texture.
* @return Pointer to the added sprite component.
*/
Sprite* GameEngine::addSpriteComponent(Entity e, const char* filePath, SDL_Point size, float scale) {
	Sprite* spriteComponent = spriteManager->addComponent(e);
	if (spriteComponent != nullptr) {
		spriteComponent->setEntity(e);
		spriteComponent->init(filePath, size.x, size.y, scale);
		spriteComponent->setActive(true);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add sprite component.", NULL);
	}
	return spriteComponent;
}

/**
* @brief Adds a sprite component to the entity with custom source rectangle.
* @param e - Entity to add component to
* @param filePath - File path to the sprite file.
* @param srcRectPosition - Start position of source rectangle.
* @param size - SDL_Point(width, height) representing the texture size.
* @param scale - Scale of the texture.
* @return Pointer to the added sprite component.
	*/
Sprite* GameEngine::addSpriteComponent(Entity e, const char* filePath, SDL_Point srcRectPosition, SDL_Point size, float scale) {
	Sprite* spriteComponent = spriteManager->addComponent(e);
	if (spriteComponent != nullptr) {
		spriteComponent->setEntity(e);
		spriteComponent->init(filePath,srcRectPosition.x, srcRectPosition.y, size.x, size.y, scale);
		spriteComponent->setActive(true);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add sprite component.", NULL);
	}
	return spriteComponent;
}

/**
* @brief Adds a collider component to the entity.
* @param e - Entity to add component to
* @param offset - SDL_Point(XOffset, YOffset). Offset of the collider. Relative to the entities position.
* @param size - SDL_Point(width, height) representing the size of the collider.
* @param isTrigger - Whether the collider is a trigger.
* @return Pointer to the added collider component.
*/
Collider* GameEngine::addColliderComponent(Entity e, SDL_Point offset, SDL_Point size, bool isTrigger) {
	Collider* colliderComponent = colliderManager->addComponent(e);
	if (colliderComponent != nullptr) {
		colliderComponent->setEntity(e);
		Position* positionComponent = posManager->getComponent(e);
		colliderComponent->init(positionComponent->x(), positionComponent->y(), offset.x, offset.y, (int)(size.x*renderSystem->getCameraZoomFactorX()), (int)(size.y*renderSystem->getCameraZoomFactorY()), isTrigger);
		colliderComponent->setActive(true);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add collider component.", NULL);
	}
	return colliderComponent;
}

/**
* @brief Adds an audio component to the entity.
* @param e - Entity to add component to.
* @return Pointer to the added audio component.
*/
Audio* GameEngine::addAudioComponent(Entity e) {
	Audio* audioComponent = audioManager->addComponent(e);
	if (audioComponent != nullptr) {
		audioComponent->setEntity(e);
		audioComponent->resetComponent();
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add audio component.", NULL);
	}
	return audioComponent;
}


/**
* @brief Adds audio clip to entities audio component.
* @param e - Entity of the audio component.
* @param filePath - File path to the audio file.
* @return Index of the added audio clip.
*/
size_t GameEngine::addAudioClip(Entity e, const char* filePath) {
	Audio* audioComponent = audioManager->getComponent(e);
	if (audioComponent != nullptr) {
		size_t audioIndex = audioComponent->addAudioClip(filePath);
		return audioIndex;
	}
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "AudioClip Initialization error", "Could not add audio clip to component.", NULL);
	return SIZE_MAX;
}

/**
* @brief Adds an animator component to the entity.
* @param e - Entity to add component to.
* @return Pointer to the added animator component.
*/
Animator* GameEngine::addAnimatorComponent(Entity e) {
	Animator* animator = animatorManager->addComponent(e);
	if (animator != nullptr) {
		animator->setEntity(e);
		animator->resetComponent();
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add animator component.", NULL);
	}
	return animator;
}

/**
* @brief Adds animation to entites animator component.
* @param e - Entity of the animator component.
* @param animationState - Animation state that triggers the animation.
* @param frames - Number of frames in the animation.
* @param frameDelayMS - Delay between the frames in MS.
* @param filePath - File path to the animation texture.
*/
void GameEngine::addAnimation(Entity e, size_t animationState, int frames, int frameDelayMS, const char* filePath) {
	Animator* animator = animatorManager->getComponent(e);
	animator->addAnimation(animationState, frames, frameDelayMS, FileLoader::loadTexture(filePath, this->window->getRenderer()));
}

/**
* @brief Adds animation to entites animator component.
* @param e - Entity of the animator component.
* @param animationState - Animation state that triggers the animation.
* @param frames - Number of frames in the animation.
* @param frameDelayMS - Delay between the frames in MS.
*/
void GameEngine::addAnimation(Entity e, size_t animationState, int frames, int frameDelayMS) {
	Animator* animator = animatorManager->getComponent(e);
	animator->addAnimation(animationState, frames, frameDelayMS);
}

/**
* @brief Adds a health component to the entity.
* @param e - Entity to add component to.
* @param maximumHealth - Maximum health value.
* @return Pointer to the added health component.
*/
Health* GameEngine::addHealthComponent(Entity e, int maximumHealth) {
	Health* health = healthManager->addComponent(e);
	if (health != nullptr) {
		health->setEntity(e);
		health->init(maximumHealth);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add health component.", NULL);
	}
	return health;
}

/**
* @brief Adds a projectile movement component to the entity.
* @param e - Entity to add component to.
* @param start - Start position.
* @param target- Target position.
* @param projectileSpeed - Projectile speed.
* @param isCursorTarget - Whether the target position is the position of the mouse cursor in the window.
* @return Pointer to the added projectile movement component.
*/
ProjectileMovement* GameEngine::addProjectileMovementComponent(Entity e, SDL_Point start, SDL_Point target, float projectileSpeed, bool isCursorTarget) {
	ProjectileMovement* component = projectileMovementManager->addComponent(e);
	if (component != nullptr) {
		component->setEntity(e);
		component->setActive(true);

		double angle = (isCursorTarget) ? this->calcAngle({start.x-renderSystem->getCameraX(),start.y-renderSystem->getCameraY()}, target) : this->calcAngle(start, target);
		component->init(angle, projectileSpeed);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add projectile movement component.", NULL);
	}
	return component;
}

/**
 * @brief Adds a enemy movement component to the entity.
 * @param e - Entity to add component to.
 * @param movementSpeed - Movement speed of the entity.
 * @return Pointer to the added enemy movement component.
*/
EnemyMovement* GameEngine::addEnemyMovementComponent(Entity e, float movementSpeed) {
	EnemyMovement* component = enemyMovementManager->addComponent(e);
	if (component != nullptr) {
		component->setEntity(e);
		component->setMovementSpeed(movementSpeed);
		component->resetComponent();
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add enemy movement component.", NULL);
	}
	return component;
}

/**
* @brief Adds a enemy movement component to the entity.
* @param e - Entity to add component to.
* @param movementSpeed - Movement speed of the entity.
* @param pathfindingTimerMS - Timer how often the path is calculated.
* @return Pointer to the added enemy movement component.
*/
EnemyMovement* GameEngine::addEnemyMovementComponent(Entity e, float movementSpeed, int pathfindingTimerMS) {
	EnemyMovement* component = this->addEnemyMovementComponent(e, movementSpeed);
	if (component != nullptr) {
		component->setPathfindingTimer(pathfindingTimerMS);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add enemy movement component.", NULL);
	}
	return component;
}

/**
* @brief Adds a enemy movement component to the entity.
* @param e - Entity to add component to.
* @param movementSpeed - Movement speed of the entity.
* @param pathfindingTimerMS - Timer how often the path is calculated.
* @param target - Target Entity.
* @return Pointer to the added enemy movement component.
*/
EnemyMovement* GameEngine::addEnemyMovementComponent(Entity e, float movementSpeed, Entity target) {
	EnemyMovement* component = this->addEnemyMovementComponent(e, movementSpeed);
	if (component != nullptr) {
		component->setTarget(target);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add enemy movement component.", NULL);
	}
	return component;
}

/**
* @brief Adds a enemy movement component to the entity.
* @param e - Entity to add component to.
* @param movementSpeed - Movement speed of the entity.
* @param pathfindingTimerMS - Timer how often the path is calculated.
* @param target - Target Entity.
* @return Pointer to the added enemy movement component.
*/
EnemyMovement* GameEngine::addEnemyMovementComponent(Entity e, float movementSpeed, int pathfindingTimerMS, Entity target) {
	EnemyMovement* component = this->addEnemyMovementComponent(e, movementSpeed);
	if (component != nullptr) {
		component->setPathfindingTimer(pathfindingTimerMS);
		component->setTarget(target);
	}
	else {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Component Initialization error", "Could not add enemy movement component.", NULL);
	}
	return component;
}

#pragma region Getters
/**
* @brief Gets position component of the entity.
* @param e - Entity to get component off.
* @return Pointer to the position component of the entity.
*/
Position* GameEngine::getPositionComponent(Entity e){
	Position* result = this->posManager->getComponent(e);
	return result;
}

/**
* @brief Gets sprite component of the entity.
* @param e - Entity to get component off.
* @return Pointer to the sprite component of the entity.
*/
Sprite* GameEngine::getSpriteComponent(Entity e) {
	Sprite* result = this->spriteManager->getComponent(e);
	return result;
}

/**
* @brief Gets collider component of the entity.
* @param e - Entity to get component off.
* @return Pointer to the collider component of the entity.
*/
Collider* GameEngine::getColliderComponent(Entity e){
	Collider* result = this->colliderManager->getComponent(e);
	return result;
}

/**
* @brief Gets audio component of the entity.
* @param e - Entity to get component off.
* @return Pointer to the audio component of the entity.
*/
Audio* GameEngine::getAudioComponent(Entity e){
	Audio* result = this->audioManager->getComponent(e);
	return result;
}

/**
* @brief Gets animator component of the entity.
* @param e - Entity to get component off.
* @return Pointer to the animator component of the entity.
*/
Animator* GameEngine::getAnimatorComponent(Entity e){
	Animator* result = this->animatorManager->getComponent(e);
	return result;
}

/**
* @brief Gets movement component of the player entity.
* @param e - Entity to get component off.
* @return Pointer to the movement component of the player entity.
*/
Movement* GameEngine::getPlayerMovementComponent(Entity e){
	return playerMovement;
}

/**
* @brief Gets health component of the entity.
* @param e - Entity to get component off.
* @return Pointer to the health component of the entity.
*/
Health* GameEngine::getHealthComponent(Entity e) {
	Health* result = this->healthManager->getComponent(e);
	return result;
}

/**
* @brief Gets enemy movement component of the entity.
* @param e - Entity to get component off.
* @return Pointer to the enemy movement component of the entity.
*/
EnemyMovement* GameEngine::getEnemyMovementComponent(Entity e){
	EnemyMovement* result = this->enemyMovementManager->getComponent(e);
	return result;
}

#pragma endregion Getters

#pragma region Initialization
/**
* @brief Initializes the managers of the game engine.
*/
void GameEngine::initManagers() {
	this->entityManager = new EntityManager();
	this->inputManager = new InputManager();
	this->uiManager = new UIManager(window->getRenderer(), inputManager);
}

/**
* @brief Initializes the component manager of the game engine.
*/
void GameEngine::initComponentManagers() {
	this->spriteManager = new ComponentManager<Sprite>();
	this->posManager = new ComponentManager<Position>();
	this->animatorManager = new ComponentManager<Animator>();
	this->audioManager = new ComponentManager<Audio>();
	this->colliderManager = new ComponentManager<Collider>();
	this->healthManager = new ComponentManager<Health>();
	this->projectileMovementManager = new ComponentManager<ProjectileMovement>();
	this->enemyMovementManager = new ComponentManager<EnemyMovement>();
}

/**
* @brief Initializes the unique components of the game engine.
*/
void GameEngine::initUniqueComponents() {
	this->playerMovement = new Movement();
	this->cameraFollow = new CameraFollow();
}

/**
* @brief Initializes the object pools.
*/
void GameEngine::initObjectPools() {
	if (this->debugDisableObjectPoolInit) return;
	// init projectile pool
	this->projectilePool = new ObjectPool(this->entityManager, this->posManager);
	this->projectilePool->init("projectile");
}

/**
* @brief Initializes the game systems.
* @param cameraWidth - Width of camera.
* @param cameraHeight - Height of camera.
* @param debug- Display position, collider and paths for debugging.
*/
void GameEngine::initSystems(int cameraWidth, int cameraHeight, bool debug) {
	this->renderSystem = new RenderSystem(this->frameDelay, spriteManager, posManager, this->window->getRenderer(), animatorManager, uiManager, colliderManager, enemyMovementManager);
	this->renderSystem->initCamera(this->window->getWindowWidth(), this->window->getWindowHeight(), cameraWidth, cameraHeight);
	this->renderSystem->debugging(debug);

	this->physicSystem = new PhysicSystem(inputManager, playerMovement, posManager, spriteManager, animatorManager, colliderManager, projectileMovementManager, enemyMovementManager);
	this->physicSystem->setCameraZoom(this->renderSystem->getCameraZoomFactorX(), this->renderSystem->getCameraZoomFactorY());
	this->physicSystem->initGrid(renderSystem->getTilemapNumberOfRows(), renderSystem->getTilemapNumberOfCols(), {renderSystem->getTileWidth(), renderSystem->getTileHeight()}, renderSystem->getTilesPerRow());

	this->audioSystem = new AudioSystem(audioManager);
	this->audioSystem->init();
}
#pragma endregion Initialization
#pragma region Scene
/**
* @brief Loads scene.
* @param scene - Scene to load.
* @param loopBGM - Whether bgm should be looped.
*/
void GameEngine::loadScene(Scene* scene, bool loopBGM) {
	this->setTilemap(scene->getTilesetFilePath(), scene->getTilemapDataFilePath(), scene->getLayerCount());
	this->setBGM(scene->getBGMFilePath(), loopBGM);
	this->physicSystem->initGrid(renderSystem->getTilemapNumberOfRows(), renderSystem->getTilemapNumberOfCols(), { renderSystem->getTileWidth(), renderSystem->getTileHeight() }, renderSystem->getTilesPerRow());
	scene->init();
	
}

/**
* @brief Changes current scene.
* @param scene - Scene to change to.
* @param collectEverything - Whether every Entity should be collected.
* @param loopBGM - Whether bgm should be looped.
*/
void GameEngine::changeScene(Scene* scene, bool clearEveryEntity, bool loopBGM) {
	// clean components and entites
	this->collectSceneGarbage(clearEveryEntity);
	this->resetLastCollisions();

	// load new scene
	this->loadScene(scene, loopBGM);
}
#pragma endregion Scene

#pragma region Garbage Collection
/**
* @brief Collects and frees the entities and component that should not be preserved when switching scenes.
* @param collectEverything - Whether every Entity should be collected.
*/
void GameEngine::collectSceneGarbage(bool collectEverything) {
	std::vector<Entity> tempVector = {};
	size_t vectorIndex = 0;

	for (std::unordered_set<Entity>::iterator i = this->entityManager->getEntityBegin(); i != this->entityManager->getEntityEnd(); i++)
	{
		Entity e = *i;
		if (collectEverything || !e.preserve) {
			this->removeEntityComponents(e);

			tempVector.insert(tempVector.begin() + vectorIndex, e);
			vectorIndex++;
		}
	}

	// clean not preserved entities.
	for (std::vector<Entity>::iterator i = tempVector.begin(); i != tempVector.end(); i++) {
		this->entityManager->destroyEntity(*i);
	}

	if (collectEverything) {
		initObjectPools();
	}
}

/**
 * @brief Resets last collisions of colliders when swapping scenes.
*/
void GameEngine::resetLastCollisions() {
	size_t componentCount = this->colliderManager->getComponentCount();

	for (size_t i = 0; i < componentCount; i++)
	{
		this->colliderManager->getComponentWithIndex(i)->resetLastCollision();
	}
}

/**
* @brief Removes components of entity.
*/
void GameEngine::removeEntityComponents(Entity e) {
	//  Clean components of entity.
	if (this->posManager->hasComponent(e)) {
		this->posManager->removeComponent(e);
	}

	if (this->audioManager->hasComponent(e)) {
		this->audioManager->removeComponent(e);
	}

	if (this->colliderManager->hasComponent(e)) {
		this->colliderManager->removeComponent(e);
	}

	if (this->spriteManager->hasComponent(e)) {
		this->spriteManager->removeComponent(e);
	}

	if (this->animatorManager->hasComponent(e)) {
		this->animatorManager->removeComponent(e);
	}

	if (this->healthManager->hasComponent(e)) {
		this->healthManager->removeComponent(e);
	}

	if (this->projectileMovementManager->hasComponent(e)) {
		this->projectileMovementManager->removeComponent(e);
	}

	if (this->enemyMovementManager->hasComponent(e)) {
		this->enemyMovementManager->removeComponent(e);
	}

	if (this->cameraFollow != nullptr && this->cameraFollow->getEntity().uid == e.uid) {
		this->cameraFollow->setEntity({ 0, "", false });
	}

	if (this->playerMovement!= nullptr && this->playerMovement->getEntity().uid == e.uid) {
		this->playerMovement->setEntity({ 0, "", false });
	}
}

/**
* @brief Collects objects of the object pools.
*/
void GameEngine::collectObjects() {
	this->collectProjectileObjects();
}

/**
* @brief Collects objects of the projectile object pool.
*/
void GameEngine::collectProjectileObjects() {
	int poolSize = this->projectilePool->getPoolSize();

	for (size_t i = 0; i < poolSize; i++)
	{
		// check if entity is currently used
		if (this->projectilePool->isEntityUsed(i)) {
			Position* position = this->posManager->getComponent(this->projectilePool->getEntityByIndex(i));
			if (position->x() < 0
				|| position->x() > this->renderSystem->getTotalTilemapWidth()*renderSystem->getCameraZoomFactorX()
				|| position->y() < 0 
				|| position->y() > this->renderSystem->getTotalTilemapHeight() * renderSystem->getCameraZoomFactorY()) {
				this->destroyProjectile(this->projectilePool->getEntityByIndex(i));
			}
		}
	}
}
#pragma endregion Garbage Collection