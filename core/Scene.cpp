#include "Scene.h"
#include <algorithm>

Scene::Scene() {}

void Scene::addPrimitive(std::unique_ptr<Object> primitive)
{
    m_primitives.push_back(std::move(primitive));
}

/**
 * @brief Удаляет примитив из сцены по его указателю.
 * Использует идиому erase-remove для поиска и удаления элемента.
 */
void Scene::removePrimitive(Object* primitiveToRemove)
{
    m_primitives.erase(
        std::remove_if(m_primitives.begin(), m_primitives.end(),
                       [primitiveToRemove](const std::unique_ptr<Object>& p) {
                           // Сравниваем сырые указатели, чтобы найти нужный unique_ptr
                           return p.get() == primitiveToRemove;
                       }),
        m_primitives.end());
}

const std::vector<std::unique_ptr<Object>>& Scene::getPrimitives() const
{
    return m_primitives;
}
