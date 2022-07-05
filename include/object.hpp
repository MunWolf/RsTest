#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <vector>

#include "lightsource.hpp"

class Scene;

class Object{
protected:
    glm::vec3 m_position;
    glm::vec3 m_size;
    Scene* m_scene;
    glm::mat4 m_model;
    glm::mat4 m_rotation;
public:
    Object();
    Object(Scene* scene, glm::vec3 pos, glm::vec3 size);
    virtual void update();
    virtual void draw();
    void rotate(float deg, glm::vec3 dir);
    glm::vec3 getPosition();
    void setPosition(glm::vec3 pos);
    float getSize();
    void setSize(float size);
};