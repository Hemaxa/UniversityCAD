#pragma once

#include "Object.h"

#include <vector>
#include <memory>

// Центральное хранилище для всех геометрических объектов в проекте.
class Scene
{
public:
    // Конструктор класса Scene.
    Scene();

    // Добавляет новый примитив (объект) на сцену.
    void addPrimitive(std::unique_ptr<Object> primitive);

    // Удаляет указанный примитив со сцены.
    void removePrimitive(Object* primitiveToRemove);

    // Возвращает константную ссылку на вектор всех примитивов на сцене.
    const std::vector<std::unique_ptr<Object>>& getPrimitives() const;

private:
    // Вектор умных указателей на все примитивы, находящиеся на сцене.
    std::vector<std::unique_ptr<Object>> m_primitives;
};
