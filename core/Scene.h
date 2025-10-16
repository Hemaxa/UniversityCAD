#pragma once
#include "Object.h"
#include <vector>
#include <memory>

/**
 * @class Scene
 * @brief Центральное хранилище для всех геометрических объектов в проекте.
 */
class Scene
{
public:
    Scene();

    /**
     * @brief Добавляет новый примитив в сцену.
     * @param primitive Умный указатель на добавляемый объект.
     */
    void addPrimitive(std::unique_ptr<Object> primitive);

    /**
     * @brief Удаляет примитив из сцены по его указателю.
     * @param primitiveToRemove Указатель на объект, который нужно удалить.
     */
    void removePrimitive(Object* primitiveToRemove);

    /**
     * @brief Возвращает константную ссылку на вектор всех примитивов в сцене.
     */
    const std::vector<std::unique_ptr<Object>>& getPrimitives() const;

private:
    /**
     * @var m_primitives
     * @brief Вектор умных указателей на все примитивы, находящиеся на сцене.
     */
    std::vector<std::unique_ptr<Object>> m_primitives;
};
