#include "rendersystem.h"
/**
* @brief Constructor for RenderSystem objects to set needed references.
* @param spriteManager - Reference to the sprite manager for sprite objects.
* @param positionManager  - Reference to the position manager for position objects.
* @param renderer - Reference to the SDL_Renderer for the actual rendering of the sprites in the gameworld.
* @param cameraFollowManager - Camera follow component manager for rendering of 2d camera. 
* @param animatorManager - Animator manager to animate the sprites with an animator component.
 *@param uiManager - UIManager to be able to render the ui elements.
*/
RenderSystem::RenderSystem(ComponentManager<Sprite>* spriteManager, ComponentManager<Position>* positionManager, SDL_Renderer* renderer,
	ComponentManager<CameraFollow>* cameraFollowManager, ComponentManager<Animator>* animatorManager, UIManager* uiManager, ComponentManager<Collider>* colliderManager) {
	this->positionManager = positionManager;
	this->spriteManager = spriteManager;
	this->renderer = renderer;
	this->cameraFollowManager = cameraFollowManager;
	this->animatorManager = animatorManager;
	this->uiManager = uiManager;
	this->colliderManager = colliderManager;

	debug = true;
}

/**
* @brief Render system update loop. Iterates over every sprite object and renders the texture to the position of the entity holding the sprite.	
*/
void RenderSystem::update() {
	// clear renderer
	SDL_RenderClear(renderer);
	moveCamera();

	controlAnimations();

	renderTilemap();
	renderSprites();

	debugPosition();
	debugColliders();

	renderUI();

	render();
}
/**
 * @brief Render the current textures of the renderer.
*/
void RenderSystem::render() {
	SDL_RenderPresent(renderer);
}

/**
* @brief Debug function that visualizes the position.
*/
void RenderSystem::debugPosition() {
	if (debug) {
		size_t componentCount = positionManager->getComponentCount();

		for (size_t i = 0; i < componentCount; i++)
		{
			Position* nextPosition = positionManager->getComponentWithIndex(i);
			SDL_RenderDrawPoint(renderer, nextPosition->x() - camera.x, nextPosition->y() - camera.y);
		}
	}
}

void RenderSystem::debugColliders() {
	if (debug) {
		size_t componentCount = colliderManager->getComponentCount();

		for (size_t i = 0; i < componentCount; i++)
		{
			Collider* nextCollider = colliderManager->getComponentWithIndex(i);
			SDL_Rect* colliderRect = nextCollider->getColliderRect();
			SDL_Rect renderRect = { colliderRect->x-camera.x, colliderRect->y-camera.y,colliderRect->w, colliderRect->h };
			SDL_RenderDrawRect(renderer, &renderRect);
		}
	}
}

#pragma region UI
/**
* @brief Renders all the ui elements in the window.
*/
void RenderSystem::renderUI(){
	renderPanels();
	renderButtons();
	renderProgressBars();
	renderLabels();
}

/**
* @brief Renders all the labels of the ui.
*/
void RenderSystem::renderLabels() {
	size_t labelCount = uiManager->getCurrentLabelIndex();
	for (size_t i = 0; i < labelCount; i++) {
		Label* nextLabel = uiManager->getLabel(i);
		if (nextLabel->isVisible()) {
			SDL_RenderCopy(renderer, nextLabel->getLabelTexture(), NULL, nextLabel->getDisplayPosition());
		}
	}
}

/**
* @brief Renders all the panels of the ui.
*/
void RenderSystem::renderPanels() {
	size_t panelCount = uiManager->getCurrentPanelIndex();
	for (size_t i = 0; i < panelCount; i++) {
		Panel* nextPanel = uiManager->getPanel(i);
		if (nextPanel->isVisible()) {
			SDL_RenderCopy(renderer, nextPanel->getPanelTexture(), NULL, nextPanel->getDisplayPosition());
		}
	}
}

/**
* @brief Renders all the progress bars of the ui.
*/
void RenderSystem::renderProgressBars(){
	size_t progressBarCount = uiManager->getCurrentProgressBarIndex();
	for (size_t i = 0; i < progressBarCount; i++) {
		ProgressBar* nextProgressBar = uiManager->getProgressBar(i);
		if (nextProgressBar->isVisible()) {
			SDL_RenderCopy(renderer, nextProgressBar->getBackgroundTexture(), NULL, nextProgressBar->getBackgroundPosition());
			SDL_RenderCopy(renderer, nextProgressBar->getProgressTexture(), NULL, nextProgressBar->getProgressPosition());
		}
	}
}

/**
* @brief Renders all the buttons of the ui.
*/
void RenderSystem::renderButtons() {
	size_t buttonCount = uiManager->getCurrentButtonIndex();
	for (size_t i = 0; i < buttonCount; i++) {
		Button* nextButton = uiManager->getButton(i);
		if (nextButton->isVisible()) {
			SDL_RenderCopy(renderer, nextButton->getButtonPanel(), NULL, nextButton->getButtonPanelPosition());
			SDL_RenderCopy(renderer, nextButton->getButtonText(), NULL, nextButton->getButtonTextPosition());
		}
	}
}
#pragma endregion UI

#pragma region Sprites
/**
* @brief Renders all current sprites in the window.
*/
void RenderSystem::renderSprites() {
	// sort sprites to display depth in 2d environment
	sortSprites();

	unsigned int index = spriteManager->getComponentCount();
	for (size_t i = 0; i < index; i++)
	{
		Position* spritePosition = &sortedSpritePositions[i];
		Entity spriteEntity = spritePosition->getEntity();
		Sprite* sprite = spriteManager->getComponent(spriteEntity);
		sprite->setDestinationRectPosition((spritePosition->x() - (sprite->getDestinationWidth() / 2)) - camera.x, (spritePosition->y() - (sprite->getDestinationHeight() / 2)) - camera.y);
		draw(sprite);
	}
}

/**
* @brief Sorts the sprite before rendering. Needed to be able to display depth in 2d environment.
*/
void RenderSystem::sortSprites() {
	// copy current array
	size_t componentCount = positionManager->getComponentCount();
	for (size_t i = 0; i < componentCount; i++) {
		sortedSpritePositions[i] = *(positionManager->getComponentWithIndex(i));
	}

	// sort array
	mergeSort(&sortedSpritePositions[0], 0, componentCount - 1);
}

/**
 * @brief Sorts and merges the two parts of the position array together.
 * @param arr - array to sort and merge the two parts of.
 * @param start - Startindex of left side.
 * @param middle - Middleindex between left and right side.
 * @param end - Endindex of right side.
*/
void RenderSystem::merge(Position* arr, size_t start, size_t middle, size_t end) {
	size_t i, j, k;
	i = start;
	k = start;
	j = middle + 1;

	Position temp[1024];

	while (i <= middle && j <= end) {
		Sprite* leftSprite = spriteManager->getComponent(arr[i].getEntity());
		Sprite* rightSprite = spriteManager->getComponent(arr[j].getEntity());

		int leftY = arr[i].y() - (leftSprite->getDestinationHeight() / 2);
		int rightY = arr[j].y() - (rightSprite->getDestinationHeight() / 2);

		if (leftY < rightY) {
			temp[k] = arr[i];
			i++;
			k++;
		}
		else {
			temp[k] = arr[j];
			j++;
			k++;
		}
	}

	while (i <= middle) {
		temp[k] = arr[i];
		i++;
		k++;
	}

	while (j <= end) {
		temp[k] = arr[j];
		j++;
		k++;
	}

	for (i = start; i < k; i++) {
		arr[i] = temp[i];
	}
}

/**
 * @brief Merge sort implementation to sort an positions array.
 * @param arr - Positions array to sort.
 * @param start - Startindex of array.
 * @param end - Endindex of array.
*/
void RenderSystem::mergeSort(Position* arr, size_t start, size_t end) {
	size_t middle;
	if (start < end) {
		// divide array and conquer
		middle = (start + end) / 2;
		mergeSort(arr, start, middle);
		mergeSort(arr, middle + 1, end);
		// merge arrays
		merge(arr, start, middle, end);
	}
}

/**
 * @brief Checks if the sprite has a texture and creates a texture if it has not. Afterwards adds the sprite to the renderer to render.
 * @param sprite
*/
void RenderSystem::draw(Sprite* sprite) {
	// if sprite has no texture
	if (!sprite->hasTexture()) {

		// load texture
		Texture spriteTexture = FileLoader::loadTexture(sprite->texturePath, renderer);

		// set texture of sprite aswell as width and height of texture
		sprite->setTexture(spriteTexture);
	}

	Animator* animator = animatorManager->getComponent(sprite->getEntity());

	if (animator && animator->hasAnimation()) {
		animateSprite(sprite, animator);
	}
	SDL_RenderCopyEx(renderer, sprite->getTexture().texture, sprite->getSourceRect(), sprite->getDestinationRect(), NULL, NULL, sprite->getTextureFlip());
}
#pragma endregion Sprites

#pragma region Animations
/**
 * @brief Control the animations depending on the animation state and the animation direction.
*/
void RenderSystem::controlAnimations() {
	size_t index = animatorManager->getComponentCount();
	for (size_t i = 0; i < index; i++) {
		Animator* currentAnimator = animatorManager->getComponentWithIndex(i);
		size_t state = currentAnimator->getState();

		switch (state) {
		case STATES::IDLE:
			//IDLE
			switch (currentAnimator->getDirection()) {
			case DIRECTION::SIDE:
				currentAnimator->play("idle_side");
				break;
			case DIRECTION::DOWN:
				currentAnimator->play("idle_down");
				break;
			case DIRECTION::UP:
				currentAnimator->play("idle_up");
				break;
			case DIRECTION::NO_DIRECTION:
				// play current animation
				currentAnimator->play(currentAnimator->getCurrentAnimationName());
				break;
			default:
				break;
			}
			break;
		case STATES::WALK:
			//WALK
			switch (currentAnimator->getDirection()) {
			case DIRECTION::SIDE:
				currentAnimator->play("walk_side");
				break;
			case DIRECTION::DOWN:
				currentAnimator->play("walk_down");
				break;
			case DIRECTION::UP:
				currentAnimator->play("walk_up");
				break;
			default:
				break;
			}
			break;
		default:
			std::cout << "Animation not found" << std::endl;
			break;
		}
	}
}

/**
* @brief Animates the sprite.
* @param sprite - Sprite to animate.
* @param animator - Animator with animation data.
*/
void RenderSystem::animateSprite(Sprite* sprite, Animator* animator) {
	int newX, newY;

	Animation* currentAnimation = animator->getCurrentAnimation();

	if (currentAnimation->hasAnimationTexture()) {
		Texture newTexture = currentAnimation->getAnimationTexture();
		sprite->setTexture(newTexture);
	}

	int currentFrame = static_cast<int>((SDL_GetTicks() / currentAnimation->frameDelayMS) % currentAnimation->frames);
	int lastTileInRow = sprite->getTexture().textureWidth - sprite->getSourceWidth();
	int currentTile = currentFrame * sprite->getSourceWidth();

	// Reset y offset when returning first frame of animation
	if (currentFrame == 0) {
		currentAnimation->resetYOffset();
	}

	// increase y offset if next row
	if (currentTile - lastTileInRow == sprite->getSourceWidth()) {
		currentAnimation->incrementYOffset(currentFrame);
	}

	// calc new x position on new row
	if ((currentTile) > (lastTileInRow)) {
		newX = (currentTile - lastTileInRow) - sprite->getSourceWidth();
	}
	else {
		newX = sprite->getSourceWidth() * currentFrame;
	}
	// calc new y position depending on offset
	newY = currentAnimation->getYOffset() * sprite->getSourceHeight();

	// set new area of sprite to display
	sprite->setSourceRectPosition(newX, newY);
}
#pragma endregion Animations

#pragma region Tilemap
/**
* @brief Sets the current tilemap to display.
* @param tilesetPath - Path to tileset image used by tilemap.
* @param tilemapPath - Path to tilemap json file generated by tiled.
* @param layerCount - Count of layers in tilemap.
*/
void RenderSystem::setMap(const char* tilesetPath, const char* tilemapPath, size_t layerCount) {
	SDL_Point size = { 0,0 };
	tilemap = FileLoader::loadTilemap(tilemapPath, layerCount);

	Texture tilesetTexture = FileLoader::loadTexture(tilesetPath, renderer);

	tileset = new Tileset(tilesetTexture);

	// set srcRect
	tileset->initSourceRect(tilemap->getTileWidth(), tilemap->getTileHeight());

	// set destRect
	tileset->initDestinationRect(tilemap->getTileWidth(), tilemap->getTileHeight());
}

/**
* @brief Renders the tilemap.
*/
void RenderSystem::renderTilemap() {
	unsigned int tileWidth, tileHeight, currentDestX, currentDestY, maxTilesPerRow;
	currentDestX = 0;
	currentDestY = 0;
	maxTilesPerRow = tilemap->getTilesPerRow();
	tileWidth = tilemap->getTileWidth();
	tileHeight = tilemap->getTileHeight();
	size_t layers = tilemap->getLayerCount();

	for (size_t i = 0; i < layers; i++)
	{
		currentDestX = 0;
		currentDestY = 0;
		std::vector<unsigned int> layer = tilemap->getLayer(i);
		for (size_t i = 0; i < layer.size(); i++)
		{
			// checks if current x-position is greater or equals to max tilecount in row
			if (currentDestX >= maxTilesPerRow) {
				currentDestX = 0;
				currentDestY = currentDestY + 1;
			}
			// check if spot is empty
			if (layer[i] != 0) {
				// calculate part to render
				setTilesetSrcRectPosition(layer[i], tileWidth, tileHeight);

				// calculate part to render at ingame
				setTilesetDestRectPosition(currentDestX, currentDestY, tileWidth, tileHeight);

				SDL_RenderCopy(renderer, tileset->getTexture().texture, tileset->getSourceRect(), tileset->getDestinationRect());
			}
			currentDestX = currentDestX + 1;
		}
	}
}

/**
* @brief Sets the source rectangle of the tileset to display the right part of the tileset. It is important that the size of the tileset is a multiple of the tilesize.
* @param tilemapData - What tile to display based on the tilemap.
* @param tileWidth - Width of the tiles.
* @param tileHeight - Height of the tiles.
*/
void RenderSystem::setTilesetSrcRectPosition(unsigned int tilemapData, unsigned int tileWidth, unsigned int tileHeight) {
	unsigned int tilesetWidth = tileset->getTexture().textureWidth;

	int newX;
	int newY;
	// check if first row tileWidth * maxWidth
	if ((tilemapData * tileWidth) <= (tilesetWidth)) {
		newX = (tilemapData == 0) ? tilemapData * tileWidth : (tilemapData - 1) * tileWidth;
		newY = 0;
	}
	else {
		// calculate row
		unsigned int yoffset = (tilemapData * tileWidth) / (tilesetWidth);
		unsigned int xoffset = ((tilemapData * tileWidth) % (tilesetWidth)) / tileWidth;

		newX = (xoffset == 0) ? xoffset * tileWidth : (xoffset - 1) * tileWidth;
		newY = yoffset * tileHeight;
	}
	// set source rect position
	tileset->setSourceRect(newX, newY);
}

/**
* @brief Sets the destination rectangle of the tileset to display the tiles in the right area.
* @param currentX - Current X position on the tilemap.
* @param currentY - Current Y position on the tilemap.
* @param tileWidth - Width of the tiles.
* @param tileHeight - Height of the tiles.
*/
void RenderSystem::setTilesetDestRectPosition(unsigned int currentX, unsigned int currentY, unsigned int tileWidth, unsigned int tileHeight) {
	int newX;
	int newY;

	//newX = (currentX == 0) ? currentX * tileWidth : (currentX - 1) * tileWidth;
	//newY = (currentY == 0) ? currentY * tileHeight : (currentY - 1) * tileHeight;
	newX = currentX * tileWidth;
	newY = currentY * tileHeight;

	// substracts the calculated tile position based on tilewidth & the position on the tilemap from the camera origin position (top-left).
	// if the result is positiv, render the tile on the substracted position (relative to the camera origin).
	// if the result is negativ, don't render the tile.
	newX = newX - camera.x;
	newY = newY - camera.y;

	//std::cout << "Tile dest pos: (" << newX << ", " << newY << ");" << std::endl;
	tileset->setDestinationRect(newX, newY);
}
#pragma endregion Tilemap

#pragma region Camera
/**
* @brief Initializes the camera with certain viewport.
* @param viewWidth - Width of viewport.
* @param viewHeight - Height of viewport.
*/
void RenderSystem::initCamera(int viewWidth, int viewHeight) {
	camera = { 0, 0, viewWidth, viewHeight };
}

/**
 * @brief Moves the camera to the entity with the follow camera component.
*/
void RenderSystem::moveCamera() {
	// TODO: test with different maps and different view sizes
	if (cameraFollowManager->getComponentCount() == 1) {
		Entity followTarget = cameraFollowManager->getComponentWithIndex(0)->getEntity();
		Position* followPosition = positionManager->getComponent(followTarget);

		camera.x = followPosition->x() - (camera.w / 2);
		camera.y = followPosition->y() - (camera.h / 2);

		// check the camera bounds
		// top-left
		if (camera.x < 0) {
			camera.x = 0;
		}
		if (camera.y < 0) {
			camera.y = 0;
		}

		//TODO: camera stops one tile to early
		//bottom-right
		if (camera.x > (tilemap->getTotalTilemapWidth() - camera.w - tilemap->getTileWidth())) {
			camera.x = tilemap->getTotalTilemapWidth() - camera.w - tilemap->getTileWidth();
		}

		if (camera.y > (tilemap->getTotalTilemapHeight() - camera.h - tilemap->getTileHeight())) {
			camera.y = tilemap->getTotalTilemapHeight() - camera.h - tilemap->getTileHeight();
		}
	}
}
#pragma endregion Camera