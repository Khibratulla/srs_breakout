// Breakout (Arkanoid) — 5-аптадағы құрылымдармен: GLFW + GLAD, шейдер, VAO/VBO/EBO, uniform, dt
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

// ---------- Шейдерлер ----------
const char* vertexSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform vec2 uPos;    // центр (NDC)
uniform vec2 uSize;   // толық ені/биіктігі (NDC)
void main() {
    gl_Position = vec4(aPos.xy * uSize + uPos, 0.0, 1.0);
}
)";

const char* fragmentSrc = R"(
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

// ---------- Параметрлер (барлығы NDC, [-1, 1]) ----------
const int   COLS = 8, ROWS = 5;
const float BRICK_HW = 0.1f, BRICK_HH = 0.04f;     // жартылай өлшем
const float BRICK_GAP_X = 0.025f, BRICK_GAP_Y = 0.025f;
const float BRICKS_TOP = 0.85f;

const float PADDLE_Y = -0.85f, PADDLE_HW = 0.15f, PADDLE_HH = 0.025f;
const float PADDLE_SPEED = 1.6f;
const float BALL_R = 0.025f;
const float BALL_START_SPEED = 0.9f, BALL_MAX_SPEED = 1.8f;

const float ROW_COLORS[ROWS][3] = {
    {0.90f, 0.20f, 0.20f}, {0.95f, 0.55f, 0.15f}, {0.95f, 0.85f, 0.20f},
    {0.30f, 0.80f, 0.35f}, {0.30f, 0.60f, 0.95f}
};

// ---------- Күй ----------
bool  bricks[ROWS][COLS];
float paddleX = 0.0f;
float ballX, ballY, ballVX, ballVY;
bool  ballLaunched = false;
int   score = 0, lives = 3, bricksLeft = 0;
bool  gameOver = false, won = false;
bool  titleDirty = true;

float brickX(int c) { return (c - (COLS - 1) * 0.5f) * (2 * BRICK_HW + BRICK_GAP_X); }
float brickY(int r) { return BRICKS_TOP - r * (2 * BRICK_HH + BRICK_GAP_Y); }

void resetBall() {
    ballLaunched = false;
    ballVX = ballVY = 0.0f;
}

void resetGame() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) bricks[r][c] = true;
    bricksLeft = ROWS * COLS;
    score = 0; lives = 3; gameOver = won = false;
    paddleX = 0.0f;
    resetBall();
    titleDirty = true;
}

void launchBall() {
    ballLaunched = true;
    ballVX = 0.35f * BALL_START_SPEED;
    ballVY = std::sqrt(BALL_START_SPEED * BALL_START_SPEED - ballVX * ballVX);
}

void keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_SPACE) {
        if (gameOver || won) resetGame();
        else if (!ballLaunched) launchBall();
    }
    if (key == GLFW_KEY_R) resetGame();
}

unsigned int compileShader(GLenum type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return shader;
}

unsigned int createProgram(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// Шар (центр+радиус) мен төртбұрыш қиылысуы; қиылысса — ену тереңдігі бойынша қай осьте соғылғанын қайтарады
bool ballHitsRect(float rx, float ry, float hw, float hh, bool& hitX) {
    float dx = ballX - rx, dy = ballY - ry;
    float px = (hw + BALL_R) - std::fabs(dx);
    float py = (hh + BALL_R) - std::fabs(dy);
    if (px <= 0 || py <= 0) return false;
    hitX = px < py;     // аз енген ось — соқтығысу осі
    return true;
}

void updateTitle(GLFWwindow* window) {
    std::string t = "Breakout | Score: " + std::to_string(score) +
                    " | Lives: " + std::to_string(lives);
    if (won)               t += " | YOU WIN! (Space)";
    else if (gameOver)     t += " | GAME OVER (Space)";
    else if (!ballLaunched) t += " | Space = launch";
    glfwSetWindowTitle(window, t.c_str());
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // квадрат терезе: NDC бұрмаланбайды

    GLFWwindow* window = glfwCreateWindow(800, 800, "Breakout", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, keyCallback);

    // ---------- Бірлік төртбұрыш (EBO): барлық нысан осыдан масштабталады ----------
    float quadVertices[] = {
         0.5f,  0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    unsigned int quadIndices[] = { 0, 3, 1,   1, 3, 2 };

    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    unsigned int shader = createProgram(vertexSrc, fragmentSrc);
    int locPos   = glGetUniformLocation(shader, "uPos");
    int locSize  = glGetUniformLocation(shader, "uSize");
    int locColor = glGetUniformLocation(shader, "uColor");

    auto drawRect = [&](float x, float y, float hw, float hh, float r, float g, float b) {
        glUniform2f(locPos, x, y);
        glUniform2f(locSize, hw * 2.0f, hh * 2.0f);
        glUniform3f(locColor, r, g, b);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    };

    resetGame();
    float lastFrame = (float)glfwGetTime();
    std::cout << "A/D or Left/Right = move, Space = launch, R = restart, Esc = quit\n";

    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt  = std::min(now - lastFrame, 0.03f);
        lastFrame = now;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // ---------- Басқару ----------
        if (!gameOver && !won) {
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                paddleX -= PADDLE_SPEED * dt;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                paddleX += PADDLE_SPEED * dt;
            paddleX = std::clamp(paddleX, -1.0f + PADDLE_HW, 1.0f - PADDLE_HW);

            if (!ballLaunched) {            // шар тақтаның үстінде тұрады
                ballX = paddleX;
                ballY = PADDLE_Y + PADDLE_HH + BALL_R;
            } else {
                ballX += ballVX * dt;
                ballY += ballVY * dt;

                // Қабырғалар мен төбе
                if (ballX < -1.0f + BALL_R) { ballX = -1.0f + BALL_R; ballVX = std::fabs(ballVX); }
                if (ballX >  1.0f - BALL_R) { ballX =  1.0f - BALL_R; ballVX = -std::fabs(ballVX); }
                if (ballY >  1.0f - BALL_R) { ballY =  1.0f - BALL_R; ballVY = -std::fabs(ballVY); }

                // Тақта: қайту бұрышы тиген орнына байланысты
                bool hx;
                if (ballVY < 0 && ballHitsRect(paddleX, PADDLE_Y, PADDLE_HW, PADDLE_HH, hx)) {
                    float t = std::clamp((ballX - paddleX) / PADDLE_HW, -1.0f, 1.0f);   // [-1, 1]
                    float speed = std::min(std::sqrt(ballVX * ballVX + ballVY * ballVY) * 1.03f, BALL_MAX_SPEED);
                    float ang = t * 1.0f;                       // макс ~57° тігінен
                    ballVX = speed * std::sin(ang);
                    ballVY = speed * std::cos(ang);
                    ballY = PADDLE_Y + PADDLE_HH + BALL_R;
                }

                // Кірпіштер: кадрға бір кірпіш жеткілікті
                for (int r = 0; r < ROWS; r++) {
                    for (int c = 0; c < COLS; c++) {
                        if (!bricks[r][c]) continue;
                        if (ballHitsRect(brickX(c), brickY(r), BRICK_HW, BRICK_HH, hx)) {
                            bricks[r][c] = false;
                            bricksLeft--;
                            score += (ROWS - r) * 10;
                            if (hx) ballVX = -ballVX; else ballVY = -ballVY;
                            titleDirty = true;
                            goto brickDone;
                        }
                    }
                }
                brickDone:

                if (bricksLeft == 0) { won = true; titleDirty = true; }

                // Шар төменге түсті
                if (ballY < -1.0f - BALL_R) {
                    lives--;
                    if (lives <= 0) gameOver = true;
                    else resetBall();
                    titleDirty = true;
                }
            }
        }

        if (titleDirty) { updateTitle(window); titleDirty = false; }

        // ---------- Сызу ----------
        glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);
        glBindVertexArray(vao);          // бір bind — барлық нысан

        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (bricks[r][c])
                    drawRect(brickX(c), brickY(r), BRICK_HW, BRICK_HH,
                             ROW_COLORS[r][0], ROW_COLORS[r][1], ROW_COLORS[r][2]);

        drawRect(paddleX, PADDLE_Y, PADDLE_HW, PADDLE_HH, 0.85f, 0.88f, 0.95f);
        drawRect(ballX, ballY, BALL_R, BALL_R, 1.0f, 1.0f, 1.0f);

        // Өмірлер: сол жақ төменгі бұрышта кішкентай шаршылар
        for (int i = 0; i < lives; i++)
            drawRect(-0.94f + i * 0.06f, -0.96f, 0.018f, 0.018f, 0.95f, 0.3f, 0.35f);

        // Ұтыс/ұтылыс: экран ортасында түсті жолақ
        if (won)      drawRect(0.0f, 0.0f, 0.6f, 0.05f, 0.3f, 0.85f, 0.4f);
        if (gameOver) drawRect(0.0f, 0.0f, 0.6f, 0.05f, 0.9f, 0.2f, 0.2f);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader);
    glfwTerminate();
}
