#include "scene.hpp"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

#include "object.hpp"
#include "shape.hpp"
#include "utils.hpp"
#include "app.hpp"

Scene::Scene(const char* name) : m_currentCamera(new Camera(this, glm::vec3(-10, 0, -1))),
                                 m_rsScene      (new RsScene())
{
    const GLchar* vsSource[] = {R"src(#version 330 core
    in vec4 a_Position;
    in vec4 a_Normal;
    in vec2 a_TexCoord;

    out vec4 fragPos;
    out vec4 normal;
    out vec2 texCoord;
    out vec3 cubeMapTexCoord;

    uniform mat4 u_Model;
    uniform mat4 u_View;
    uniform mat4 u_Proj;

    void main() {
        fragPos = u_Model * a_Position;
        normal = u_Model * a_Normal;
        texCoord = normalize(a_Position.xy);
        cubeMapTexCoord = vec3(a_Position);
        gl_Position = u_Proj * u_View * fragPos;
    }
    )src" };

    const GLchar* fsSource[] = {R"src(#version 330 core
    in vec4 fragPos;
    in vec4 normal;
    in vec2 texCoord;
    in vec3 cubeMapTexCoord;

    uniform vec3 u_LightPos;
    uniform vec3 u_LightColour;
    uniform float u_LightBrightness;
    uniform int u_ObjectType;
    uniform vec3 u_ObjectColour;
    uniform float u_AmbientStrength;
    uniform bool u_IsTextured;
    uniform sampler2D u_Texture;
    uniform samplerCube u_CubeMap;

    void main(){
	    vec3 ambient = u_AmbientStrength * u_LightColour;
	    vec4 norm = normalize(normal);
	    vec4 texColour;
	    if (u_IsTextured)
            if(u_ObjectType == 0)
                texColour = texture(u_CubeMap, cubeMapTexCoord);
            else
		        texColour = texture(u_Texture, texCoord);
	    else
		    texColour = vec4(1, 1, 1, 1);
	    vec4 lightDir = normalize(vec4(u_LightPos, 1.f) - fragPos);
	    float diffuseStrength = max(dot(norm, lightDir), 0.0f) * u_LightBrightness;	
	    vec3 diffuse = diffuseStrength * u_LightColour;
	    vec3 result = (ambient + diffuse) * u_ObjectColour;
	    gl_FragColor = vec4(result, 1.0f) * texColour;
    }
    )src" };

    m_shader = utils::createShader(vsSource, fsSource);

	glUseProgram(m_shader);
    utils::checkGLError(" creating shader program");

    m_rsScene->name = name;

    App::getSchema().addScene(*m_rsScene);
    App::reloadSchema();

    updateMatrices();
}

Scene::~Scene(){
    glDeleteShader(m_shader);
}

void Scene::updateMatrices() {
    App* app = App::getInstance();
    const float width = app->getWindowWidth();
    const float height = app->getWindowHeight();
    const glm::vec3 camPos(m_currentCamera->getPosition());
    const glm::vec3 camFront(m_currentCamera->getFront());
    const glm::vec3 camUp(m_currentCamera->getUp());
    const glm::mat4 m_projection = glm::perspective(glm::radians(m_currentCamera->getFov()), width / height, 0.1f, 9000.0f);
    const glm::mat4 m_view = glm::lookAt(camPos, camPos + camFront, camUp);

    const unsigned int viewLoc = glGetUniformLocation(m_shader, "u_View");
    const unsigned int projLoc = glGetUniformLocation(m_shader, "u_Proj");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &m_view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &m_projection[0][0]);
    const unsigned int mvpLoc = glGetUniformLocation(m_shader, "u_Mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &(m_projection * m_view)[0][0]);
}

void Scene::render(){
    if (!m_lightSources.size()) {
        std::cout << "no light source!!\n";
        return;
    }
    const unsigned int lightPosLoc = glGetUniformLocation(m_shader, "u_LightPos");
    const unsigned int lightColourLoc = glGetUniformLocation(m_shader, "u_LightColour");
    const unsigned int brightnessLoc = glGetUniformLocation(m_shader, "u_LightBrightness");
    const unsigned int ambientLoc = glGetUniformLocation(m_shader, "u_AmbientStrength");

    glUniform3fv(lightPosLoc, 1, &m_lightSources[0].getPosition()[0]);
    glUniform3fv(lightColourLoc, 1, &m_lightSources[0].getColour()[0]);
    glUniform1f(brightnessLoc, m_lightSources[0].getBrightness());
    glUniform1f(ambientLoc, m_lightSources[0].getAmbientStrength());

    const std::vector<float>& params = App::getParams();
    const std::vector<ImageFrameData>& imgData = App::getImgData();
    for (int i = 0; i < m_objects.size(); ++i)
    {
        Object* obj = m_objects[i];

        int ind = !i ? 0 : i * 6;

        // set object position and rotation to values returned by frame parameters
        obj->setPosition(glm::vec3(params[ind + 2], -params[ind + 1], params[ind]));
        obj->setRotation(-params[ind + 5], params[ind + 3], -params[ind + 4]);

        obj->update(imgData[i]);

        obj->draw();
    }
}

Object* Scene::addObject(ObjectType type, ObjectArgs args){
    Object* obj;
    switch(type){
    case Object_Cube:
        obj = new Cube(this, args.pos, args.size, args.colour);
        break;
    case Object_Sphere:
        obj = new Sphere(this, args.pos, args.size, args.stackCount, args.sectorCount, args.colour);
        break;
    }
    m_objects.push_back(obj);

    if (args.name == "")
    {
        args.name = objectTypes[type];
        args.name += " ";
        args.name += std::to_string(getObjectCount(type));
    }

    // add remote parameters for object
    std::vector<RemoteParameter> params;
    params.push_back(RsFloatParam(args.name + "pos_x", "posX", args.name, args.pos.x, -1000, 1000, 0.1));
    params.push_back(RsFloatParam(args.name + "pos_y", "posY", args.name, args.pos.y, -1000, 1000, 0.1));
    params.push_back(RsFloatParam(args.name + "pos_z", "posZ", args.name, args.pos.z, -1000, 1000, 0.1));
    params.push_back(RsFloatParam(args.name + "rot_x", "rotX", args.name, 0, 0, 359, 1));
    params.push_back(RsFloatParam(args.name + "rot_y", "rotY", args.name, 0, 0, 359, 1));
    params.push_back(RsFloatParam(args.name + "rot_z", "rotZ", args.name, 0, 0, 359, 1));

    params.push_back(RsTextureParam(args.name + "texture", "texture", args.name));

    for (const RemoteParameter& param : params)
        m_rsScene->addParam(param);

    App::getSchema().reloadScene(*m_rsScene);
    App::reloadSchema();

    return obj;
}

unsigned int Scene::getShader(){
    return m_shader;
}

LightSource* Scene::addLightSource(glm::vec3 position, float brightness, float ambientStrength, glm::vec3 colour){
    Cube* lightCube = new Cube(this, position, .02f);
    LightSource light(position, brightness, ambientStrength, colour, lightCube);
    m_lightSources.push_back(light);
    return &m_lightSources[m_lightSources.size() - 1];
}

Camera* Scene::addCamera(glm::vec3 pos, float fov) {
    Camera balls(this);
    return &balls;
}

Camera* Scene::getCurrentCamera() {
    return m_currentCamera;
}

int Scene::getObjectCount()
{
    return m_objects.size();
}

int Scene::getObjectCount(ObjectType type)
{
    int count = 0;
    for (Object* o : m_objects)
        if (o->getType() == type)
            count++;
    return count;
}
