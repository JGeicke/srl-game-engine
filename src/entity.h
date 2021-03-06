#include <string>
#pragma once
/**
 * @brief Base struct to represent Entities in the game.
*/
struct Entity {
	/**
	 * @brief Unique identifier for each entity.
	*/
	unsigned int uid;

	/**
	 * @brief Tag of the entity.
	*/
	std::string tag = "";

	/**
	 * @brief Whether the game object should be preserved between scenes.
	*/
	bool preserve = false;

	/**
	 * @brief Overloading == operator to be able to check if entities are equal.
	 * @param other - Other entity to check if equal
	 * @return Whether both entities are equal.
	*/
	bool operator== (const Entity& other) const {
		return uid == other.uid;
	}

	/**
	 * @brief Overloading < operator to be able to check if one entity are lesser than the other.
	 * @param other - Other entity to check with
	 * @return Whether one entity is lesser than the other
	*/
	bool operator< (const Entity& other) const {
		return uid < other.uid;
	}

	/**
	 * @brief Overloading != operator to be able to check if entities are not equal.
	 * @param other - Other entity to check if not equal
	 * @return Whether both entities are not equal.
	*/
	bool operator != (const Entity& other) const {
		return uid != other.uid;
	}
};
