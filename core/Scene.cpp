#include "Scene.h"

#include <algorithm>

// Конструктор класса Scene.
Scene::Scene() : m_nextId(1) // Инициализируем счетчик ID (начинаем с 1)
{
}

// Добавляет примитив на сцену.
void Scene::addPrimitive(std::unique_ptr<Object> primitive)
{
    // Присваиваем объекту ID и увеличиваем счетчик
    primitive->setID(m_nextId++);
    m_primitives.push_back(std::move(primitive));
}

// Удаляет примитив из сцены по его указателю.
void Scene::removePrimitive(Object* primitiveToRemove)
{
    m_primitives.erase(
        std::remove_if(m_primitives.begin(), m_primitives.end(),
            [primitiveToRemove](const std::unique_ptr<Object>& p) {
               // Сравниваем сырые указатели, чтобы найти нужный unique_ptr.
               return p.get() == primitiveToRemove;
            }),
        m_primitives.end());

    // Примечание: m_nextId не сбрасывается, чтобы гарантировать уникальность ID.
}

// Возвращает константную ссылку на вектор всех примитивов.
const std::vector<std::unique_ptr<Object>>& Scene::getPrimitives() const
{
    return m_primitives;
}
