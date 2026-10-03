#include "Render/Renderer.h"
#include "Utils/Log.h"

#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

namespace SectorG {

    // --- Vertex shader (with UV) ---

    static const char* s_VertexShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aUV;

        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;

        out vec3 vFragPos;
        out vec3 vNormal;
        out vec2 vUV;

        void main() {
            vFragPos = vec3(uModel * vec4(aPos, 1.0));
            vNormal  = mat3(transpose(inverse(uModel))) * aNormal;
            vUV      = aUV;
            gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        }
    )";

    // --- Fragment shader (two lights + texture) ---

    static const char* s_FragmentShaderSrc = R"(
        #version 330 core
        in vec3 vFragPos;
        in vec3 vNormal;
        in vec2 vUV;

        out vec4 FragColor;

        uniform vec3 uLightPos1;
        uniform vec3 uLightColor1;
        uniform vec3 uLightPos2;
        uniform vec3 uLightColor2;
        uniform vec3 uViewPos;
        uniform vec3 uObjectColor;
        uniform sampler2D uTexture;

        vec3 CalculateLight(vec3 lightPos, vec3 lightColor, vec3 norm, vec3 viewDir, vec3 fragPos) {
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * lightColor;

            vec3  lightDir = normalize(lightPos - fragPos);
            float diff     = max(dot(norm, lightDir), 0.0);
            vec3  diffuse  = diff * lightColor;

            float specularStrength = 0.5;
            vec3  halfDir = normalize(lightDir + viewDir);
            float spec    = pow(max(dot(norm, halfDir), 0.0), 64);
            vec3  specular = specularStrength * spec * lightColor;

            return ambient + diffuse + specular;
        }

        void main() {
            vec3 norm    = normalize(vNormal);
            vec3 viewDir = normalize(uViewPos - vFragPos);

            vec3 lighting = CalculateLight(uLightPos1, uLightColor1, norm, viewDir, vFragPos)
                          + CalculateLight(uLightPos2, uLightColor2, norm, viewDir, vFragPos);

            vec4 texColor = texture(uTexture, vUV);

            vec3 result = lighting * texColor.rgb * uObjectColor;
            FragColor = vec4(result, texColor.a);
        }
    )";

    // --- Shadow shaders ---

    static const char* s_ShadowVertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;
        uniform vec3 uLightPos;
        uniform float uGroundY;

        out vec3 vShadowWorldPos;

        void main() {
            vec3 worldPos = vec3(uModel * vec4(aPos, 1.0));

            float t = (worldPos.y - uGroundY) / (worldPos.y - uLightPos.y);
            vec3 shadowPos = worldPos + t * (uLightPos - worldPos);
            shadowPos.y = uGroundY + 0.001;

            vShadowWorldPos = shadowPos;

            gl_Position = uProjection * uView * vec4(shadowPos, 1.0);
        }
    )";

    static const char* s_ShadowFragmentSrc = R"(
        #version 330 core
        in vec3 vShadowWorldPos;
        out vec4 FragColor;
        uniform vec3 uCubeCenter;
        void main() {
            float dist = length(vShadowWorldPos.xz - uCubeCenter.xz);
            float inner = 0.45;
            float outer = 1.1;
            float alpha = 0.5 * (1.0 - smoothstep(inner, outer, dist));
            FragColor = vec4(0.0, 0.0, 0.0, alpha);
        }
    )";

    // --- Shader helpers ---

    static unsigned int CompileShader(unsigned int type, const char* src) {
        unsigned int id = glCreateShader(type);
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (!result) {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char* message = new char[length];
            glGetShaderInfoLog(id, length, &length, message);
            Log::Error(std::string("Shader compilation failed: ") + message);
            delete[] message;
            glDeleteShader(id);
            return 0;
        }
        return id;
    }

    static unsigned int CreateShaderProgram(const char* vs, const char* fs) {
        unsigned int vsId = CompileShader(GL_VERTEX_SHADER, vs);
        unsigned int fsId = CompileShader(GL_FRAGMENT_SHADER, fs);
        if (!vsId || !fsId) return 0;

        unsigned int program = glCreateProgram();
        glAttachShader(program, vsId);
        glAttachShader(program, fsId);
        glLinkProgram(program);

        int result;
        glGetProgramiv(program, GL_LINK_STATUS, &result);
        if (!result) {
            int length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            char* message = new char[length];
            glGetProgramInfoLog(program, length, &length, message);
            Log::Error(std::string("Program link failed: ") + message);
            delete[] message;
            return 0;
        }

        glDeleteShader(vsId);
        glDeleteShader(fsId);
        return program;
    }

    // --- Texture loader ---

    static unsigned int LoadTexture(const char* path) {
        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
        if (data) {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            Log::Info(std::string("Texture loaded: ") + path);
        } else {
            Log::Error(std::string("Failed to load texture: ") + path);
        }
        stbi_image_free(data);

        return texID;
    }

    // --- Renderer ---

    Renderer::Renderer()  {}
    Renderer::~Renderer() {}

    void Renderer::Init() {
        Log::Info("Renderer initializing...");

        // --- Cube (8 float per vertex: pos + normal + uv) ---
        float cubeVerts[] = {
            // Задняя
            -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
            // Передняя
            -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
            // Левая
            -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
            // Правая
             0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
            // Нижняя
            -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
            // Верхняя
            -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
        };

        unsigned int cubeIdx[] = {
             0,  1,  2,   2,  3,  0,
             4,  5,  6,   6,  7,  4,
             8,  9, 10,  10, 11,  8,
            12, 13, 14,  14, 15, 12,
            16, 17, 18,  18, 19, 16,
            20, 21, 22,  22, 23, 20,
        };

        glGenVertexArrays(1, &m_CubeVAO);
        glBindVertexArray(m_CubeVAO);

        glGenBuffers(1, &m_CubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

        glGenBuffers(1, &m_CubeEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // --- Floor ---
        float floorVerts[] = {
            -5.0f, -0.5f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
             5.0f, -0.5f, -5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 0.0f,
             5.0f, -0.5f,  5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 5.0f,
            -5.0f, -0.5f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
        };

        unsigned int floorIdx[] = {
            0, 1, 2,
            2, 3, 0,
        };

        glGenVertexArrays(1, &m_FloorVAO);
        glBindVertexArray(m_FloorVAO);

        glGenBuffers(1, &m_FloorVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_FloorVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(floorVerts), floorVerts, GL_STATIC_DRAW);

        unsigned int floorEBO;
        glGenBuffers(1, &floorEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIdx), floorIdx, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // --- Programs ---
        m_ShaderProgram = CreateShaderProgram(s_VertexShaderSrc, s_FragmentShaderSrc);
        if (!m_ShaderProgram) {
            Log::Error("Renderer failed to create main shader");
            return;
        }

        m_ShadowProgram = CreateShaderProgram(s_ShadowVertexSrc, s_ShadowFragmentSrc);
        if (!m_ShadowProgram) {
            Log::Error("Renderer failed to create shadow shader");
            return;
        }

        m_ModelLoc = glGetUniformLocation(m_ShaderProgram, "uModel");
        m_ViewLoc  = glGetUniformLocation(m_ShaderProgram, "uView");
        m_ProjLoc  = glGetUniformLocation(m_ShaderProgram, "uProjection");

        m_SModelLoc  = glGetUniformLocation(m_ShadowProgram, "uModel");
        m_SViewLoc   = glGetUniformLocation(m_ShadowProgram, "uView");
        m_SProjLoc   = glGetUniformLocation(m_ShadowProgram, "uProjection");
        m_SLightLoc  = glGetUniformLocation(m_ShadowProgram, "uLightPos");
        m_SGroundLoc = glGetUniformLocation(m_ShadowProgram, "uGroundY");





        // --- Textures ---
        m_CubeTexture  = LoadTexture("assets/test.jpg");
        m_FloorTexture = LoadTexture("assets/test.jpg");












        glEnable(GL_DEPTH_TEST);

        Log::Info("Renderer ready");
    }

    void Renderer::BeginFrame() {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::Draw(const glm::mat4& view,
                        const glm::mat4& projection,
                        const glm::vec3& cameraPos,
                        float time)
    {
        glm::vec3 lightPos1   = glm::vec3(3.0f, 5.0f, 3.0f);
        glm::vec3 lightColor1 = glm::vec3(1.0f, 0.95f, 0.85f);

        glm::vec3 lightPos2   = glm::vec3(-4.0f, 1.5f, -3.0f);
        glm::vec3 lightColor2 = glm::vec3(0.25f, 0.35f, 0.6f);

        glm::vec3 cubeColor  = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 floorColor = glm::vec3(1.0f, 1.0f, 1.0f);

        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::rotate(cubeModel, time, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 floorModel = glm::mat4(1.0f);

        // ---------- 1. FLOOR ----------
        glUseProgram(m_ShaderProgram);
        glUniformMatrix4fv(m_ModelLoc, 1, GL_FALSE, glm::value_ptr(floorModel));
        glUniformMatrix4fv(m_ViewLoc,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(m_ProjLoc,  1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightPos1"),   1, glm::value_ptr(lightPos1));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightColor1"), 1, glm::value_ptr(lightColor1));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightPos2"),   1, glm::value_ptr(lightPos2));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightColor2"), 1, glm::value_ptr(lightColor2));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uObjectColor"), 1, glm::value_ptr(floorColor));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uViewPos"),     1, glm::value_ptr(cameraPos));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_FloorTexture);
        glUniform1i(glGetUniformLocation(m_ShaderProgram, "uTexture"), 0);

        glBindVertexArray(m_FloorVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---------- 2. SHADOW ----------
        glUseProgram(m_ShadowProgram);
        glUniformMatrix4fv(m_SModelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel));
        glUniformMatrix4fv(m_SViewLoc,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(m_SProjLoc,  1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(m_SLightLoc, 1, glm::value_ptr(lightPos1));
        glUniform1f(m_SGroundLoc, -0.5f);

        glm::vec3 cubeCenter = glm::vec3(0.0f);
        glUniform3fv(glGetUniformLocation(m_ShadowProgram, "uCubeCenter"),
                     1, glm::value_ptr(cubeCenter));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glBindVertexArray(m_CubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // ---------- 3. CUBE ----------
        glUseProgram(m_ShaderProgram);
        glUniformMatrix4fv(m_ModelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel));
        glUniformMatrix4fv(m_ViewLoc,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(m_ProjLoc,  1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightPos1"),   1, glm::value_ptr(lightPos1));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightColor1"), 1, glm::value_ptr(lightColor1));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightPos2"),   1, glm::value_ptr(lightPos2));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uLightColor2"), 1, glm::value_ptr(lightColor2));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uObjectColor"), 1, glm::value_ptr(cubeColor));
        glUniform3fv(glGetUniformLocation(m_ShaderProgram, "uViewPos"),     1, glm::value_ptr(cameraPos));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_CubeTexture);
        glUniform1i(glGetUniformLocation(m_ShaderProgram, "uTexture"), 0);

        glBindVertexArray(m_CubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

}