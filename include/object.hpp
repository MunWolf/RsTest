#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <vector>

#include "lightsource.hpp"
#include "scene.hpp"

#include "d3/d3renderstream.h"

class Scene;

class Object{
protected:
    ObjectType m_type;
    glm::vec3 m_position;
    glm::vec3 m_size;
    Scene* m_scene;
    glm::mat4 m_model;
    glm::mat4 m_rotation;
    GLuint m_texture;
    glm::vec2 m_lastTexSize;
    virtual void init();
public:
    Object();
    Object(Scene* scene, glm::vec3 pos, glm::vec3 size);
    // take in image data to update texture
    virtual void update(const ImageFrameData& imgData = ImageFrameData());
    virtual void draw();
    void rotate(float deg, glm::vec3 dir);
    glm::vec3 getPosition();
    void setPosition(glm::vec3 pos);
    float getSize();
    ObjectType getType();
    void setSize(float size);
    void setRotation(float x, float y, float z);
};