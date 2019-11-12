#include "ECSManager.h"
#include "IdFactory.hpp"


Meltdown::ECS::ECSManager::ECSManager(Core::Engine* engine)
{
	this->engine = engine;

	void* entityStartPtr = engine->GetGlobalAllocator().Allocate(Settings::MAX_ENTITIES * sizeof(EntityHandle), alignof(EntityHandle));
	this->entityAllocator = new Memory::PoolAllocator(sizeof(EntityHandle), alignof(EntityHandle), Settings::MAX_ENTITIES * sizeof(EntityHandle), entityStartPtr);

	void* componentStartPtr = engine->GetGlobalAllocator().Allocate(Settings::MAX_ENTITIES * sizeof(EntityHandle), Memory::NORMAL_ALIGNMENT);
	this->componentAllocator = new Memory::ChunkListAllocator(Settings::MAX_ENTITIES * sizeof(EntityHandle), componentStartPtr);
}

Meltdown::ECS::ECSManager::~ECSManager()
{
	delete entityAllocator;
	delete componentAllocator;
}

void Meltdown::ECS::ECSManager::AddEntity()
{
	EntityHandle* newEnt = nullptr;
	//Check if dead entity can be revived
	if (entityVector.size() > aliveEntities)
		newEnt = entityVector[aliveEntities]; //Revive
	else
	{
		//Create new entity
		newEnt = Memory::AllocateNew<EntityHandle>(*entityAllocator);
		newEnt->componentMask.reset();
		newEnt->index = entityVector.size();
		entityVector.push_back(newEnt);
	}
	newEnt->dataIndex = Util::IdFactory<EntityHandle>::GetId();
	newEnt->isAlive = true;
	++aliveEntities;

	//Create new storage for components
	if(newEnt->dataIndex >= componentVector.size() / Settings::MAX_COMPONENT_TYPES - 1)
	{
		//Fill entity data space
		for (int i = 0; i < Settings::MAX_COMPONENT_TYPES; ++i)
			componentVector.push_back(nullptr);
	}
}

void Meltdown::ECS::ECSManager::RemoveEntity(EntityHandle& entity)
{
	entity.pendingDeath = true;
}

void Meltdown::ECS::ECSManager::Refresh()
{
	unsigned a = 0;
	unsigned b = aliveEntities;
	//Sort EntityVector so all alive entities are in front
	while (true)
	{
		for (; true; ++a)
		{
			if (a >= b)
				return;

			if (entityVector[a]->pendingDeath) //Entity should die
			{
				--aliveEntities;
				//Reset entity
				entityVector[a]->isAlive = false;
				entityVector[a]->pendingDeath = false;
				entityVector[a]->componentMask.reset();
				//Deallocate components
				for(unsigned i = 0; i<Settings::MAX_COMPONENT_TYPES; ++i)
				{
					if (componentVector[entityVector[a]->dataIndex * Settings::MAX_COMPONENT_TYPES + i] != nullptr)
						componentAllocator->Deallocate(componentVector[entityVector[a]->dataIndex * Settings::MAX_COMPONENT_TYPES + i]);
				}
				Util::IdFactory<EntityHandle>::FreeId(entityVector[a]->dataIndex);
				entityVector[a]->dataIndex = -1;
			}

			if (!entityVector[a]->isAlive)
				break;
		}
		for (; true; --b)
		{
			if (entityVector[b]->isAlive)
				break;
			if (b <= a)
				return;
		}
		EntityHandle* temp = entityVector[a];
		entityVector[a] = entityVector[b];
		entityVector[b] = temp;
	}
}