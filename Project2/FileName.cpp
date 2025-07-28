/*
 * 优化版智能农场监控系统 - 基于原始脚本完整优化
 * 修复：自由镜头、天气系统、植物位置、参数显示、农场功能
 * 完全基于原始代码结构，确保兼容性
 */

#define GLEW_STATIC
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "opengl32.lib")
#endif

 // 兼容性修复
template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

// 增强的顶点结构
struct Vertex {
    float x, y, z;        // 位置
    float r, g, b;        // 颜色
    float nx, ny, nz;     // 法线
    float u, v;           // 纹理坐标
    float materialType;   // 材质类型

    Vertex() : x(0), y(0), z(0), r(1), g(1), b(1), nx(0), ny(1), nz(0), u(0), v(0), materialType(0) {}

    Vertex(float px, float py, float pz, float pr, float pg, float pb,
        float pnx, float pny, float pnz, float mat = 0.0f)
        : x(px), y(py), z(pz), r(pr), g(pg), b(pb), nx(pnx), ny(pny), nz(pnz), u(0), v(0), materialType(mat) {
    }
};

struct RenderObject {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;
    bool transparent;
    glm::vec3 center;
    bool isValid;
    bool castShadow;

    RenderObject() : VAO(0), VBO(0), EBO(0), transparent(false), isValid(false), castShadow(true) {}

    ~RenderObject() { cleanup(); }

    void cleanup() {
        if (isValid) {
            if (VAO != 0) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
            if (VBO != 0) { glDeleteBuffers(1, &VBO); VBO = 0; }
            if (EBO != 0) { glDeleteBuffers(1, &EBO); EBO = 0; }
            isValid = false;
        }
    }
};

// 天气系统结构
struct WeatherSystem {
    float temperature;
    float humidity;
    float windSpeed;
    float cloudCoverage;
    float precipitation;
    int weatherType;        // 0=晴天，1=多云，2=雨天，3=暴风雨
    glm::vec3 fogColor;
    float fogDensity;

    WeatherSystem() : temperature(22.0f), humidity(65.0f), windSpeed(5.0f),
        cloudCoverage(0.3f), precipitation(0.0f), weatherType(0),
        fogColor(0.8f, 0.8f, 0.9f), fogDensity(0.02f) {
    }
};

// 传感器数据结构
struct SensorData {
    glm::vec3 position;
    float temperature;
    float humidity;
    float soilMoisture;
    float lightLevel;
    float pH;
    float nitrogenLevel;     // 新增：氮含量
    float phosphorusLevel;   // 新增：磷含量
    float potassiumLevel;    // 新增：钾含量
    glm::vec3 statusColor;
    float dataHeight[7];     // 扩展到7个数据维度
    bool isActive;

    SensorData() : position(0.0f), temperature(22.0f), humidity(60.0f),
        soilMoisture(45.0f), lightLevel(700.0f), pH(6.8f),
        nitrogenLevel(50.0f), phosphorusLevel(30.0f), potassiumLevel(40.0f),
        statusColor(0.2f, 1.0f, 0.3f), isActive(true) {
        for (int i = 0; i < 7; i++) dataHeight[i] = 0.5f;
    }
};

// 增强的植物实例结构
struct DetailedPlant {
    glm::vec3 position;
    float height;
    float stemRadius;
    int leafCount;
    float leafSize;
    float leafAngle;
    float windPhase;
    float healthFactor;
    glm::vec3 stemColor;
    glm::vec3 leafColor;
    int plantType; // 0=玉米, 1=小麦, 2=番茄, 3=菠菜
    float growthStage; // 0.0-1.0
    bool hasFlowers;
    bool hasFruits;

    // 新增细节特性
    std::vector<glm::vec3> leafPositions;    // 每片叶子的位置
    std::vector<float> leafSizes;            // 每片叶子的大小
    std::vector<glm::vec3> branchPositions;  // 分支位置
    float rootSpread;                        // 根系扩展范围
    int diseaseLevel;                        // 病害等级 0-3
    float waterNeed;                         // 水分需求
    glm::vec3 flowerColor;                   // 花朵颜色
    glm::vec3 fruitColor;                    // 果实颜色
    bool isPestInfected;                     // 是否有病虫害

    DetailedPlant() : position(0.0f), height(1.2f), stemRadius(0.03f), leafCount(8),
        leafSize(0.15f), leafAngle(45.0f), windPhase(0.0f), healthFactor(0.9f),
        stemColor(0.4f, 0.6f, 0.2f), leafColor(0.2f, 0.8f, 0.1f),
        plantType(0), growthStage(0.8f), hasFlowers(false), hasFruits(false),
        rootSpread(0.5f), diseaseLevel(0), waterNeed(0.6f),
        flowerColor(1.0f, 0.8f, 0.2f), fruitColor(0.8f, 0.2f, 0.1f), isPestInfected(false) {
    }
};

// 建筑结构定义
struct Building {
    glm::vec3 position;
    glm::vec3 size;
    std::string name;
    glm::vec3 color;
    bool hasDoor;
    bool hasWindows;
    glm::vec3 doorPos;
    std::vector<glm::vec3> windowPositions;

    Building(const std::string& n, glm::vec3 pos, glm::vec3 s, glm::vec3 col)
        : name(n), position(pos), size(s), color(col), hasDoor(true), hasWindows(true) {
    }
};

// 贝塞尔曲线路径结构
struct BezierPath {
    std::vector<glm::vec3> controlPoints;
    glm::vec3 pathColor;
    float pathWidth;
    int segments;

    BezierPath() : pathColor(0.8f, 0.6f, 0.2f), pathWidth(0.2f), segments(20) {}

    glm::vec3 calculateBezierPoint(float t) const {
        if (controlPoints.size() < 4) return glm::vec3(0.0f);

        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        glm::vec3 p = uuu * controlPoints[0];
        p += 3 * uu * t * controlPoints[1];
        p += 3 * u * tt * controlPoints[2];
        p += ttt * controlPoints[3];

        return p;
    }
};

// 增强农场状态监控 - 添加灌溉系统
struct FarmStatus {
    int healthyPlants;
    int sickPlants;
    int alertSensors;
    float avgTemperature;
    float avgHumidity;
    float avgSoilMoisture;
    bool autoIrrigation;
    bool pestControl;
    bool autoFertilizer;
    bool autoHarvest;
    bool nightLighting;
    bool climateControl;
    float waterUsage;
    float powerConsumption;
    float fertilizerLevel;
    float harvestYield;
    int irrigationIntensity; // 1-5
    int automationLevel;     // 1-3
    int plantingMode;        // 0-2 (Dense/Normal/Sparse)

    // 新增灌溉系统状态
    bool irrigationActive;
    float waterPressure;
    float waterTankLevel;
    int activeNozzles;
    float irrigationEfficiency;

    FarmStatus() : healthyPlants(0), sickPlants(0), alertSensors(0),
        avgTemperature(22.0f), avgHumidity(65.0f), avgSoilMoisture(50.0f),
        autoIrrigation(true), pestControl(false), autoFertilizer(false), autoHarvest(false),
        nightLighting(false), climateControl(true), waterUsage(0.0f), powerConsumption(0.0f),
        fertilizerLevel(75.0f), harvestYield(0.0f), irrigationIntensity(3), automationLevel(2), plantingMode(1),
        irrigationActive(false), waterPressure(85.0f), waterTankLevel(75.0f), activeNozzles(0), irrigationEfficiency(88.0f) {
    }
};

// 增强着色器源码 - 优化天气效果
const char* vertexShaderSource = R"(
#version 120
attribute vec3 aPos;
attribute vec3 aColor;
attribute vec3 aNormal;
attribute vec2 aTexCoord;
attribute float aMaterialType;

varying vec3 FragPos;
varying vec3 vertexColor;
varying vec3 Normal;
varying vec2 TexCoord;
varying float MaterialType;
varying vec4 FragPosLightSpace;
varying float FogFactor;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform float time;
uniform vec2 windDirection;
uniform float windStrength;
uniform vec3 viewPos;
uniform float fogDensity;
uniform int weatherType;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    
    // 增强的植物风动画系统
    if (aMaterialType > 0.5 && aMaterialType < 1.5) {
        float height = aPos.y;
        float bendFactor = height * windStrength * 0.4;
        
        // 根据天气类型调整风效果
        float weatherMultiplier = 1.0;
        if (weatherType == 3) weatherMultiplier = 2.5; // 暴风雨
        else if (weatherType == 2) weatherMultiplier = 1.8; // 雨天
        else if (weatherType == 1) weatherMultiplier = 1.3; // 多云
        
        vec2 windDir = normalize(windDirection);
        float heightFactor = height * height * weatherMultiplier;
        
        // 多层次风动画
        float primaryWave = sin(time * 1.2 + worldPos.x * 0.1 + worldPos.z * 0.15);
        float secondaryWave = sin(time * 3.5 + worldPos.x * 0.3) * 0.3;
        float tertiaryWave = sin(time * 8.0 + worldPos.z * 0.5) * 0.15;
        
        float combinedWave = primaryWave + secondaryWave + tertiaryWave;
        
        vec3 windOffset = vec3(
            windDir.x * combinedWave * bendFactor * heightFactor,
            -abs(combinedWave) * bendFactor * 0.15,
            windDir.y * combinedWave * bendFactor * heightFactor
        );
        
        // 雨滴效果
        if (weatherType >= 2) {
            float rainEffect = sin(time * 6.0 + worldPos.x + worldPos.z) * 0.1;
            windOffset.y += rainEffect * height;
        }
        
        worldPos.xyz += windOffset;
    }
    
    FragPos = worldPos.xyz;
    vertexColor = aColor;
    Normal = mat3(model) * aNormal;
    TexCoord = aTexCoord;
    MaterialType = aMaterialType;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // 雾效果计算
    float distance = length(viewPos - FragPos);
    FogFactor = exp(-fogDensity * distance);
    FogFactor = clamp(FogFactor, 0.0, 1.0);
    
    gl_Position = mvp * vec4(worldPos.xyz, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 120
varying vec3 FragPos;
varying vec3 vertexColor;
varying vec3 Normal;
varying vec2 TexCoord;
varying float MaterialType;
varying vec4 FragPosLightSpace;
varying float FogFactor;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 viewPos;
uniform float time;
uniform int weatherType;
uniform float cloudCoverage;
uniform float precipitation;
uniform vec3 fogColor;

vec3 calculateAdvancedWeatherLighting(vec3 albedo, vec3 normal, vec3 lightDirection, vec3 viewDirection) {
    // 基础环境光 - 根据天气调整
    vec3 ambient;
    if (weatherType == 0) { // 晴天
        ambient = vec3(0.15, 0.18, 0.2) * albedo;
    } else if (weatherType == 1) { // 多云
        ambient = vec3(0.12, 0.15, 0.18) * albedo;
    } else if (weatherType == 2) { // 雨天
        ambient = vec3(0.08, 0.10, 0.15) * albedo;
    } else { // 暴风雨
        ambient = vec3(0.05, 0.07, 0.12) * albedo;
    }
    
    // 云层影响的光照强度
    float effectiveLightIntensity = lightIntensity * (1.0 - cloudCoverage * 0.7);
    
    // 漫反射
    float diff = max(dot(normal, -lightDirection), 0.0);
    vec3 diffuse = diff * lightColor * effectiveLightIntensity * albedo;
    
    // 材质特定效果
    vec3 specular = vec3(0.0);
    
    if (MaterialType < 0.5) { // 金属材质
        vec3 reflectDir = reflect(lightDirection, normal);
        float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 128.0);
        specular = spec * lightColor * effectiveLightIntensity * 0.8;
    } 
    else if (MaterialType < 1.5) { // 植物材质
        // 次表面散射效果
        float subsurface = pow(max(dot(-lightDirection, viewDirection), 0.0), 2.0);
        vec3 subsurfaceColor = vec3(0.1, 0.4, 0.1) * subsurface * 0.3;
        ambient += subsurfaceColor;
        
        // 叶片边缘发光效果
        float rim = 1.0 - max(dot(viewDirection, normal), 0.0);
        rim = pow(rim, 2.0);
        ambient += rim * vec3(0.2, 0.6, 0.1) * 0.2;
        
        // 雨滴反射效果
        if (precipitation > 0.1) {
            vec3 reflectDir = reflect(lightDirection, normal);
            float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 32.0);
            specular += spec * lightColor * precipitation * 0.5;
        }
    }
    else if (MaterialType < 2.5) { // 玻璃材质
        vec3 reflectDir = reflect(lightDirection, normal);
        float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 256.0);
        specular = spec * lightColor * effectiveLightIntensity * 0.9;
    }
    else if (MaterialType < 3.5) { // 土壤材质
        vec3 reflectDir = reflect(lightDirection, normal);
        float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 4.0);
        specular = spec * lightColor * effectiveLightIntensity * 0.05;
    }
    else { // 传感器/电子设备
        float pulse = sin(time * 8.0 + FragPos.x + FragPos.z) * 0.3 + 0.7;
        vec3 reflectDir = reflect(lightDirection, normal);
        float spec = pow(max(dot(viewDirection, reflectDir), 0.0), 64.0);
        specular = spec * lightColor * pulse * 0.5;
    }
    
    return ambient + diffuse + specular;
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 color = calculateAdvancedWeatherLighting(vertexColor, norm, lightDir, viewDir);
    
    // 雨天额外效果
    if (weatherType >= 2) {
        // 雨滴闪烁效果
        float rainSparkle = sin(time * 12.0 + FragPos.x * 10.0 + FragPos.z * 8.0) * 0.5 + 0.5;
        rainSparkle = pow(rainSparkle, 8.0) * precipitation;
        color += vec3(0.8, 0.9, 1.0) * rainSparkle * 0.3;
    }
    
    // HDR色调映射
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    // 雾效果混合
    color = mix(fogColor, color, FogFactor);
    
    float alpha = 1.0;
    if (MaterialType > 1.5 && MaterialType < 2.5) { // 玻璃
        alpha = 0.3;
    }
    
    gl_FragColor = vec4(color, alpha);
}
)";

// 全局变量
GLuint shaderProgram = 0;
std::vector<RenderObject> renderObjects;
std::vector<SensorData> sensors;
std::vector<DetailedPlant> plants;
std::vector<Building> buildings;
std::vector<BezierPath> paths;
WeatherSystem weather;
FarmStatus farmStatus; // 新增
float systemTime = 0.0f;
float cameraAngle = 0.0f;
bool isInitialized = false;
GLFWwindow* g_window = nullptr;
bool useVAO = false;

// 摄像机控制 - 优化
glm::vec3 cameraPos = glm::vec3(15.0f, 8.0f, 15.0f);
glm::vec3 cameraFront = glm::vec3(-0.6f, -0.3f, -0.6f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -135.0f;
float pitch = -20.0f;
float lastX = 600.0f;
float lastY = 400.0f;
bool firstMouse = true;
bool freeCamera = false;

// 环境参数
glm::vec2 windDirection = glm::vec2(1.0f, 0.3f);
float windStrength = 0.4f;
float dayNightCycle = 0.0f;

// 光照参数
glm::vec3 lightPos = glm::vec3(10.0f, 15.0f, 10.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 0.95f, 0.8f);

// UI控制 - 新增
bool showUI = true;
bool showDetailedStats = false;

// 函数声明
bool initializeOpenGL();
bool createShaderProgram();
GLuint compileShader(GLenum type, const char* source);
void generateDetailedFarm();
void createDetailedBuildings();
void initializeAdvancedSensorNetwork();
void initializeDetailedPlants();
void createBezierPaths();
void updateFarmSimulation(float deltaTime);
void updateFarmStatus(); // 新增
void printUIInfo(); // 新增
void addDetailedCube(RenderObject& obj, glm::vec3 center, glm::vec3 size, glm::vec3 color,
    glm::vec3 normal = glm::vec3(0, 1, 0), float material = 0.0f);
void createDetailedPlantGeometry(RenderObject& obj, const DetailedPlant& plant);
void createBuildingGeometry(RenderObject& obj, const Building& building);
void addCylinder(RenderObject& obj, glm::vec3 bottom, glm::vec3 top, float radius,
    glm::vec3 color, int segments = 8, float material = 0.0f);
void addDetailedLeaf(RenderObject& obj, glm::vec3 position, glm::vec3 direction, float size,
    glm::vec3 color, float material = 1.0f);
void addBezierCurve(RenderObject& obj, const BezierPath& path);
bool setupBuffers(RenderObject& obj);
void render();
void updateCamera();
void updateLighting();
void checkOpenGLError(const char* operation);

// 辅助函数
void errorCallback(int error, const char* description) {
    std::cout << "GLFW Error " << error << ": " << description << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (isInitialized && width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }
}

// 优化的鼠标回调 - 修复自由镜头
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!freeCamera) return;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.05f; // 降低敏感度
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // 限制俯仰角
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// 优化的输入处理
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 镜头模式切换
    static bool cKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cKeyPressed) {
        freeCamera = !freeCamera;
        if (freeCamera) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
            std::cout << "Free Camera Mode - WASD to move, Mouse to look, Shift for speed" << std::endl;
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "Auto Cruise Mode - Left/Right arrows to control" << std::endl;
        }
        cKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        cKeyPressed = false;
    }

    // 天气控制 - 修复按键
    static bool wKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !wKeyPressed) {
        weather.weatherType = (weather.weatherType + 1) % 4;
        const char* weatherNames[] = { "Sunny", "Cloudy", "Rainy", "Stormy" };
        std::cout << "Weather changed to: " << weatherNames[weather.weatherType] << std::endl;
        wKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        wKeyPressed = false;
    }

    // UI显示控制 - 新增
    static bool iKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && !iKeyPressed) {
        showUI = !showUI;
        std::cout << (showUI ? "Farm Information: ON" : "Farm Information: OFF") << std::endl;
        iKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE) {
        iKeyPressed = false;
    }

    // 详细统计切换 - 新增
    static bool hKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hKeyPressed) {
        showDetailedStats = !showDetailedStats;
        std::cout << (showDetailedStats ? "Detailed Statistics: ON" : "Basic Information: ON") << std::endl;
        hKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE) {
        hKeyPressed = false;
    }

    // 自动灌溉控制
    static bool f1KeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1KeyPressed) {
        farmStatus.autoIrrigation = !farmStatus.autoIrrigation;
        std::cout << "Auto Irrigation: " << (farmStatus.autoIrrigation ? "ON" : "OFF") << std::endl;
        f1KeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1KeyPressed = false;
    }

    // 病虫防治控制
    static bool f2KeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2KeyPressed) {
        farmStatus.pestControl = !farmStatus.pestControl;
        std::cout << "Pest Control: " << (farmStatus.pestControl ? "ON" : "OFF") << std::endl;
        f2KeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE) {
        f2KeyPressed = false;
    }

    // 自动施肥系统
    static bool f3KeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3KeyPressed) {
        farmStatus.autoFertilizer = !farmStatus.autoFertilizer;
        std::cout << "Auto Fertilizer: " << (farmStatus.autoFertilizer ? "ON" : "OFF") << std::endl;
        f3KeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) {
        f3KeyPressed = false;
    }

    // 自动收获系统
    static bool f4KeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS && !f4KeyPressed) {
        farmStatus.autoHarvest = !farmStatus.autoHarvest;
        std::cout << "Auto Harvest: " << (farmStatus.autoHarvest ? "ON" : "OFF") << std::endl;
        f4KeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_RELEASE) {
        f4KeyPressed = false;
    }

    // 夜间照明系统
    static bool f5KeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS && !f5KeyPressed) {
        farmStatus.nightLighting = !farmStatus.nightLighting;
        std::cout << "Night Lighting: " << (farmStatus.nightLighting ? "ON" : "OFF") << std::endl;
        f5KeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_RELEASE) {
        f5KeyPressed = false;
    }

    // 气候控制系统
    static bool f6KeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS && !f6KeyPressed) {
        farmStatus.climateControl = !farmStatus.climateControl;
        std::cout << "Climate Control: " << (farmStatus.climateControl ? "ON" : "OFF") << std::endl;
        f6KeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_RELEASE) {
        f6KeyPressed = false;
    }

    // 灌溉强度调节
    static bool numKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !numKeyPressed) {
        farmStatus.irrigationIntensity = 1;
        std::cout << "Irrigation Intensity: Low (1/5)" << std::endl;
        numKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !numKeyPressed) {
        farmStatus.irrigationIntensity = 2;
        std::cout << "Irrigation Intensity: Medium-Low (2/5)" << std::endl;
        numKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !numKeyPressed) {
        farmStatus.irrigationIntensity = 3;
        std::cout << "Irrigation Intensity: Medium (3/5)" << std::endl;
        numKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !numKeyPressed) {
        farmStatus.irrigationIntensity = 4;
        std::cout << "Irrigation Intensity: Medium-High (4/5)" << std::endl;
        numKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && !numKeyPressed) {
        farmStatus.irrigationIntensity = 5;
        std::cout << "Irrigation Intensity: High (5/5)" << std::endl;
        numKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE) {
        numKeyPressed = false;
    }

    // 种植模式切换
    static bool mKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed) {
        farmStatus.plantingMode = (farmStatus.plantingMode + 1) % 3;
        const char* modes[] = { "Dense", "Normal", "Sparse" };
        std::cout << "Planting Mode: " << modes[farmStatus.plantingMode] << std::endl;
        mKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        mKeyPressed = false;
    }

    // 自动化级别调节
    static bool lKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed) {
        farmStatus.automationLevel = (farmStatus.automationLevel % 3) + 1;
        const char* levels[] = { "", "Basic", "Advanced", "Full" };
        std::cout << "Automation Level: " << levels[farmStatus.automationLevel] << std::endl;
        lKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        lKeyPressed = false;
    }

    // 自由镜头移动 - 优化
    if (freeCamera) {
        float cameraSpeed = 0.12f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraSpeed *= 2.0f; // 加速

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraUp;
    }
    else {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraAngle -= 0.015f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraAngle += 0.015f;
    }
}

void checkOpenGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL Error in " << operation << ": " << error << std::endl;
    }
}

// 主函数
int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "🚜 优化版智能农场监控系统 - DMT201 Final Project" << std::endl;
    std::cout << "基于原始脚本完整优化 - 保持所有功能" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "🌟 优化内容：" << std::endl;
    std::cout << "   🎮 自由镜头控制优化" << std::endl;
    std::cout << "   🌦️ 天气系统修复" << std::endl;
    std::cout << "   🌱 植物位置对齐" << std::endl;
    std::cout << "   📊 实时参数显示" << std::endl;
    std::cout << "   🚜 农场功能增强" << std::endl;
    std::cout << "================================================" << std::endl;

    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cout << "❌ GLFW初始化失败" << std::endl;
        system("pause");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    g_window = glfwCreateWindow(1400, 900, "优化版智能农场监控系统", NULL, NULL);
    if (g_window == NULL) {
        std::cout << "❌ 窗口创建失败" << std::endl;
        glfwTerminate();
        system("pause");
        return -1;
    }

    glfwMakeContextCurrent(g_window);
    glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(g_window, mouse_callback);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        std::cout << "⚠️ GLEW初始化警告，使用兼容模式" << std::endl;
    }
    else {
        useVAO = GLEW_ARB_vertex_array_object;
        std::cout << "✅ OpenGL已就绪, VAO支持: " << (useVAO ? "是" : "否") << std::endl;
    }

    if (!initializeOpenGL()) {
        std::cout << "❌ 应用程序初始化失败" << std::endl;
        glfwTerminate();
        system("pause");
        return -1;
    }

    std::cout << "Smart Farm System Ready!" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "QUICK START TUTORIAL:" << std::endl;
    std::cout << "  1. Press 'I' to see current farm status" << std::endl;
    std::cout << "  2. Press 'F1' to enable auto irrigation" << std::endl;
    std::cout << "  3. Press 'F6' to enable climate control" << std::endl;
    std::cout << "  4. Press 'T' to change weather and see effects" << std::endl;
    std::cout << "  5. Press 'C' to switch to free camera and explore" << std::endl;
    std::cout << "  6. Watch the colored sensor bars change in real-time!" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "EXPECTED VISUAL EFFECTS:" << std::endl;
    std::cout << "  - Sensor bars will grow/shrink based on conditions" << std::endl;
    std::cout << "  - Plant colors change based on health (greener = healthier)" << std::endl;
    std::cout << "  - Weather affects plant movement (wind animation)" << std::endl;
    std::cout << "  - System messages show when automation works" << std::endl;
    std::cout << "  - Console displays real-time farm data updates" << std::endl;
    std::cout << "========================================================================" << std::endl;
    std::cout << "                        DETAILED OPERATION GUIDE" << std::endl;
    std::cout << "========================================================================" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "BASIC CONTROLS:" << std::endl;
    std::cout << "  C     - Toggle Camera Mode (Free/Auto)" << std::endl;
    std::cout << "  T     - Change Weather (Sunny -> Cloudy -> Rainy -> Stormy)" << std::endl;
    std::cout << "  I     - Toggle Farm Information Display" << std::endl;
    std::cout << "  H     - Toggle Detailed Statistics" << std::endl;
    std::cout << "  ESC   - Exit Program" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "CAMERA CONTROLS:" << std::endl;
    std::cout << "  Free Camera Mode:" << std::endl;
    std::cout << "    W/A/S/D    - Move Forward/Left/Backward/Right" << std::endl;
    std::cout << "    Mouse      - Look Around (360 degrees)" << std::endl;
    std::cout << "    Space      - Move Up" << std::endl;
    std::cout << "    Ctrl       - Move Down" << std::endl;
    std::cout << "    Shift      - Speed Boost (2x faster movement)" << std::endl;
    std::cout << "  Auto Cruise Mode:" << std::endl;
    std::cout << "    Left/Right - Control Orbit Speed" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "AUTOMATED FARM SYSTEMS (F1-F6):" << std::endl;
    std::cout << "  F1 - AUTO IRRIGATION    : Automatic soil watering system" << std::endl;
    std::cout << "       Effect: Maintains soil moisture above 40%" << std::endl;
    std::cout << "       Visual: Watch soil moisture bars (brown) increase" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  F2 - PEST CONTROL       : Disease and pest management" << std::endl;
    std::cout << "       Effect: Cures infected plants, boosts health +15%" << std::endl;
    std::cout << "       Visual: Sick plants become healthier, greener color" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  F3 - AUTO FERTILIZER    : NPK nutrient management" << std::endl;
    std::cout << "       Effect: Boosts N/P/K levels, accelerates growth" << std::endl;
    std::cout << "       Visual: Watch colored nutrient bars increase" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  F4 - AUTO HARVEST       : Automatic crop collection" << std::endl;
    std::cout << "       Effect: Harvests mature plants, replants automatically" << std::endl;
    std::cout << "       Visual: Plants reset to young stage when harvested" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  F5 - NIGHT LIGHTING     : 24/7 growth enhancement" << std::endl;
    std::cout << "       Effect: +8% growth rate, extended growing hours" << std::endl;
    std::cout << "       Visual: Plants grow faster even in dark conditions" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  F6 - CLIMATE CONTROL    : Temperature & humidity management" << std::endl;
    std::cout << "       Effect: Maintains optimal 20-28C, 55-75% humidity" << std::endl;
    std::cout << "       Visual: Temperature (red) and humidity (blue) bars stabilize" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "SYSTEM OPTIMIZATION (1-5, M, L):" << std::endl;
    std::cout << "  1-5  - IRRIGATION INTENSITY:" << std::endl;
    std::cout << "         1 = Low (slow watering, low power)" << std::endl;
    std::cout << "         3 = Medium (balanced efficiency)" << std::endl;
    std::cout << "         5 = High (fast watering, high power)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  M    - PLANTING MODE:" << std::endl;
    std::cout << "         Dense   = More plants, higher yield, more resources" << std::endl;
    std::cout << "         Normal  = Balanced configuration" << std::endl;
    std::cout << "         Sparse  = Fewer plants, easier management" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  L    - AUTOMATION LEVEL:" << std::endl;
    std::cout << "         Basic    = Manual control, lower efficiency" << std::endl;
    std::cout << "         Advanced = Smart automation, balanced efficiency" << std::endl;
    std::cout << "         Full     = Maximum automation, highest efficiency" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "MONITORING GUIDE:" << std::endl;
    std::cout << "  Ground Sensors (7 colored bars around each sensor):" << std::endl;
    std::cout << "    Red     = Temperature  |  Blue   = Humidity" << std::endl;
    std::cout << "    Brown   = Soil Moisture|  Purple = pH Level" << std::endl;
    std::cout << "    Green   = Nitrogen     |  Orange = Phosphorus" << std::endl;
    std::cout << "    Pink    = Potassium    |" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  Status Lights on Sensors:" << std::endl;
    std::cout << "    Green   = All systems normal" << std::endl;
    std::cout << "    Yellow  = Minor issues detected" << std::endl;
    std::cout << "    Red     = Critical alerts requiring attention" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "WEATHER EFFECTS:" << std::endl;
    std::cout << "  Sunny   = Optimal growth, clear visibility" << std::endl;
    std::cout << "  Cloudy  = Reduced light, moderate growth" << std::endl;
    std::cout << "  Rainy   = Natural irrigation, strong wind effects" << std::endl;
    std::cout << "  Stormy  = Extreme wind, challenging conditions" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "TIPS FOR OPTIMAL FARMING:" << std::endl;
    std::cout << "  1. Monitor sensor data regularly (press I to view)" << std::endl;
    std::cout << "  2. Activate Auto Irrigation + Climate Control for best results" << std::endl;
    std::cout << "  3. Use medium irrigation intensity (3) for balanced efficiency" << std::endl;
    std::cout << "  4. Enable Night Lighting for maximum growth rate" << std::endl;
    std::cout << "  5. Watch for red sensor lights indicating problems" << std::endl;
    std::cout << "  6. Different weather affects plant behavior - experiment!" << std::endl;
    std::cout << "========================================================================" << std::endl;

    // 主渲染循环
    float lastFrame = 0.0f;
    int frameCount = 0;
    float lastStatusReport = 0.0f;

    while (!glfwWindowShouldClose(g_window)) {
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        systemTime = currentFrame;
        frameCount++;

        // Farm status report every 6 seconds (更频繁)
        if (currentFrame - lastStatusReport > 6.0f) {
            float fps = frameCount / (currentFrame - lastStatusReport);
            updateFarmStatus();
            if (showUI) {
                std::cout << "Farm Status - FPS: " << (int)fps
                    << " | Weather: " << (weather.weatherType == 0 ? "Sunny" :
                        weather.weatherType == 1 ? "Cloudy" :
                        weather.weatherType == 2 ? "Rainy" : "Stormy")
                    << " | Healthy Plants: " << farmStatus.healthyPlants << "/" << plants.size()
                    << " | Alert Sensors: " << farmStatus.alertSensors << "/" << sensors.size()
                    << " | Auto Systems: " << (farmStatus.autoIrrigation ? "I" : "")
                    << (farmStatus.pestControl ? "P" : "")
                    << (farmStatus.autoFertilizer ? "F" : "")
                    << (farmStatus.autoHarvest ? "H" : "") << std::endl;
                printUIInfo();
            }
            frameCount = 0;
            lastStatusReport = currentFrame;
        }

        processInput(g_window);

        if (isInitialized) {
            updateFarmSimulation(deltaTime);
            render();
        }

        glfwSwapBuffers(g_window);
        glfwPollEvents();
    }

    // Resource cleanup
    std::cout << "Cleaning up system resources..." << std::endl;
    for (auto& obj : renderObjects) obj.cleanup();
    renderObjects.clear();
    sensors.clear();
    plants.clear();
    buildings.clear();
    paths.clear();

    if (shaderProgram != 0) glDeleteProgram(shaderProgram);

    glfwTerminate();
    std::cout << "Smart Farm System shutdown complete. Thank you!" << std::endl;
    system("pause");
    return 0;
}

bool initializeOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);

    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height);
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }

    if (!createShaderProgram()) {
        std::cout << "❌ 着色器程序创建失败" << std::endl;
        return false;
    }

    // 初始化农场系统（保持原有顺序）
    createDetailedBuildings();
    initializeAdvancedSensorNetwork();
    initializeDetailedPlants();
    createBezierPaths();
    generateDetailedFarm();

    // 设置渲染缓冲区
    for (auto& obj : renderObjects) {
        if (!setupBuffers(obj)) {
            std::cout << "⚠️ 部分对象缓冲区设置失败" << std::endl;
        }
    }

    isInitialized = true;
    std::cout << "Farm Component Statistics:" << std::endl;
    std::cout << "   Buildings: " << buildings.size() << " units" << std::endl;
    std::cout << "   Plants: " << plants.size() << " specimens" << std::endl;
    std::cout << "   Sensors: " << sensors.size() << " nodes" << std::endl;
    std::cout << "   Paths: " << paths.size() << " routes" << std::endl;
    std::cout << "   Render Objects: " << renderObjects.size() << " groups" << std::endl;
    return true;
}

bool createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vertexShader == 0) return false;

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "❌ 着色器链接失败: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "❌ 着色器编译失败: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// 创建详细的建筑群（保持原有功能）
void createDetailedBuildings() {
    buildings.clear();

    // 1. 主控制中心 (0,0为中心)
    buildings.emplace_back("控制中心", glm::vec3(0.0f, 0.0f, -15.0f),
        glm::vec3(6.0f, 4.0f, 4.0f), glm::vec3(0.6f, 0.6f, 0.7f));

    // 2. 温室A (东侧)
    buildings.emplace_back("温室A", glm::vec3(12.0f, 0.0f, -8.0f),
        glm::vec3(8.0f, 3.5f, 6.0f), glm::vec3(0.9f, 0.95f, 0.9f));

    // 3. 温室B (西侧)
    buildings.emplace_back("温室B", glm::vec3(-12.0f, 0.0f, -8.0f),
        glm::vec3(8.0f, 3.5f, 6.0f), glm::vec3(0.9f, 0.95f, 0.9f));

    // 4. 仓储中心 (南侧)
    buildings.emplace_back("仓储中心", glm::vec3(0.0f, 0.0f, 15.0f),
        glm::vec3(10.0f, 5.0f, 6.0f), glm::vec3(0.7f, 0.5f, 0.4f));

    // 5. 工具房 (东南)
    buildings.emplace_back("工具房", glm::vec3(15.0f, 0.0f, 8.0f),
        glm::vec3(4.0f, 3.0f, 4.0f), glm::vec3(0.6f, 0.4f, 0.3f));

    // 6. 水处理站 (西南)
    buildings.emplace_back("水处理站", glm::vec3(-15.0f, 0.0f, 8.0f),
        glm::vec3(5.0f, 4.0f, 5.0f), glm::vec3(0.5f, 0.7f, 0.8f));

    std::cout << "Building infrastructure completed - " << buildings.size() << " functional buildings" << std::endl;
}

// 初始化高级传感器网络（修复位置到地面）
void initializeAdvancedSensorNetwork() {
    sensors.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> tempDis(18.0f, 32.0f);
    std::uniform_real_distribution<float> humidDis(40.0f, 85.0f);
    std::uniform_real_distribution<float> soilDis(25.0f, 80.0f);
    std::uniform_real_distribution<float> lightDis(400.0f, 1200.0f);
    std::uniform_real_distribution<float> pHDis(5.5f, 8.0f);
    std::uniform_real_distribution<float> nutrientDis(20.0f, 80.0f);

    // 传感器放置在地面 - 修复高度
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            SensorData sensor;

            // 传感器位置：在地面上，更好分布
            sensor.position = glm::vec3(
                (i - 2) * 7.0f + (j % 2) * 1.0f,   // X轴分布
                0.3f,                               // 地面高度！
                (j - 2) * 7.0f + (i % 2) * 1.0f    // Z轴分布
            );

            sensor.temperature = tempDis(gen);
            sensor.humidity = humidDis(gen);
            sensor.soilMoisture = soilDis(gen);
            sensor.lightLevel = lightDis(gen);
            sensor.pH = pHDis(gen);

            // 营养数据
            sensor.nitrogenLevel = nutrientDis(gen);
            sensor.phosphorusLevel = nutrientDis(gen);
            sensor.potassiumLevel = nutrientDis(gen);

            // 温室区域调整
            bool inGreenhouse = (abs(sensor.position.x) > 6.0f && abs(sensor.position.z) < 10.0f);
            if (inGreenhouse) {
                sensor.temperature += 4.0f;  // 更明显的温室效果
                sensor.humidity += 15.0f;
            }

            // 计算7个数据柱高度 - 增强可见性
            sensor.dataHeight[0] = (sensor.temperature - 15.0f) / 25.0f * 3.0f; // 更高的柱子
            sensor.dataHeight[1] = sensor.humidity / 100.0f * 3.0f;
            sensor.dataHeight[2] = sensor.soilMoisture / 100.0f * 3.0f;
            sensor.dataHeight[3] = (sensor.pH - 5.0f) / 3.5f * 3.0f;
            sensor.dataHeight[4] = sensor.nitrogenLevel / 100.0f * 3.0f;
            sensor.dataHeight[5] = sensor.phosphorusLevel / 100.0f * 3.0f;
            sensor.dataHeight[6] = sensor.potassiumLevel / 100.0f * 3.0f;

            sensor.statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
            sensors.push_back(sensor);
        }
    }

    std::cout << "Ground-based Sensor Network Deployed - " << sensors.size() << " monitoring nodes" << std::endl;
}

// 初始化增强植物系统 - 修复位置对齐
void initializeDetailedPlants() {
    plants.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDis(-18.0f, 18.0f);
    std::uniform_real_distribution<float> heightDis(0.8f, 2.5f);
    std::uniform_real_distribution<float> healthDis(0.7f, 1.0f);
    std::uniform_int_distribution<int> typeDis(0, 3);
    std::uniform_real_distribution<float> phaseDis(0.0f, 6.28f);

    // 生成300个精细植物实例 - 优化位置检测
    for (int i = 0; i < 300; i++) {
        DetailedPlant plant;
        bool validPosition = false;
        int attempts = 0;

        // 尝试找到合适位置
        while (!validPosition && attempts < 15) {
            plant.position = glm::vec3(posDis(gen), 0.0f, posDis(gen));
            validPosition = true;

            // 检查与建筑物重叠 - 优化距离计算
            for (const auto& building : buildings) {
                float dist = glm::length(plant.position - glm::vec3(building.position.x, 0, building.position.z));
                if (dist < 7.0f) { // 增加安全距离
                    validPosition = false;
                    break;
                }
            }

            // 检查与传感器重叠
            if (validPosition) {
                for (const auto& sensor : sensors) {
                    float dist = glm::length(plant.position - glm::vec3(sensor.position.x, 0, sensor.position.z));
                    if (dist < 2.0f) { // 增加传感器周围空间
                        validPosition = false;
                        break;
                    }
                }
            }

            // 检查与其他植物重叠
            if (validPosition) {
                for (const auto& otherPlant : plants) {
                    float dist = glm::length(plant.position - otherPlant.position);
                    if (dist < 1.0f) { // 植物间最小距离
                        validPosition = false;
                        break;
                    }
                }
            }

            attempts++;
        }

        if (!validPosition) continue; // 如果找不到合适位置，跳过这个植物

        plant.plantType = typeDis(gen);
        plant.healthFactor = healthDis(gen);
        plant.windPhase = phaseDis(gen);
        plant.growthStage = 0.6f + healthDis(gen) * 0.4f;

        // 根据植物类型设置参数
        switch (plant.plantType) {
        case 0: // 玉米
            plant.height = heightDis(gen) * 1.2f;
            plant.stemRadius = 0.04f;
            plant.leafCount = 12 + (i % 4);  // 增加叶片数量
            plant.leafSize = 0.25f;
            plant.stemColor = glm::vec3(0.4f, 0.6f, 0.2f);
            plant.leafColor = glm::vec3(0.2f, 0.8f, 0.1f);
            plant.hasFlowers = plant.growthStage > 0.8f;
            break;

        case 1: // 小麦
            plant.height = heightDis(gen) * 0.6f;
            plant.stemRadius = 0.02f;
            plant.leafCount = 8 + (i % 3);
            plant.leafSize = 0.15f;
            plant.stemColor = glm::vec3(0.6f, 0.7f, 0.3f);
            plant.leafColor = glm::vec3(0.3f, 0.7f, 0.2f);
            plant.hasFlowers = plant.growthStage > 0.7f;
            break;

        case 2: // 番茄
            plant.height = heightDis(gen) * 0.8f;
            plant.stemRadius = 0.03f;
            plant.leafCount = 15 + (i % 4);  // 增加叶片数量
            plant.leafSize = 0.2f;
            plant.stemColor = glm::vec3(0.3f, 0.5f, 0.2f);
            plant.leafColor = glm::vec3(0.2f, 0.6f, 0.1f);
            plant.hasFlowers = plant.growthStage > 0.6f;
            plant.hasFruits = plant.growthStage > 0.8f;
            break;

        case 3: // 菠菜
            plant.height = heightDis(gen) * 0.4f;
            plant.stemRadius = 0.015f;
            plant.leafCount = 20 + (i % 5);  // 增加叶片数量
            plant.leafSize = 0.18f;
            plant.stemColor = glm::vec3(0.2f, 0.4f, 0.1f);
            plant.leafColor = glm::vec3(0.1f, 0.5f, 0.1f);
            break;
        }

        // 生成详细的叶片位置数据
        plant.leafPositions.clear();
        plant.leafSizes.clear();
        plant.branchPositions.clear();

        for (int j = 0; j < plant.leafCount; j++) {
            float heightRatio = (float)(j + 1) / (plant.leafCount + 1);
            float angle = j * 137.5f * 3.14159f / 180.0f; // 黄金角度螺旋

            glm::vec3 leafPos = plant.position + glm::vec3(
                cos(angle) * plant.leafSize * (1.0f + heightRatio * 0.5f),
                plant.height * heightRatio * plant.growthStage,
                sin(angle) * plant.leafSize * (1.0f + heightRatio * 0.5f)
            );

            plant.leafPositions.push_back(leafPos);
            plant.leafSizes.push_back(plant.leafSize * (0.7f + heightRatio * 0.5f) * plant.healthFactor);
        }

        // 根系扩展
        plant.rootSpread = plant.leafSize * 1.5f * plant.healthFactor;

        // 随机疾病和虫害
        if (gen() % 100 < 5) { // 5%概率
            plant.isPestInfected = true;
            plant.healthFactor *= 0.7f;
        }

        // 健康度影响颜色
        float healthEffect = plant.healthFactor;
        plant.leafColor *= healthEffect;
        plant.stemColor *= healthEffect;

        plants.push_back(plant);
    }

    std::cout << "Advanced plant ecosystem established - " << plants.size() << " multi-type crops (position optimized)" << std::endl;
}

// 创建贝塞尔曲线路径
void createBezierPaths() {
    paths.clear();

    // 主要道路系统 - 连接各建筑的曲线道路
    BezierPath mainRoad;
    mainRoad.controlPoints = {
        glm::vec3(-20.0f, 0.1f, -20.0f),  // 起点
        glm::vec3(-5.0f, 0.1f, -25.0f),   // 控制点1
        glm::vec3(5.0f, 0.1f, -25.0f),    // 控制点2
        glm::vec3(20.0f, 0.1f, -20.0f)    // 终点
    };
    mainRoad.pathColor = glm::vec3(0.4f, 0.4f, 0.4f); // 灰色道路
    mainRoad.pathWidth = 0.8f;
    mainRoad.segments = 30;
    paths.push_back(mainRoad);

    // 灌溉主管道 - 弯曲的水管系统
    BezierPath irrigation;
    irrigation.controlPoints = {
        glm::vec3(-15.0f, 0.05f, 8.0f),   // 从水处理站开始
        glm::vec3(-5.0f, 0.05f, 12.0f),   // 控制点1
        glm::vec3(5.0f, 0.05f, 12.0f),    // 控制点2
        glm::vec3(15.0f, 0.05f, 8.0f)     // 到工具房
    };
    irrigation.pathColor = glm::vec3(0.2f, 0.6f, 0.8f); // 蓝色管道
    irrigation.pathWidth = 0.3f;
    irrigation.segments = 25;
    paths.push_back(irrigation);

    // 温室连接路径
    BezierPath greenhouseConnection;
    greenhouseConnection.controlPoints = {
        glm::vec3(-12.0f, 0.1f, -8.0f),  // 温室B
        glm::vec3(-6.0f, 0.1f, -5.0f),   // 控制点1
        glm::vec3(6.0f, 0.1f, -5.0f),    // 控制点2
        glm::vec3(12.0f, 0.1f, -8.0f)    // 温室A
    };
    greenhouseConnection.pathColor = glm::vec3(0.5f, 0.5f, 0.5f);
    greenhouseConnection.pathWidth = 0.6f;
    greenhouseConnection.segments = 20;
    paths.push_back(greenhouseConnection);

    std::cout << "Bezier curve path system created - " << paths.size() << " intelligent routes" << std::endl;
}

// 更新农场模拟 - 优化
void updateFarmSimulation(float deltaTime) {
    // 动态天气系统更新 - 修复
    static float weatherTimer = 0.0f;
    weatherTimer += deltaTime;

    // 根据天气类型更新参数 - 修复雨天和暴风雨
    switch (weather.weatherType) {
    case 0: // 晴天
        weather.cloudCoverage = 0.1f + sin(systemTime * 0.1f) * 0.1f;
        weather.precipitation = 0.0f;
        windStrength = 0.3f + sin(systemTime * 0.5f) * 0.2f;
        weather.fogDensity = 0.005f;
        weather.fogColor = glm::vec3(0.9f, 0.9f, 1.0f);
        break;
    case 1: // 多云
        weather.cloudCoverage = 0.5f + sin(systemTime * 0.15f) * 0.2f;
        weather.precipitation = 0.0f;
        windStrength = 0.5f + sin(systemTime * 0.8f) * 0.3f;
        weather.fogDensity = 0.01f;
        weather.fogColor = glm::vec3(0.8f, 0.8f, 0.9f);
        break;
    case 2: // 雨天 - 修复
        weather.cloudCoverage = clamp(0.8f + sin(systemTime * 0.2f) * 0.15f, 0.7f, 0.95f);
        weather.precipitation = clamp(0.6f + sin(systemTime * 1.2f) * 0.3f, 0.4f, 0.9f);
        windStrength = clamp(0.7f + sin(systemTime * 1.0f) * 0.4f, 0.5f, 1.1f);
        weather.fogDensity = 0.025f;
        weather.fogColor = glm::vec3(0.65f, 0.65f, 0.75f);
        break;
    case 3: // 暴风雨 - 修复
        weather.cloudCoverage = clamp(0.95f + sin(systemTime * 0.3f) * 0.05f, 0.9f, 1.0f);
        weather.precipitation = clamp(0.85f + sin(systemTime * 2.0f) * 0.15f, 0.7f, 1.0f);
        windStrength = clamp(1.5f + sin(systemTime * 1.5f) * 0.8f, 1.0f, 2.3f);
        weather.fogDensity = 0.04f;
        weather.fogColor = glm::vec3(0.4f, 0.4f, 0.55f);
        break;
    }

    // 昼夜循环 (90秒一个周期)
    dayNightCycle += deltaTime / 90.0f;
    if (dayNightCycle > 1.0f) dayNightCycle -= 1.0f;

    // 动态风效果
    windDirection.x = cos(systemTime * 0.3f) + sin(systemTime * 0.8f) * 0.5f;
    windDirection.y = sin(systemTime * 0.4f) + cos(systemTime * 1.1f) * 0.4f;
    windDirection = glm::normalize(windDirection);

    // 更新传感器数据 (每2秒更频繁更新)
    static float lastSensorUpdate = 0.0f;
    if (systemTime - lastSensorUpdate > 2.0f) {
        lastSensorUpdate = systemTime;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> variation(-1.0f, 1.0f);

        float dayFactor = (sin(dayNightCycle * 2.0f * 3.14159f) + 1.0f) * 0.5f;

        for (auto& sensor : sensors) {
            // 温度随昼夜和天气变化
            sensor.temperature += variation(gen) * 0.8f;
            sensor.temperature += (dayFactor - 0.5f) * 3.0f; // 昼夜温差

            // 天气影响
            if (weather.weatherType >= 2) { // 雨天/暴风雨
                sensor.temperature -= 2.0f; // 降温
                sensor.humidity += 15.0f;   // 增湿
                sensor.soilMoisture += 10.0f; // 土壤湿润
            }

            sensor.temperature = clamp(sensor.temperature, 12.0f, 40.0f);

            // 湿度变化
            sensor.humidity += variation(gen) * 2.0f;
            sensor.humidity = clamp(sensor.humidity, 25.0f, 95.0f);

            // 土壤湿度 (考虑灌溉和蒸发)
            sensor.soilMoisture += variation(gen) * 1.5f;
            sensor.soilMoisture -= dayFactor * 0.5f; // 白天蒸发

            // 超级明显的灌溉系统效果
            bool needsIrrigation = sensor.soilMoisture < 40.0f;
            if (farmStatus.autoIrrigation && needsIrrigation) {
                float irrigationEffect = 8.0f + farmStatus.irrigationIntensity * 5.0f;
                sensor.soilMoisture += irrigationEffect;
                farmStatus.waterUsage += 0.2f * farmStatus.irrigationIntensity;
                farmStatus.irrigationActive = true;
                farmStatus.activeNozzles++;

                // 水箱水位下降
                farmStatus.waterTankLevel -= 0.1f * farmStatus.irrigationIntensity;
                farmStatus.waterTankLevel = clamp(farmStatus.waterTankLevel, 10.0f, 100.0f);

                std::cout << "IRRIGATION SYSTEM ACTIVE - Sensor " << (&sensor - &sensors[0])
                    << " | Soil +" << irrigationEffect << "% | Water Tank: "
                    << (int)farmStatus.waterTankLevel << "% | Pressure: "
                    << (int)farmStatus.waterPressure << " PSI" << std::endl;
            }

            // 施肥系统超明显效果
            if (farmStatus.autoFertilizer && farmStatus.fertilizerLevel > 10.0f) {
                bool needsFertilizer = sensor.nitrogenLevel < 60.0f ||
                    sensor.phosphorusLevel < 60.0f ||
                    sensor.potassiumLevel < 60.0f;
                if (needsFertilizer) {
                    sensor.nitrogenLevel += 8.0f;    // 大幅增加
                    sensor.phosphorusLevel += 6.0f;
                    sensor.potassiumLevel += 7.0f;
                    farmStatus.fertilizerLevel -= 0.5f;
                    std::cout << "FERTILIZER APPLIED at sensor " << (&sensor - &sensors[0])
                        << " - NPK levels boosted!" << std::endl;
                }
            }

            // 气候控制超明显效果
            if (farmStatus.climateControl) {
                bool climateAdjusted = false;
                if (sensor.temperature > 28.0f) {
                    sensor.temperature -= 3.0f;  // 大幅调整
                    climateAdjusted = true;
                }
                if (sensor.temperature < 20.0f) {
                    sensor.temperature += 3.0f;
                    climateAdjusted = true;
                }
                if (sensor.humidity < 55.0f) {
                    sensor.humidity += 5.0f;
                    climateAdjusted = true;
                }
                if (sensor.humidity > 75.0f) {
                    sensor.humidity -= 5.0f;
                    climateAdjusted = true;
                }
                if (climateAdjusted) {
                    std::cout << "CLIMATE CONTROL adjusted sensor " << (&sensor - &sensors[0])
                        << " - Temperature: " << sensor.temperature
                        << "C, Humidity: " << sensor.humidity << "%" << std::endl;
                }
            }

            sensor.soilMoisture = clamp(sensor.soilMoisture, 15.0f, 85.0f);

            // pH值缓慢变化
            sensor.pH += variation(gen) * 0.1f;
            sensor.pH = clamp(sensor.pH, 5.0f, 8.5f);

            // 营养元素变化
            sensor.nitrogenLevel += variation(gen) * 2.0f;
            sensor.phosphorusLevel += variation(gen) * 1.5f;
            sensor.potassiumLevel += variation(gen) * 2.0f;

            sensor.nitrogenLevel = clamp(sensor.nitrogenLevel, 10.0f, 90.0f);
            sensor.phosphorusLevel = clamp(sensor.phosphorusLevel, 10.0f, 90.0f);
            sensor.potassiumLevel = clamp(sensor.potassiumLevel, 10.0f, 90.0f);

            // 光照强度
            sensor.lightLevel = 200.0f + dayFactor * 1000.0f + variation(gen) * 100.0f;
            sensor.lightLevel = clamp(sensor.lightLevel, 100.0f, 1400.0f);

            // 更新7个可视化数据柱
            sensor.dataHeight[0] = (sensor.temperature - 10.0f) / 35.0f * 2.5f;
            sensor.dataHeight[1] = sensor.humidity / 100.0f * 2.5f;
            sensor.dataHeight[2] = sensor.soilMoisture / 100.0f * 2.5f;
            sensor.dataHeight[3] = (sensor.pH - 4.5f) / 4.5f * 2.5f;
            sensor.dataHeight[4] = sensor.nitrogenLevel / 100.0f * 2.5f;
            sensor.dataHeight[5] = sensor.phosphorusLevel / 100.0f * 2.5f;
            sensor.dataHeight[6] = sensor.potassiumLevel / 100.0f * 2.5f;

            // 状态指示灯逻辑
            bool tempAlert = (sensor.temperature > 35.0f || sensor.temperature < 15.0f);
            bool humidAlert = (sensor.humidity < 30.0f);
            bool soilAlert = (sensor.soilMoisture < 25.0f);
            bool pHAlert = (sensor.pH < 5.8f || sensor.pH > 7.8f);
            bool nutrientAlert = (sensor.nitrogenLevel < 30.0f || sensor.phosphorusLevel < 20.0f);

            if (tempAlert || humidAlert || soilAlert || pHAlert || nutrientAlert) {
                sensor.statusColor = glm::vec3(1.0f, 0.2f, 0.2f); // 红色警告
            }
            else if (sensor.temperature > 32.0f || sensor.humidity < 40.0f || sensor.soilMoisture < 35.0f) {
                sensor.statusColor = glm::vec3(1.0f, 0.8f, 0.0f); // 黄色注意
            }
            else {
                sensor.statusColor = glm::vec3(0.2f, 1.0f, 0.3f); // 绿色正常
            }
        }
    }

    // 更新植物生长和健康度
    for (auto& plant : plants) {
        // 天气对植物的影响
        float weatherEffect = 1.0f;
        if (weather.weatherType == 0) weatherEffect = 1.01f;      // 晴天有利
        else if (weather.weatherType == 1) weatherEffect = 1.005f; // 多云中性
        else if (weather.weatherType == 2) weatherEffect = 1.002f; // 雨天适度有利
        else if (weather.weatherType == 3) weatherEffect = 0.995f; // 暴风雨不利

        // 超级明显的农场系统对植物的影响
        if (farmStatus.pestControl && plant.isPestInfected) {
            plant.isPestInfected = false;
            plant.healthFactor = std::min(1.0f, plant.healthFactor + 0.15f); // 大幅提升
            std::cout << "PEST CONTROL cured plant at (" << plant.position.x
                << ", " << plant.position.z << ") - Health +" << 0.15f << std::endl;
        }

        // 施肥系统对植物的超明显影响
        if (farmStatus.autoFertilizer && farmStatus.fertilizerLevel > 20.0f) {
            float oldHealth = plant.healthFactor;
            plant.healthFactor = std::min(1.0f, plant.healthFactor + 0.005f); // 增加效果
            plant.growthStage = std::min(1.0f, plant.growthStage + 0.002f);   // 加速生长
            if (plant.healthFactor > oldHealth) {
                static int fertilizerCount = 0;
                if (++fertilizerCount % 50 == 0) { // 每50次输出一次避免刷屏
                    std::cout << "FERTILIZER boosting plant growth - "
                        << fertilizerCount << " plants enhanced!" << std::endl;
                }
            }
        }

        // 夜间照明超明显延长生长时间
        if (farmStatus.nightLighting) {
            weatherEffect *= 1.08f; // 大幅生长奖励
            plant.growthStage += deltaTime * 0.001f; // 额外夜间生长
            static int lightingBonusCount = 0;
            if (++lightingBonusCount % 100 == 0) {
                std::cout << "NIGHT LIGHTING providing 24/7 growth boost - "
                    << lightingBonusCount << " growth cycles enhanced!" << std::endl;
            }
        }

        // 自动收获成熟植物 - 更明显的效果
        if (farmStatus.autoHarvest && plant.growthStage > 0.9f && plant.healthFactor > 0.75f) {
            farmStatus.harvestYield += 0.5f; // 增加收获量
            plant.growthStage = 0.2f; // 重新种植
            plant.healthFactor = 0.8f; // 新植物起始健康度
            std::cout << "AUTO HARVEST collected mature plant! Total yield: "
                << std::fixed << std::setprecision(1) << farmStatus.harvestYield << " kg" << std::endl;
        }

        // 气候控制提供超稳定生长环境
        if (farmStatus.climateControl) {
            weatherEffect = std::max(weatherEffect, 1.05f); // 确保优秀生长率
            plant.healthFactor = std::min(1.0f, plant.healthFactor + 0.0005f); // 长期健康提升
        }

        // 健康度微调
        plant.healthFactor *= weatherEffect;
        plant.healthFactor = clamp(plant.healthFactor, 0.4f, 1.0f);

        // 缓慢生长
        plant.growthStage += deltaTime * 0.002f * plant.healthFactor;
        plant.growthStage = clamp(plant.growthStage, 0.0f, 1.0f);

        // 根据健康度和生长阶段调整外观
        float healthEffect = plant.healthFactor * plant.growthStage;
        plant.leafColor = glm::vec3(
            0.1f + (1.0f - healthEffect) * 0.2f,  // 不健康时稍微偏红
            0.3f + healthEffect * 0.5f,           // 健康时更绿
            0.1f
        );

        // 更新叶片位置（重新计算以反映生长）
        if ((int)plant.leafPositions.size() != plant.leafCount) {
            plant.leafPositions.clear();
            plant.leafSizes.clear();

            for (int j = 0; j < plant.leafCount; j++) {
                float heightRatio = (float)(j + 1) / (plant.leafCount + 1);
                float angle = j * 137.5f * 3.14159f / 180.0f; // 黄金角度螺旋

                glm::vec3 leafPos = plant.position + glm::vec3(
                    cos(angle) * plant.leafSize * (1.0f + heightRatio * 0.5f),
                    plant.height * heightRatio * plant.growthStage,
                    sin(angle) * plant.leafSize * (1.0f + heightRatio * 0.5f)
                );

                plant.leafPositions.push_back(leafPos);
                plant.leafSizes.push_back(plant.leafSize * (0.7f + heightRatio * 0.5f) * plant.healthFactor);
            }
        }

        // 开花结果逻辑
        if (plant.plantType == 2) { // 番茄
            plant.hasFlowers = plant.growthStage > 0.6f;
            plant.hasFruits = plant.growthStage > 0.8f && plant.healthFactor > 0.7f;
        }
        else if (plant.plantType == 0) { // 玉米
            plant.hasFlowers = plant.growthStage > 0.8f;
        }
        else if (plant.plantType == 1) { // 小麦
            plant.hasFlowers = plant.growthStage > 0.7f;
        }
    }

    // 更新光照位置 (太阳轨迹)
    float sunAngle = dayNightCycle * 2.0f * 3.14159f;
    lightPos = glm::vec3(
        cos(sunAngle) * 25.0f,
        5.0f + sin(sunAngle) * 15.0f,
        sin(sunAngle) * 15.0f
    );

    // 动态光照颜色
    float dayIntensity = clamp(lightPos.y / 20.0f, 0.0f, 1.0f);
    if (dayIntensity > 0.1f) {
        lightColor = glm::mix(glm::vec3(1.0f, 0.7f, 0.4f), glm::vec3(1.0f, 0.95f, 0.8f), dayIntensity);
    }
    else {
        lightColor = glm::vec3(0.3f, 0.4f, 0.8f); // 夜晚蓝色调
    }

    // 根据天气调整光照颜色
    switch (weather.weatherType) {
    case 1: // 多云
        lightColor *= 0.9f;
        break;
    case 2: // 雨天
        lightColor *= 0.7f;
        break;
    case 3: // 暴风雨
        lightColor *= 0.5f;
        break;
    }
}

// 更新农场状态 - 超级明显版
void updateFarmStatus() {
    // 统计健康植物 - 更详细
    farmStatus.healthyPlants = 0;
    farmStatus.sickPlants = 0;
    int excellentPlants = 0;
    int criticalPlants = 0;

    for (const auto& plant : plants) {
        if (plant.healthFactor > 0.9f && !plant.isPestInfected) {
            excellentPlants++;
            farmStatus.healthyPlants++;
        }
        else if (plant.healthFactor > 0.7f && !plant.isPestInfected) {
            farmStatus.healthyPlants++;
        }
        else if (plant.healthFactor < 0.5f || plant.isPestInfected) {
            criticalPlants++;
            farmStatus.sickPlants++;
        }
        else {
            farmStatus.sickPlants++;
        }
    }

    // 统计警报传感器和计算平均值 - 更详细
    farmStatus.alertSensors = 0;
    int warningeSensors = 0;
    int perfectSensors = 0;
    float totalTemp = 0.0f, totalHumid = 0.0f, totalSoil = 0.0f;

    for (const auto& sensor : sensors) {
        totalTemp += sensor.temperature;
        totalHumid += sensor.humidity;
        totalSoil += sensor.soilMoisture;

        if (sensor.statusColor.r > 0.8f) { // 红色警报
            farmStatus.alertSensors++;
        }
        else if (sensor.statusColor.r > 0.7f || sensor.statusColor.g < 0.9f) { // 黄色警告
            warningeSensors++;
        }
        else { // 绿色正常
            perfectSensors++;
        }
    }

    if (!sensors.empty()) {
        farmStatus.avgTemperature = totalTemp / sensors.size();
        farmStatus.avgHumidity = totalHumid / sensors.size();
        farmStatus.avgSoilMoisture = totalSoil / sensors.size();
    }

    // 计算功耗（超详细）
    farmStatus.powerConsumption = 28.5f; // 基础功耗
    if (farmStatus.autoIrrigation) farmStatus.powerConsumption += 6.2f * farmStatus.irrigationIntensity;
    if (farmStatus.pestControl) farmStatus.powerConsumption += 8.1f;      // 增加
    if (farmStatus.autoFertilizer) farmStatus.powerConsumption += 12.8f;  // 增加
    if (farmStatus.autoHarvest) farmStatus.powerConsumption += 18.5f;     // 增加
    if (farmStatus.nightLighting) farmStatus.powerConsumption += 25.3f;   // 增加
    if (farmStatus.climateControl) farmStatus.powerConsumption += 35.7f;  // 增加

    // 根据自动化级别调整效率
    float automationMultiplier = 1.0f + (farmStatus.automationLevel - 1) * 0.25f;
    farmStatus.powerConsumption *= automationMultiplier;

    // 施肥水平管理 - 更动态
    if (farmStatus.autoFertilizer && farmStatus.fertilizerLevel < 100.0f) {
        farmStatus.fertilizerLevel += 1.5f; // 更快补充
    }
    else if (!farmStatus.autoFertilizer && farmStatus.fertilizerLevel > 0.0f) {
        farmStatus.fertilizerLevel -= 0.3f; // 更快消耗
    }
    farmStatus.fertilizerLevel = clamp(farmStatus.fertilizerLevel, 0.0f, 100.0f);

    // 收获产量计算 - 更现实
    if (farmStatus.autoHarvest) {
        farmStatus.harvestYield += excellentPlants * 0.005f + farmStatus.healthyPlants * 0.002f;
    }

    // 输出详细状态变化
    static float lastReport = 0.0f;
    if (systemTime - lastReport > 10.0f) {
        std::cout << "=== FARM STATUS SUMMARY ===" << std::endl;
        std::cout << "Plant Health: " << excellentPlants << " excellent, "
            << farmStatus.healthyPlants << " healthy, " << farmStatus.sickPlants << " sick, "
            << criticalPlants << " critical" << std::endl;
        std::cout << "Sensor Status: " << perfectSensors << " perfect, " << warningeSensors
            << " warning, " << farmStatus.alertSensors << " alert" << std::endl;
        std::cout << "Active Systems: ";
        if (farmStatus.autoIrrigation) std::cout << "IRRIGATION ";
        if (farmStatus.pestControl) std::cout << "PEST_CONTROL ";
        if (farmStatus.autoFertilizer) std::cout << "FERTILIZER ";
        if (farmStatus.autoHarvest) std::cout << "HARVEST ";
        if (farmStatus.nightLighting) std::cout << "LIGHTING ";
        if (farmStatus.climateControl) std::cout << "CLIMATE ";
        std::cout << std::endl;
        lastReport = systemTime;
    }
}

// 打印UI信息 - 英文版
void printUIInfo() {
    const char* weatherNames[] = { "Sunny", "Cloudy", "Rainy", "Stormy" };
    const char* plantingModes[] = { "Dense", "Normal", "Sparse" };
    const char* automationLevels[] = { "", "Basic", "Advanced", "Full" };

    std::cout << "\n==================== Smart Farm Control Panel ====================" << std::endl;
    std::cout << "Weather: " << weatherNames[weather.weatherType]
        << " | Cloud Cover: " << (int)(weather.cloudCoverage * 100) << "%" << std::endl;

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Temperature: " << farmStatus.avgTemperature << "C"
        << " | Humidity: " << farmStatus.avgHumidity << "%" << std::endl;

    std::cout << "Healthy Plants: " << farmStatus.healthyPlants << "/" << plants.size()
        << " | Sick Plants: " << farmStatus.sickPlants << std::endl;

    std::cout << "Alert Sensors: " << farmStatus.alertSensors << "/" << sensors.size()
        << " | Power Usage: " << farmStatus.powerConsumption << " kW" << std::endl;

    std::cout << "Auto Irrigation: " << (farmStatus.autoIrrigation ? "ON" : "OFF")
        << " | Pest Control: " << (farmStatus.pestControl ? "ON" : "OFF") << std::endl;

    std::cout << "Auto Fertilizer: " << (farmStatus.autoFertilizer ? "ON" : "OFF")
        << " | Auto Harvest: " << (farmStatus.autoHarvest ? "ON" : "OFF") << std::endl;

    if (showDetailedStats) {
        std::cout << "Soil Moisture: " << farmStatus.avgSoilMoisture << "%"
            << " | Water Usage: " << farmStatus.waterUsage << " L" << std::endl;
        std::cout << "Night Lighting: " << (farmStatus.nightLighting ? "ON" : "OFF")
            << " | Climate Control: " << (farmStatus.climateControl ? "ON" : "OFF") << std::endl;
        std::cout << "Irrigation Intensity: " << farmStatus.irrigationIntensity << "/5"
            << " | Fertilizer Level: " << (int)farmStatus.fertilizerLevel << "%" << std::endl;
        std::cout << "Planting Mode: " << plantingModes[farmStatus.plantingMode]
            << " | Automation: " << automationLevels[farmStatus.automationLevel] << std::endl;
        std::cout << "Harvest Yield: " << std::setprecision(2) << farmStatus.harvestYield << " kg"
            << " | Buildings: " << buildings.size()
            << " | Sensors: " << sensors.size() << std::endl;
    }

    std::cout << "==================================================================" << std::endl;
}

// 生成详细农场场景（保持原有所有功能）
void generateDetailedFarm() {
    renderObjects.clear();

    // 1. 地面系统 - 分区域不同材质
    RenderObject ground;
    ground.transparent = false;
    ground.castShadow = false;

    // 主要农田区域 (深棕色土壤)
    for (int i = -25; i <= 25; i++) {
        for (int j = -25; j <= 25; j++) {
            float distance = sqrt(i * i + j * j);
            if (distance < 22.0f) {
                float soilVariation = sin(i * 0.1f) * cos(j * 0.1f) * 0.05f;
                glm::vec3 soilColor = glm::vec3(0.3f + soilVariation, 0.2f + soilVariation, 0.1f);
                addDetailedCube(ground, glm::vec3(i * 2.0f, -0.1f, j * 2.0f),
                    glm::vec3(2.0f, 0.2f, 2.0f), soilColor, glm::vec3(0, 1, 0), 3.0f);
            }
        }
    }
    renderObjects.push_back(std::move(ground));

    // 2. 道路网络系统
    RenderObject roads;
    roads.transparent = false;
    roads.castShadow = false;

    // 主干道 (十字形)
    for (int i = -25; i <= 25; i++) {
        // 南北主干道
        addDetailedCube(roads, glm::vec3(0.0f, 0.0f, i * 2.0f),
            glm::vec3(3.0f, 0.1f, 2.0f), glm::vec3(0.4f, 0.4f, 0.4f));
        // 东西主干道
        addDetailedCube(roads, glm::vec3(i * 2.0f, 0.0f, 0.0f),
            glm::vec3(2.0f, 0.1f, 3.0f), glm::vec3(0.4f, 0.4f, 0.4f));
    }

    // 连接道路到各个建筑
    for (const auto& building : buildings) {
        int steps = 8;
        glm::vec3 start = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 end = building.position;
        for (int i = 0; i < steps; i++) {
            float t = (float)i / steps;
            glm::vec3 pos = glm::mix(start, end, t);
            addDetailedCube(roads, pos + glm::vec3(0, 0.05f, 0),
                glm::vec3(1.5f, 0.1f, 1.5f), glm::vec3(0.45f, 0.45f, 0.45f));
        }
    }
    renderObjects.push_back(std::move(roads));

    // 3. 贝塞尔曲线路径系统
    RenderObject pathsObj;
    pathsObj.transparent = false;
    pathsObj.castShadow = false;

    for (const auto& path : paths) {
        addBezierCurve(pathsObj, path);
    }
    renderObjects.push_back(std::move(pathsObj));

    // 4. 建筑群渲染
    for (const auto& building : buildings) {
        RenderObject buildingObj;
        createBuildingGeometry(buildingObj, building);
        renderObjects.push_back(std::move(buildingObj));
    }

    // 5. 传感器网络可视化
    RenderObject sensorNetwork;
    sensorNetwork.transparent = false;

    for (const auto& sensor : sensors) {
        // 地面传感器支柱 (更短，贴地)
        addCylinder(sensorNetwork, sensor.position,
            sensor.position + glm::vec3(0, 1.5f, 0), 0.08f,  // 降低高度，增加粗细
            glm::vec3(0.8f, 0.8f, 0.9f), 8, 0.0f);

        // 传感器设备舱 (更大更明显)
        addDetailedCube(sensorNetwork, sensor.position + glm::vec3(0, 1.3f, 0),
            glm::vec3(0.25f, 0.3f, 0.25f), glm::vec3(0.9f, 0.5f, 0.2f),  // 增大尺寸
            glm::vec3(0, 1, 0), 4.0f);

        // 状态指示灯 (更大更亮)
        addDetailedCube(sensorNetwork, sensor.position + glm::vec3(0, 1.6f, 0),
            glm::vec3(0.06f, 0.06f, 0.06f), sensor.statusColor,  // 更大的指示灯
            glm::vec3(0, 1, 0), 4.0f);

        // 七个数据可视化柱 (更高更明显)
        glm::vec3 dataColors[7] = {
            glm::vec3(1.0f, 0.3f, 0.3f), // 温度 - 红色
            glm::vec3(0.3f, 0.3f, 1.0f), // 湿度 - 蓝色
            glm::vec3(0.6f, 0.4f, 0.2f), // 土壤 - 棕色
            glm::vec3(0.8f, 0.2f, 0.8f), // pH - 紫色
            glm::vec3(0.2f, 0.8f, 0.2f), // 氮 - 绿色
            glm::vec3(0.8f, 0.6f, 0.2f), // 磷 - 橙色
            glm::vec3(0.6f, 0.2f, 0.8f)  // 钾 - 紫红色
        };

        for (int i = 0; i < 7; i++) {
            float angle = i * 51.43f * 3.14159f / 180.0f; // 360/7度
            glm::vec3 offset = glm::vec3(cos(angle) * 0.5f, 0, sin(angle) * 0.5f);  // 增大半径
            glm::vec3 columnPos = sensor.position + offset + glm::vec3(0, sensor.dataHeight[i] * 0.5f, 0);

            addDetailedCube(sensorNetwork, columnPos,
                glm::vec3(0.1f, sensor.dataHeight[i], 0.1f),  // 更粗的数据柱
                dataColors[i], glm::vec3(0, 1, 0), 4.0f);
        }
    }
    renderObjects.push_back(std::move(sensorNetwork));

    // 6. 精细植物群渲染
    RenderObject plantGroup;
    plantGroup.transparent = false;

    for (const auto& plant : plants) {
        createDetailedPlantGeometry(plantGroup, plant);
    }
    renderObjects.push_back(std::move(plantGroup));

    // 7. 增强灌溉系统（明显的视觉效果）
    RenderObject irrigation;
    irrigation.transparent = false;

    // 主水管网络 - 更大更明显
    for (int i = -20; i <= 20; i += 4) {
        for (int j = -20; j <= 20; j += 4) {
            // 主水管 (更粗的蓝色管道)
            addCylinder(irrigation, glm::vec3(i, 0.15f, j),
                glm::vec3(i + 4, 0.15f, j), 0.12f,  // 增加半径
                glm::vec3(0.2f, 0.5f, 0.9f), 8, 0.0f);

            // 垂直供水管
            addCylinder(irrigation, glm::vec3(i, 0.15f, j),
                glm::vec3(i, 0.15f, j + 4), 0.12f,
                glm::vec3(0.2f, 0.5f, 0.9f), 8, 0.0f);

            // 大型喷头 (更明显)
            addDetailedCube(irrigation, glm::vec3(i, 0.5f, j),
                glm::vec3(0.15f, 0.2f, 0.15f),  // 增大尺寸
                glm::vec3(0.7f, 0.8f, 0.9f), glm::vec3(0, 1, 0), 0.0f);

            // 喷水效果指示器 (当灌溉激活时)
            if (farmStatus.irrigationActive && (i + j) % 8 == 0) {  // 部分喷头激活
                // 水珠效果 (小蓝色圆球)
                for (int k = 0; k < 6; k++) {
                    float angle = k * 60.0f * 3.14159f / 180.0f;
                    float radius = 0.8f + sin(systemTime * 3.0f + k) * 0.3f;
                    glm::vec3 dropPos = glm::vec3(i, 0.7f, j) + glm::vec3(
                        cos(angle) * radius,
                        sin(systemTime * 2.0f + k) * 0.5f,
                        sin(angle) * radius
                    );
                    addDetailedCube(irrigation, dropPos,
                        glm::vec3(0.03f, 0.03f, 0.03f),
                        glm::vec3(0.3f, 0.7f, 1.0f), glm::vec3(0, 1, 0), 0.0f);
                }
            }
        }
    }

    // 主水源连接 (从水处理站)
    addCylinder(irrigation, glm::vec3(-15.0f, 0.15f, 8.0f),
        glm::vec3(-20.0f, 0.15f, 0.0f), 0.2f,  // 主供水管道
        glm::vec3(0.1f, 0.4f, 0.8f), 8, 0.0f);

    // 水泵站指示器
    addDetailedCube(irrigation, glm::vec3(-22.0f, 1.0f, 0.0f),
        glm::vec3(0.8f, 0.6f, 0.8f),
        glm::vec3(0.6f, 0.7f, 0.8f), glm::vec3(0, 1, 0), 0.0f);

    renderObjects.push_back(std::move(irrigation));

    // 8. 围栏系统（保持原有功能）
    RenderObject fencing;
    fencing.transparent = false;

    // 外围围栏
    for (int i = -25; i <= 25; i += 2) {
        // 北侧和南侧围栏
        addDetailedCube(fencing, glm::vec3(i * 2.0f, 1.2f, -50.0f),
            glm::vec3(0.1f, 2.4f, 0.1f), glm::vec3(0.6f, 0.4f, 0.2f));
        addDetailedCube(fencing, glm::vec3(i * 2.0f, 1.2f, 50.0f),
            glm::vec3(0.1f, 2.4f, 0.1f), glm::vec3(0.6f, 0.4f, 0.2f));

        // 东侧和西侧围栏
        addDetailedCube(fencing, glm::vec3(-50.0f, 1.2f, i * 2.0f),
            glm::vec3(0.1f, 2.4f, 0.1f), glm::vec3(0.6f, 0.4f, 0.2f));
        addDetailedCube(fencing, glm::vec3(50.0f, 1.2f, i * 2.0f),
            glm::vec3(0.1f, 2.4f, 0.1f), glm::vec3(0.6f, 0.4f, 0.2f));
    }
    renderObjects.push_back(std::move(fencing));

    std::cout << "Complete optimized farm scene construction finished - " << renderObjects.size() << " render groups" << std::endl;
}

// 创建建筑几何体（保持原有功能）
void createBuildingGeometry(RenderObject& obj, const Building& building) {
    obj.transparent = (building.name == "温室A" || building.name == "温室B");

    glm::vec3 pos = building.position;
    glm::vec3 size = building.size;
    glm::vec3 color = building.color;

    // 主体建筑
    addDetailedCube(obj, pos + glm::vec3(0, size.y * 0.5f, 0), size, color, glm::vec3(0, 1, 0), 0.0f);

    // 屋顶 (稍大一点，不同颜色)
    glm::vec3 roofColor = color * 0.7f;
    addDetailedCube(obj, pos + glm::vec3(0, size.y + 0.3f, 0),
        glm::vec3(size.x + 0.5f, 0.6f, size.z + 0.5f), roofColor, glm::vec3(0, 1, 0), 0.0f);

    // 门 (深色)
    if (building.hasDoor) {
        glm::vec3 doorPos = pos + glm::vec3(0, 1.0f, size.z * 0.5f + 0.1f);
        addDetailedCube(obj, doorPos, glm::vec3(0.8f, 2.0f, 0.1f),
            glm::vec3(0.3f, 0.2f, 0.1f), glm::vec3(0, 0, 1), 0.0f);

        // 门把手
        addDetailedCube(obj, doorPos + glm::vec3(0.3f, 0, 0.05f),
            glm::vec3(0.05f, 0.05f, 0.05f), glm::vec3(0.8f, 0.7f, 0.3f), glm::vec3(0, 0, 1), 0.0f);
    }

    // 窗户
    if (building.hasWindows) {
        // 前面窗户
        for (int i = -1; i <= 1; i += 2) {
            glm::vec3 windowPos = pos + glm::vec3(i * size.x * 0.25f, size.y * 0.6f, size.z * 0.5f + 0.05f);
            glm::vec3 windowColor = (building.name.find("温室") != std::string::npos) ?
                glm::vec3(0.8f, 0.9f, 0.95f) : glm::vec3(0.7f, 0.8f, 0.9f);
            addDetailedCube(obj, windowPos, glm::vec3(0.8f, 1.0f, 0.05f),
                windowColor, glm::vec3(0, 0, 1), 2.0f);
        }

        // 侧面窗户
        for (int i = -1; i <= 1; i += 2) {
            glm::vec3 windowPos = pos + glm::vec3(size.x * 0.5f + 0.05f, size.y * 0.6f, i * size.z * 0.25f);
            glm::vec3 windowColor = (building.name.find("温室") != std::string::npos) ?
                glm::vec3(0.8f, 0.9f, 0.95f) : glm::vec3(0.7f, 0.8f, 0.9f);
            addDetailedCube(obj, windowPos, glm::vec3(0.05f, 1.0f, 0.8f),
                windowColor, glm::vec3(1, 0, 0), 2.0f);
        }
    }

    // 特殊建筑装饰
    if (building.name == "控制中心") {
        // 通信天线
        addCylinder(obj, pos + glm::vec3(0, size.y + 0.6f, 0),
            pos + glm::vec3(0, size.y + 3.0f, 0), 0.05f,
            glm::vec3(0.9f, 0.9f, 0.9f), 6, 0.0f);

        // 雷达设备
        addDetailedCube(obj, pos + glm::vec3(0, size.y + 2.8f, 0),
            glm::vec3(0.3f, 0.2f, 0.3f), glm::vec3(0.8f, 0.3f, 0.2f), glm::vec3(0, 1, 0), 4.0f);
    }

    if (building.name.find("温室") != std::string::npos) {
        // 温室顶部框架
        for (int i = -2; i <= 2; i++) {
            addCylinder(obj, pos + glm::vec3(i * 2.0f, 0, -size.z * 0.5f),
                pos + glm::vec3(i * 2.0f, size.y, size.z * 0.5f), 0.08f,
                glm::vec3(0.7f, 0.7f, 0.7f), 6, 0.0f);
        }

        // 通风设备
        addDetailedCube(obj, pos + glm::vec3(0, size.y + 0.8f, 0),
            glm::vec3(1.0f, 0.5f, 1.0f), glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(0, 1, 0), 0.0f);
    }

    if (building.name == "水处理站") {
        // 水塔
        addCylinder(obj, pos + glm::vec3(3.0f, 0, 0),
            pos + glm::vec3(3.0f, size.y + 2.0f, 0), 1.0f,
            glm::vec3(0.5f, 0.7f, 0.9f), 10, 0.0f);

        // 管道系统
        for (int i = 0; i < 4; i++) {
            float angle = i * 90.0f * 3.14159f / 180.0f;
            glm::vec3 pipeEnd = pos + glm::vec3(cos(angle) * 6.0f, 1.0f, sin(angle) * 6.0f);
            addCylinder(obj, pos + glm::vec3(0, 1.0f, 0), pipeEnd, 0.15f,
                glm::vec3(0.4f, 0.6f, 0.8f), 6, 0.0f);
        }
    }
}

// 创建精细植物几何体（增强版）
void createDetailedPlantGeometry(RenderObject& obj, const DetailedPlant& plant) {
    glm::vec3 basePos = plant.position;

    // 主茎 - 多段式，根据植物类型调整
    int stemSegments = std::max(3, (int)(plant.height * 4));
    for (int i = 0; i < stemSegments; i++) {
        float segmentHeight = plant.height / stemSegments;
        glm::vec3 stemBottom = basePos + glm::vec3(0, i * segmentHeight * plant.growthStage, 0);
        glm::vec3 stemTop = basePos + glm::vec3(0, (i + 1) * segmentHeight * plant.growthStage, 0);

        // 茎的半径随高度逐渐变细
        float radiusRatio = 1.0f - (float)i / stemSegments * 0.3f;
        float segmentRadius = plant.stemRadius * radiusRatio;

        addCylinder(obj, stemBottom, stemTop, segmentRadius, plant.stemColor, 8, 1.0f);
    }

    // 节点 (茎上的关节点)
    int nodeCount = std::max(2, (int)(plant.leafCount * 0.4f));
    for (int i = 0; i < nodeCount; i++) {
        float heightRatio = (float)(i + 1) / nodeCount;
        glm::vec3 nodePos = basePos + glm::vec3(0, plant.height * heightRatio * plant.growthStage, 0);
        addDetailedCube(obj, nodePos, glm::vec3(plant.stemRadius * 1.5f),
            plant.stemColor * 0.8f, glm::vec3(0, 1, 0), 1.0f);
    }

    // 详细叶片系统 - 使用预计算的位置
    for (size_t i = 0; i < plant.leafPositions.size() && i < plant.leafSizes.size(); i++) {
        float heightRatio = (float)(i + 1) / plant.leafPositions.size();

        // 叶片方向计算
        float angle = i * 137.5f * 3.14159f / 180.0f; // 黄金角度螺旋
        glm::vec3 leafDirection = glm::vec3(cos(angle), 0.3f + heightRatio * 0.2f, sin(angle));

        // 根据植物类型调整叶片形状
        switch (plant.plantType) {
        case 0: // 玉米 - 长条形叶片
            addDetailedLeaf(obj, plant.leafPositions[i], leafDirection,
                plant.leafSizes[i] * 1.5f, plant.leafColor);
            break;
        case 1: // 小麦 - 细长叶片
            addDetailedLeaf(obj, plant.leafPositions[i], leafDirection,
                plant.leafSizes[i] * 0.8f, plant.leafColor);
            break;
        case 2: // 番茄 - 复合叶片
            for (int j = 0; j < 3; j++) {
                glm::vec3 subLeafPos = plant.leafPositions[i] + glm::vec3((j - 1) * plant.leafSizes[i] * 0.3f, 0, 0);
                addDetailedLeaf(obj, subLeafPos, leafDirection,
                    plant.leafSizes[i] * 0.7f, plant.leafColor);
            }
            break;
        case 3: // 菠菜 - 圆形叶片
            addDetailedCube(obj, plant.leafPositions[i],
                glm::vec3(plant.leafSizes[i], 0.02f, plant.leafSizes[i] * 0.8f),
                plant.leafColor, glm::vec3(0, 1, 0), 1.0f);
            break;
        }

        // 叶脉细节
        if (i < plant.leafPositions.size()) {
            glm::vec3 leafTip = plant.leafPositions[i] + leafDirection * plant.leafSizes[i] * 0.8f;
            addCylinder(obj, plant.leafPositions[i], leafTip, 0.003f,
                plant.leafColor * 0.7f, 4, 1.0f);
        }
    }

    // 花朵和果实系统
    if (plant.hasFlowers && plant.growthStage > 0.7f) {
        glm::vec3 flowerPos = basePos + glm::vec3(0, plant.height * 0.9f * plant.growthStage, 0);

        // 花蕊
        addDetailedCube(obj, flowerPos, glm::vec3(0.02f, 0.04f, 0.02f),
            glm::vec3(0.8f, 0.6f, 0.2f), glm::vec3(0, 1, 0), 1.0f);

        // 花瓣 - 根据植物类型
        int petalCount = (plant.plantType == 2) ? 5 : 6;
        for (int i = 0; i < petalCount; i++) {
            float angle = i * (360.0f / petalCount) * 3.14159f / 180.0f;
            glm::vec3 petalPos = flowerPos + glm::vec3(cos(angle) * 0.08f, 0.02f, sin(angle) * 0.08f);
            addDetailedLeaf(obj, petalPos, glm::vec3(cos(angle), 0.5f, sin(angle)),
                0.06f, plant.flowerColor, 1.0f);
        }
    }

    if (plant.hasFruits && plant.plantType == 2) { // 番茄果实
        int fruitCount = (int)(plant.healthFactor * 4.0f) + 1;
        for (int i = 0; i < fruitCount; i++) {
            float angle = i * 120.0f * 3.14159f / 180.0f;
            glm::vec3 fruitPos = basePos + glm::vec3(
                cos(angle) * plant.leafSize * 0.8f,
                plant.height * (0.5f + i * 0.15f) * plant.growthStage,
                sin(angle) * plant.leafSize * 0.8f
            );

            // 成熟度影响颜色
            float maturity = clamp(plant.growthStage * 1.5f - 0.5f, 0.0f, 1.0f);
            glm::vec3 fruitColor = glm::mix(glm::vec3(0.2f, 0.8f, 0.2f), plant.fruitColor, maturity);

            // 果实形状 - 椭圆形
            addDetailedCube(obj, fruitPos, glm::vec3(0.06f, 0.08f, 0.06f), fruitColor, glm::vec3(0, 1, 0), 1.0f);

            // 果实茎
            glm::vec3 stemPos = fruitPos + glm::vec3(0, 0.04f, 0);
            addCylinder(obj, fruitPos, stemPos, 0.01f, plant.stemColor, 4, 1.0f);
        }
    }

    // 根系可视化（更详细）
    if (plant.healthFactor > 0.5f) {
        int rootCount = 6;
        for (int i = 0; i < rootCount; i++) {
            float rootAngle = i * (360.0f / rootCount) * 3.14159f / 180.0f;
            float rootDistance = plant.rootSpread * plant.healthFactor;

            // 主根
            glm::vec3 rootEnd = basePos + glm::vec3(
                cos(rootAngle) * rootDistance,
                -0.15f,
                sin(rootAngle) * rootDistance
            );
            addCylinder(obj, basePos + glm::vec3(0, -0.02f, 0), rootEnd,
                plant.stemRadius * 0.4f, plant.stemColor * 0.6f, 6, 1.0f);

            // 细根
            for (int j = 0; j < 3; j++) {
                float subAngle = rootAngle + (j - 1) * 20.0f * 3.14159f / 180.0f;
                glm::vec3 subRootEnd = rootEnd + glm::vec3(
                    cos(subAngle) * rootDistance * 0.3f,
                    -0.05f,
                    sin(subAngle) * rootDistance * 0.3f
                );
                addCylinder(obj, rootEnd, subRootEnd, plant.stemRadius * 0.15f,
                    plant.stemColor * 0.4f, 4, 1.0f);
            }
        }
    }

    // 病虫害可视化
    if (plant.isPestInfected) {
        // 病变叶片（部分叶片变色）
        int affectedLeaves = std::min(3, (int)plant.leafPositions.size());
        for (int i = 0; i < affectedLeaves; i++) {
            if (i < (int)plant.leafPositions.size()) {
                addDetailedCube(obj, plant.leafPositions[i] + glm::vec3(0, 0, 0.01f),
                    glm::vec3(0.02f, 0.02f, 0.01f), glm::vec3(0.6f, 0.3f, 0.1f), glm::vec3(0, 1, 0), 1.0f);
            }
        }
    }
}

// 添加详细叶片
void addDetailedLeaf(RenderObject& obj, glm::vec3 position, glm::vec3 direction, float size, glm::vec3 color, float material) {
    direction = glm::normalize(direction);
    glm::vec3 up = glm::vec3(0, 1, 0);
    if (abs(glm::dot(direction, up)) > 0.9f) {
        up = glm::vec3(1, 0, 0);
    }
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    unsigned int baseIndex = (unsigned int)obj.vertices.size();

    // 创建更复杂的叶片形状 - 锯齿边缘
    int leafVertices = 8;
    std::vector<glm::vec3> leafPoints;

    for (int i = 0; i < leafVertices; i++) {
        float t = (float)i / (leafVertices - 1);
        float leafWidth = size * (1.0f - t * t) * 0.5f; // 叶片宽度随位置变化

        // 锯齿效果
        if (i > 0 && i < leafVertices - 1 && i % 2 == 1) {
            leafWidth *= 0.8f;
        }

        glm::vec3 centerPoint = position + direction * size * t;
        leafPoints.push_back(centerPoint - right * leafWidth);
        leafPoints.push_back(centerPoint + right * leafWidth);
    }

    glm::vec3 normal = glm::normalize(glm::cross(right, direction));

    // 添加顶点
    for (const auto& point : leafPoints) {
        obj.vertices.emplace_back(point.x, point.y, point.z,
            color.r, color.g, color.b,
            normal.x, normal.y, normal.z, material);
    }

    // 添加三角形索引
    for (int i = 0; i < leafVertices - 1; i++) {
        unsigned int i1 = baseIndex + i * 2;
        unsigned int i2 = baseIndex + i * 2 + 1;
        unsigned int i3 = baseIndex + (i + 1) * 2;
        unsigned int i4 = baseIndex + (i + 1) * 2 + 1;

        // 两个三角形构成四边形
        obj.indices.push_back(i1); obj.indices.push_back(i2); obj.indices.push_back(i3);
        obj.indices.push_back(i2); obj.indices.push_back(i4); obj.indices.push_back(i3);
    }
}

// 添加贝塞尔曲线
void addBezierCurve(RenderObject& obj, const BezierPath& path) {
    if (path.controlPoints.size() < 4) return;

    unsigned int baseIndex = (unsigned int)obj.vertices.size();

    // 生成曲线点
    for (int i = 0; i <= path.segments; i++) {
        float t = (float)i / path.segments;
        glm::vec3 point = path.calculateBezierPoint(t);

        // 计算切线方向
        float dt = 0.01f;
        glm::vec3 tangent;
        if (i == 0) {
            tangent = path.calculateBezierPoint(t + dt) - point;
        }
        else if (i == path.segments) {
            tangent = point - path.calculateBezierPoint(t - dt);
        }
        else {
            tangent = path.calculateBezierPoint(t + dt) - path.calculateBezierPoint(t - dt);
        }
        tangent = glm::normalize(tangent);

        // 计算法线
        glm::vec3 up = glm::vec3(0, 1, 0);
        glm::vec3 right = glm::normalize(glm::cross(tangent, up));

        // 创建路径宽度
        glm::vec3 leftPoint = point - right * path.pathWidth;
        glm::vec3 rightPoint = point + right * path.pathWidth;

        // 添加顶点
        obj.vertices.emplace_back(leftPoint.x, leftPoint.y, leftPoint.z,
            path.pathColor.r, path.pathColor.g, path.pathColor.b,
            0.0f, 1.0f, 0.0f, 0.0f);
        obj.vertices.emplace_back(rightPoint.x, rightPoint.y, rightPoint.z,
            path.pathColor.r, path.pathColor.g, path.pathColor.b,
            0.0f, 1.0f, 0.0f, 0.0f);

        // 添加索引
        if (i > 0) {
            unsigned int prevLeft = baseIndex + (i - 1) * 2;
            unsigned int prevRight = baseIndex + (i - 1) * 2 + 1;
            unsigned int currLeft = baseIndex + i * 2;
            unsigned int currRight = baseIndex + i * 2 + 1;

            // 路径段的两个三角形
            obj.indices.push_back(prevLeft); obj.indices.push_back(currLeft); obj.indices.push_back(prevRight);
            obj.indices.push_back(currLeft); obj.indices.push_back(currRight); obj.indices.push_back(prevRight);
        }
    }
}

// 添加详细立方体
void addDetailedCube(RenderObject& obj, glm::vec3 center, glm::vec3 size, glm::vec3 color, glm::vec3 normal, float material) {
    unsigned int baseIndex = (unsigned int)obj.vertices.size();
    glm::vec3 halfSize = size * 0.5f;

    glm::vec3 positions[8] = {
        center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z),
        center + glm::vec3(halfSize.x, -halfSize.y, -halfSize.z),
        center + glm::vec3(halfSize.x,  halfSize.y, -halfSize.z),
        center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z),
        center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z),
        center + glm::vec3(halfSize.x, -halfSize.y,  halfSize.z),
        center + glm::vec3(halfSize.x,  halfSize.y,  halfSize.z),
        center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z)
    };

    glm::vec3 normals[6] = {
        glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f,  0.0f,  1.0f),
        glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  1.0f,  0.0f)
    };

    int faceIndices[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {4, 5, 1, 0}, {3, 2, 6, 7}
    };

    for (int face = 0; face < 6; face++) {
        for (int vertex = 0; vertex < 4; vertex++) {
            glm::vec3 pos = positions[faceIndices[face][vertex]];
            obj.vertices.emplace_back(pos.x, pos.y, pos.z,
                color.r, color.g, color.b,
                normals[face].x, normals[face].y, normals[face].z, material);
        }

        unsigned int faceBase = baseIndex + face * 4;
        obj.indices.push_back(faceBase + 0); obj.indices.push_back(faceBase + 1); obj.indices.push_back(faceBase + 2);
        obj.indices.push_back(faceBase + 0); obj.indices.push_back(faceBase + 2); obj.indices.push_back(faceBase + 3);
    }
}

// 添加圆柱体
void addCylinder(RenderObject& obj, glm::vec3 bottom, glm::vec3 top, float radius, glm::vec3 color, int segments, float material) {
    unsigned int baseIndex = (unsigned int)obj.vertices.size();

    glm::vec3 direction = glm::normalize(top - bottom);
    glm::vec3 up = (abs(direction.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    // 生成圆柱体顶点
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159f;
        glm::vec3 circlePos = right * cos(angle) + up * sin(angle);
        glm::vec3 normal = glm::normalize(circlePos);

        // 底部顶点
        glm::vec3 bottomPos = bottom + circlePos * radius;
        obj.vertices.emplace_back(bottomPos.x, bottomPos.y, bottomPos.z,
            color.r, color.g, color.b,
            normal.x, normal.y, normal.z, material);

        // 顶部顶点
        glm::vec3 topPos = top + circlePos * radius;
        obj.vertices.emplace_back(topPos.x, topPos.y, topPos.z,
            color.r, color.g, color.b,
            normal.x, normal.y, normal.z, material);
    }

    // 生成圆柱体面
    for (int i = 0; i < segments; i++) {
        unsigned int current = baseIndex + i * 2;
        unsigned int next = baseIndex + ((i + 1) % (segments + 1)) * 2;

        // 侧面四边形 (两个三角形)
        obj.indices.push_back(current); obj.indices.push_back(next); obj.indices.push_back(current + 1);
        obj.indices.push_back(current + 1); obj.indices.push_back(next); obj.indices.push_back(next + 1);
    }
}

bool setupBuffers(RenderObject& obj) {
    if (obj.vertices.empty() || obj.indices.empty()) {
        return true;
    }

    if (useVAO && glGenVertexArrays != NULL) {
        glGenVertexArrays(1, &obj.VAO);
        glBindVertexArray(obj.VAO);
    }

    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);

    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER, obj.vertices.size() * sizeof(Vertex), obj.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.indices.size() * sizeof(unsigned int), obj.indices.data(), GL_STATIC_DRAW);

    // 设置顶点属性
    GLint posLoc = glGetAttribLocation(shaderProgram, "aPos");
    GLint colorLoc = glGetAttribLocation(shaderProgram, "aColor");
    GLint normalLoc = glGetAttribLocation(shaderProgram, "aNormal");
    GLint texLoc = glGetAttribLocation(shaderProgram, "aTexCoord");
    GLint materialLoc = glGetAttribLocation(shaderProgram, "aMaterialType");

    if (posLoc >= 0) {
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    }
    if (colorLoc >= 0) {
        glEnableVertexAttribArray(colorLoc);
        glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    }
    if (normalLoc >= 0) {
        glEnableVertexAttribArray(normalLoc);
        glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    }
    if (texLoc >= 0) {
        glEnableVertexAttribArray(texLoc);
        glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(9 * sizeof(float)));
    }
    if (materialLoc >= 0) {
        glEnableVertexAttribArray(materialLoc);
        glVertexAttribPointer(materialLoc, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(11 * sizeof(float)));
    }

    if (obj.VAO != 0) {
        glBindVertexArray(0);
    }

    obj.isValid = true;
    return true;
}

void render() {
    if (!isInitialized || shaderProgram == 0) return;

    // 动态天空颜色根据天气
    glm::vec3 skyColor;
    switch (weather.weatherType) {
    case 0: skyColor = glm::vec3(0.5f, 0.7f, 0.9f); break; // 晴天
    case 1: skyColor = glm::vec3(0.6f, 0.6f, 0.7f); break; // 多云
    case 2: skyColor = glm::vec3(0.4f, 0.4f, 0.5f); break; // 雨天
    case 3: skyColor = glm::vec3(0.2f, 0.2f, 0.3f); break; // 暴风雨
    }
    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    updateCamera();
    updateLighting();

    // 设置天气uniform
    GLint weatherTypeLoc = glGetUniformLocation(shaderProgram, "weatherType");
    if (weatherTypeLoc >= 0) glUniform1i(weatherTypeLoc, weather.weatherType);

    GLint cloudCoverageLoc = glGetUniformLocation(shaderProgram, "cloudCoverage");
    if (cloudCoverageLoc >= 0) glUniform1f(cloudCoverageLoc, weather.cloudCoverage);

    GLint precipitationLoc = glGetUniformLocation(shaderProgram, "precipitation");
    if (precipitationLoc >= 0) glUniform1f(precipitationLoc, weather.precipitation);

    GLint fogColorLoc = glGetUniformLocation(shaderProgram, "fogColor");
    if (fogColorLoc >= 0) glUniform3fv(fogColorLoc, 1, glm::value_ptr(weather.fogColor));

    GLint fogDensityLoc = glGetUniformLocation(shaderProgram, "fogDensity");
    if (fogDensityLoc >= 0) glUniform1f(fogDensityLoc, weather.fogDensity);

    // 设置动画uniform
    GLint timeLoc = glGetUniformLocation(shaderProgram, "time");
    if (timeLoc >= 0) glUniform1f(timeLoc, systemTime);

    GLint windDirLoc = glGetUniformLocation(shaderProgram, "windDirection");
    if (windDirLoc >= 0) glUniform2fv(windDirLoc, 1, glm::value_ptr(windDirection));

    GLint windStrLoc = glGetUniformLocation(shaderProgram, "windStrength");
    if (windStrLoc >= 0) glUniform1f(windStrLoc, windStrength);

    // 光照空间矩阵
    glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 1.0f, 50.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    GLint lightSpaceLoc = glGetUniformLocation(shaderProgram, "lightSpaceMatrix");
    if (lightSpaceLoc >= 0) glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glm::mat4 model = glm::mat4(1.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // 渲染所有对象
    for (const auto& obj : renderObjects) {
        if (!obj.isValid) continue;

        if (obj.transparent) {
            glEnable(GL_BLEND);
        }
        else {
            glDisable(GL_BLEND);
        }

        if (obj.VAO != 0) {
            glBindVertexArray(obj.VAO);
        }
        else {
            glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);

            GLint posLoc = glGetAttribLocation(shaderProgram, "aPos");
            GLint colorLoc = glGetAttribLocation(shaderProgram, "aColor");
            GLint normalLoc = glGetAttribLocation(shaderProgram, "aNormal");
            GLint materialLoc = glGetAttribLocation(shaderProgram, "aMaterialType");

            if (posLoc >= 0) {
                glEnableVertexAttribArray(posLoc);
                glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            }
            if (colorLoc >= 0) {
                glEnableVertexAttribArray(colorLoc);
                glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
            }
            if (normalLoc >= 0) {
                glEnableVertexAttribArray(normalLoc);
                glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
            }
            if (materialLoc >= 0) {
                glEnableVertexAttribArray(materialLoc);
                glVertexAttribPointer(materialLoc, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(11 * sizeof(float)));
            }
        }

        glDrawElements(GL_TRIANGLES, (GLsizei)obj.indices.size(), GL_UNSIGNED_INT, 0);
        checkOpenGLError("Draw elements");
    }

    // 恢复默认状态
    glEnable(GL_BLEND);
    if (useVAO && !renderObjects.empty() && renderObjects[0].VAO != 0) {
        glBindVertexArray(0);
    }
}

void updateLighting() {
    // 根据天气调整光照
    glm::vec3 adjustedLightColor = lightColor;
    float lightIntensity = 1.0f;

    switch (weather.weatherType) {
    case 0: // 晴天
        adjustedLightColor = glm::vec3(1.0f, 0.95f, 0.8f);
        lightIntensity = 1.2f;
        break;
    case 1: // 多云
        adjustedLightColor = glm::vec3(0.9f, 0.9f, 0.95f);
        lightIntensity = 0.8f;
        break;
    case 2: // 雨天
        adjustedLightColor = glm::vec3(0.7f, 0.8f, 0.9f);
        lightIntensity = 0.6f;
        break;
    case 3: // 暴风雨
        adjustedLightColor = glm::vec3(0.5f, 0.6f, 0.8f);
        lightIntensity = 0.4f;
        break;
    }

    // 计算动态光照方向
    float dayIntensity = clamp(lightPos.y / 20.0f, 0.0f, 1.0f);
    glm::vec3 lightDirection = glm::normalize(-lightPos);

    // 设置光照参数
    GLint lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");
    if (lightDirLoc >= 0) {
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirection));
    }

    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    if (lightColorLoc >= 0) {
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(adjustedLightColor));
    }

    GLint lightIntensityLoc = glGetUniformLocation(shaderProgram, "lightIntensity");
    if (lightIntensityLoc >= 0) {
        float intensity = lightIntensity * (0.8f + dayIntensity * 1.2f);
        glUniform1f(lightIntensityLoc, intensity);
    }

    // 设置观察位置
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    if (viewPosLoc >= 0) {
        if (freeCamera) {
            glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
        }
        else {
            float radius = 25.0f;
            float height = 12.0f + sin(systemTime * 0.08f) * 4.0f; // 缓慢上下浮动
            glm::vec3 autoCameraPos(
                cos(cameraAngle) * radius,
                height,
                sin(cameraAngle) * radius
            );
            glUniform3fv(viewPosLoc, 1, glm::value_ptr(autoCameraPos));
        }
    }
}

void updateCamera() {
    glm::mat4 view;

    if (freeCamera) {
        // 自由镜头模式
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }
    else {
        // 自动巡航模式 - 更平滑的运动
        cameraAngle += 0.005f; // 减慢旋转速度
        float radius = 25.0f;
        float height = 12.0f + sin(systemTime * 0.08f) * 4.0f; // 缓慢上下浮动

        glm::vec3 autoCameraPos(
            cos(cameraAngle) * radius,
            height,
            sin(cameraAngle) * radius
        );

        // 摄像机目标点 - 稍微偏向农场中心的上方
        glm::vec3 cameraTarget(0.0f, 3.0f, 0.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        view = glm::lookAt(autoCameraPos, cameraTarget, up);
    }

    // 获取窗口尺寸并设置投影矩阵
    int width, height_win;
    glfwGetFramebufferSize(g_window, &width, &height_win);

    if (width > 0 && height_win > 0) {
        float aspect = (float)width / (float)height_win;
        glm::mat4 projection = glm::perspective(glm::radians(50.0f), aspect, 0.1f, 200.0f);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;

        GLint mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
        if (mvpLoc >= 0) {
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        }
    }
} 