#include <raylib.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SPEED 15
#define MAX_OBJECTS_ON_SCREEN 8
#define INITIAL_SPEED 2
#define SPEED_INCREMENT 1
#define OBJECTS_PER_LEVEL 5
#define MAX_TOP_SCORES 100
#define MAX_NAME_LENGTH 10
#define SCORE_FILE "scores.txt"
#define MAX_PARTICLES 100

#define OBJECT_SCALE 0.05f
#define PLAYER_SCALE 0.15f

typedef enum {
    MENU,
    PLAYING,
    RANKING,
    ENTER_NAME,
    GAME_OVER,
    INSTRUCTIONS
} GameState;

typedef struct ScoreEntry {
    char name[MAX_NAME_LENGTH + 1];
    int score;
    struct ScoreEntry *next;
} ScoreEntry;

typedef struct {
    Rectangle rect;
    int speed;
} Player;

typedef struct FallingObject {
    Rectangle rect;
    int speed;
    bool active;
    int type;
    float offset;
    struct FallingObject *next;
} FallingObject;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
    bool active;
    float lifeTime;
} Particle;

int currentSpeed = INITIAL_SPEED;
int objectsCaptured = 0;
int totalObjectsCaptured = 0;
int score = 0;
bool gameOver = false;
int lives = 3;
unsigned int lastSpawnTime = 0;
const int SPAWN_INTERVAL = 1000;
GameState gameState = MENU;
int totalObjectsSpawned = 0;
int powerUpCount = 0; // Alterado para int
Particle particles[MAX_PARTICLES];
float flashTimer = 0.0f;
float totalTimePlayed = 0.0f;

ScoreEntry *topScores = NULL;
FallingObject *fallingObjects = NULL;

void LoadScores();
void SaveScores();
void UpdateTopScores(char *name, int newScore);
void SortScores(ScoreEntry **head);
ScoreEntry *Partition(ScoreEntry *head, ScoreEntry *end, ScoreEntry **newHead, ScoreEntry **newEnd);
ScoreEntry *QuickSortRecur(ScoreEntry *head, ScoreEntry *end);
ScoreEntry *GetTail(ScoreEntry *cur);
void InitParticles();
void CreateParticleEffect(Vector2 position, Color color);
void UpdateParticles(float deltaTime);
void DrawParticles();
FallingObject *createFallingObject(int totalObjectsSpawned, Texture2D objectTexture);
void updateFallingObjects(FallingObject **head, Player player, Texture2D objectTexture);
void updateSpawning(FallingObject **head, Texture2D objectTexture);
void resetGame(Player *player, FallingObject **fallingObjects, char *playerName, int *nameIndex, Texture2D playerTexture);
void FreeFallingObjects(FallingObject **head);
void FreeScores(ScoreEntry **head);

void FreeScores(ScoreEntry **head) {
    ScoreEntry *current = *head;
    while (current) {
        ScoreEntry *temp = current;
        current = current->next;
        free(temp);
    }
    *head = NULL;
}

void LoadScores() {
    FILE *file = fopen(SCORE_FILE, "r");
    if (file) {
        FreeScores(&topScores);
        topScores = NULL;
        ScoreEntry *tail = NULL;
        int count = 0;
        while (count < MAX_TOP_SCORES) {
            ScoreEntry *newEntry = (ScoreEntry *)malloc(sizeof(ScoreEntry));
            if (fscanf(file, "%10s %d", newEntry->name, &newEntry->score) == 2) {
                newEntry->next = NULL;
                if (!topScores) {
                    topScores = newEntry;
                    tail = newEntry;
                } else {
                    tail->next = newEntry;
                    tail = newEntry;
                }
            } else {
                free(newEntry);
                break;
            }
            count++;
        }
        fclose(file);
    } else {
        for (int i = 0; i < MAX_TOP_SCORES; i++) {
            ScoreEntry *newEntry = (ScoreEntry *)malloc(sizeof(ScoreEntry));
            strcpy(newEntry->name, "N/A");
            newEntry->score = 0;
            newEntry->next = topScores;
            topScores = newEntry;
        }
    }
}

void SaveScores() {
    FILE *file = fopen(SCORE_FILE, "w");
    if (file) {
        ScoreEntry *current = topScores;
        int count = 0;
        while (current && count < MAX_TOP_SCORES) {
            fprintf(file, "%s %d\n", current->name, current->score);
            current = current->next;
            count++;
        }
        fclose(file);
    }
}

ScoreEntry *Partition(ScoreEntry *head, ScoreEntry *end, ScoreEntry **newHead, ScoreEntry **newEnd) {
    ScoreEntry *pivot = end;
    ScoreEntry *prev = NULL, *cur = head, *tail = pivot;

    while (cur != pivot) {
        if (cur->score >= pivot->score) {
            if (!(*newHead))
                *newHead = cur;
            prev = cur;
            cur = cur->next;
        } else {
            if (prev)
                prev->next = cur->next;
            ScoreEntry *tmp = cur->next;
            cur->next = NULL;
            tail->next = cur;
            tail = cur;
            cur = tmp;
        }
    }

    if (!(*newHead))
        *newHead = pivot;

    *newEnd = tail;
    return pivot;
}

ScoreEntry *QuickSortRecur(ScoreEntry *head, ScoreEntry *end) {
    if (!head || head == end)
        return head;

    ScoreEntry *newHead = NULL, *newEnd = NULL;
    ScoreEntry *pivot = Partition(head, end, &newHead, &newEnd);

    if (newHead != pivot) {
        ScoreEntry *tmp = newHead;
        while (tmp->next != pivot)
            tmp = tmp->next;
        tmp->next = NULL;

        newHead = QuickSortRecur(newHead, tmp);

        tmp = GetTail(newHead);
        tmp->next = pivot;
    }

    pivot->next = QuickSortRecur(pivot->next, newEnd);
    return newHead;
}

ScoreEntry *GetTail(ScoreEntry *cur) {
    while (cur != NULL && cur->next != NULL)
        cur = cur->next;
    return cur;
}

void SortScores(ScoreEntry **headRef) {
    *headRef = QuickSortRecur(*headRef, GetTail(*headRef));
}

void UpdateTopScores(char *name, int newScore) {
    ScoreEntry *newEntry = (ScoreEntry *)malloc(sizeof(ScoreEntry));
    strncpy(newEntry->name, name, MAX_NAME_LENGTH);
    newEntry->name[MAX_NAME_LENGTH] = '\0';
    newEntry->score = newScore;
    newEntry->next = topScores;
    topScores = newEntry;

    SortScores(&topScores);

    int count = 1;
    ScoreEntry *current = topScores;
    while (current->next && count < MAX_TOP_SCORES) {
        current = current->next;
        count++;
    }

    if (current->next) {
        FreeScores(&(current->next));
        current->next = NULL;
    }
}

void InitParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
}

void CreateParticleEffect(Vector2 position, Color color) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!particles[j].active) {
                particles[j].position = position;
                particles[j].velocity = (Vector2){
                        (float)(rand() % 201 - 100) / 100.0f,
                        (float)(rand() % 201 - 100) / 100.0f
                };
                particles[j].radius = 3;
                particles[j].color = color;
                particles[j].active = true;
                particles[j].lifeTime = 1.0f;
                break;
            }
        }
    }
}

void UpdateParticles(float deltaTime) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].position.x += particles[i].velocity.x * deltaTime * 100;
            particles[i].position.y += particles[i].velocity.y * deltaTime * 100;
            particles[i].lifeTime -= deltaTime;

            if (particles[i].lifeTime <= 0) {
                particles[i].active = false;
            }
        }
    }
}

void DrawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            float size = particles[i].radius * 2;
            DrawRectangle(
                    particles[i].position.x - particles[i].radius,
                    particles[i].position.y - particles[i].radius,
                    size,
                    size,
                    particles[i].color
            );
        }
    }
}

FallingObject *createFallingObject(int totalObjectsSpawned, Texture2D objectTexture) {
    FallingObject *obj = (FallingObject *)malloc(sizeof(FallingObject));

    obj->rect.width = objectTexture.width * OBJECT_SCALE;
    obj->rect.height = objectTexture.height * OBJECT_SCALE;
    obj->rect.x = rand() % (int)(SCREEN_WIDTH - obj->rect.width);
    obj->rect.y = -obj->rect.height;

    if ((totalObjectsSpawned + 1) % 20 == 0) {
        obj->type = 2;
        obj->speed = currentSpeed;
    } else if ((totalObjectsSpawned + 1) % 11 == 0) {
        obj->type = 1;
        obj->speed = INITIAL_SPEED;
    } else {
        obj->type = 0;
        obj->speed = currentSpeed;
    }

    obj->active = true;
    obj->offset = 0;
    obj->next = NULL;
    return obj;
}

void updateFallingObjects(FallingObject **head, Player player, Texture2D objectTexture) {
    FallingObject *current = *head;
    FallingObject *prev = NULL;

    while (current != NULL) {
        if (current->type == 1) {
            current->speed = INITIAL_SPEED;
        }

        switch (current->type) {
            case 0:
                current->rect.y += current->speed;
                break;
            case 1:
                current->rect.y += current->speed;
                current->rect.x += 0.5f * sin(current->rect.y / 100.0f);
                break;
            case 2:
                current->rect.y += current->speed;
                break;
        }

        if (CheckCollisionRecs(player.rect, current->rect)) {
            if (current->type == 2) {
                powerUpCount++; // Incrementa o contador de power-ups
            } else {
                objectsCaptured++;
                totalObjectsCaptured++;
                score += 10;

                CreateParticleEffect((Vector2){
                        current->rect.x + current->rect.width / 2,
                        current->rect.y + current->rect.height / 2
                }, GREEN);
            }

            FallingObject *toRemove = current;
            if (prev) {
                prev->next = current->next;
            } else {
                *head = current->next;
            }
            current = current->next;
            free(toRemove);

            if (objectsCaptured % OBJECTS_PER_LEVEL == 0) {
                currentSpeed += SPEED_INCREMENT;
            }
        }
        else if (current->rect.y > SCREEN_HEIGHT) {
            lives--;

            CreateParticleEffect((Vector2){
                    current->rect.x + current->rect.width / 2,
                    SCREEN_HEIGHT
            }, RED);

            flashTimer = 0.2f;

            FallingObject *toRemove = current;
            if (prev) {
                prev->next = current->next;
            } else {
                *head = current->next;
            }
            current = current->next;
            free(toRemove);

            if (lives <= 0) {
                gameState = GAME_OVER;
            }
        }
        else {
            prev = current;
            current = current->next;
        }
    }
}

void updateSpawning(FallingObject **head, Texture2D objectTexture) {
    unsigned int currentTime = (unsigned int)(GetTime() * 1000);
    int activeObjects = 0;
    FallingObject *current = *head;
    while (current) {
        activeObjects++;
        current = current->next;
    }
    if ((currentTime - lastSpawnTime) >= SPAWN_INTERVAL && activeObjects < MAX_OBJECTS_ON_SCREEN) {
        FallingObject *newObject = createFallingObject(totalObjectsSpawned, objectTexture);
        newObject->next = *head;
        *head = newObject;
        lastSpawnTime = currentTime;
        totalObjectsSpawned++;
    }
}

void FreeFallingObjects(FallingObject **head) {
    FallingObject *current = *head;
    while (current) {
        FallingObject *temp = current;
        current = current->next;
        free(temp);
    }
    *head = NULL;
}

void resetGame(Player *player, FallingObject **fallingObjects, char *playerName, int *nameIndex, Texture2D playerTexture) {
    score = 0;
    lives = 3;
    currentSpeed = INITIAL_SPEED;
    objectsCaptured = 0;
    totalObjectsCaptured = 0;
    totalTimePlayed = 0.0f;
    *nameIndex = 0;
    playerName[0] = '\0';

    player->rect.width = playerTexture.width * PLAYER_SCALE;
    player->rect.height = playerTexture.height * PLAYER_SCALE;
    player->rect.x = (SCREEN_WIDTH - player->rect.width) / 2;
    player->rect.y = SCREEN_HEIGHT - player->rect.height;
    player->speed = PLAYER_SPEED;

    FreeFallingObjects(fallingObjects);
    totalObjectsSpawned = 0;
    powerUpCount = 0; // Resetar o contador de power-ups
    InitParticles();
    flashTimer = 0.0f;
}

int main() {
    srand((unsigned int)time(NULL));

    LoadScores();

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Jogo de Captura com Raylib");
    SetExitKey(0);
    SetTargetFPS(60);

    InitParticles();

    Texture2D menuBackground = LoadTexture("cmake-build-debug/marco_zero_menu.png");
    Texture2D gameBackground = LoadTexture("cmake-build-debug/marco_zero_fotoDeFundo.png");
    Texture2D playerTexture = LoadTexture("cmake-build-debug/turista.png");
    Texture2D objectTexture = LoadTexture("cmake-build-debug/object1.png");

    if (!menuBackground.id || !gameBackground.id || !playerTexture.id || !objectTexture.id) {
        printf("Falha ao carregar uma ou mais texturas.\n");
        CloseWindow();
        return 1;
    }

    Player player;
    player.rect.width = playerTexture.width * PLAYER_SCALE;
    player.rect.height = playerTexture.height * PLAYER_SCALE;
    player.rect.x = (SCREEN_WIDTH - player.rect.width) / 2;
    player.rect.y = SCREEN_HEIGHT - player.rect.height;
    player.speed = PLAYER_SPEED;

    char playerName[MAX_NAME_LENGTH + 1] = "";
    int nameIndex = 0;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        switch (gameState) {
            case MENU:
                if (IsKeyPressed(KEY_ONE)) {
                    resetGame(&player, &fallingObjects, playerName, &nameIndex, playerTexture);
                    gameState = PLAYING;
                } else if (IsKeyPressed(KEY_TWO)) {
                    gameState = INSTRUCTIONS;
                } else if (IsKeyPressed(KEY_THREE)) {
                    gameState = RANKING;
                } else if (IsKeyPressed(KEY_FOUR)) {
                    CloseWindow();
                    return 0;
                }
                break;

            case INSTRUCTIONS:
                if (IsKeyPressed(KEY_R)) {
                    gameState = MENU;
                }
                break;

            case PLAYING:
                if (IsKeyDown(KEY_LEFT)) player.rect.x -= player.speed;
                if (IsKeyDown(KEY_RIGHT)) player.rect.x += player.speed;

                if (player.rect.x < 0)
                    player.rect.x = 0;
                else if (player.rect.x + player.rect.width > SCREEN_WIDTH)
                    player.rect.x = SCREEN_WIDTH - player.rect.width;

                updateFallingObjects(&fallingObjects, player, objectTexture);
                updateSpawning(&fallingObjects, objectTexture);

                UpdateParticles(deltaTime);

                totalTimePlayed += deltaTime;

                if (flashTimer > 0.0f) {
                    flashTimer -= deltaTime;
                    if (flashTimer < 0.0f) {
                        flashTimer = 0.0f;
                    }
                }

                if (IsKeyPressed(KEY_SPACE) && powerUpCount > 0) {
                    FallingObject *current = fallingObjects;
                    while (current) {
                        CreateParticleEffect((Vector2){
                                current->rect.x + current->rect.width / 2,
                                current->rect.y + current->rect.height / 2
                        }, ORANGE);
                        current = current->next;
                    }
                    FreeFallingObjects(&fallingObjects);
                    powerUpCount--; // Decrementa o contador de power-ups
                }

                break;

            case RANKING:
                if (IsKeyPressed(KEY_R)) {
                    gameState = MENU;
                }
                break;

            case ENTER_NAME:
            {
                int key = GetCharPressed();
                if ((key >= 32) && (key <= 125) && nameIndex < MAX_NAME_LENGTH) {
                    playerName[nameIndex++] = (char)key;
                    playerName[nameIndex] = '\0';
                }
                if (IsKeyPressed(KEY_BACKSPACE) && nameIndex > 0) {
                    playerName[--nameIndex] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER) && nameIndex > 0) {
                    UpdateTopScores(playerName, score);
                    SaveScores();
                    gameState = RANKING;
                }
            }
                break;

            case GAME_OVER:
                if (IsKeyPressed(KEY_ENTER)) {
                    if (score > 0) {
                        gameState = ENTER_NAME;
                    } else {
                        gameState = MENU;
                        resetGame(&player, &fallingObjects, playerName, &nameIndex, playerTexture);
                    }
                }
                break;
        }

        BeginDrawing();

        switch (gameState) {
            case MENU:
                ClearBackground(RAYWHITE);
                DrawTexture(menuBackground, 0, 0, WHITE);
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(WHITE, 0.4f));

                DrawText("Bem-vindo ao Jogo!", SCREEN_WIDTH / 2 - MeasureText("Bem-vindo ao Jogo!", 40) / 2, 100, 40, BLACK);
                DrawText("1. Iniciar Jogo", SCREEN_WIDTH / 2 - MeasureText("1. Iniciar Jogo", 20) / 2, 200, 20, BLACK);
                DrawText("2. Instruções", SCREEN_WIDTH / 2 - MeasureText("2. Instruções", 20) / 2, 250, 20, BLACK);
                DrawText("3. Ranking de Pontuações", SCREEN_WIDTH / 2 - MeasureText("3. Ranking de Pontuações", 20) / 2, 300, 20, BLACK);
                DrawText("4. Sair do Jogo", SCREEN_WIDTH / 2 - MeasureText("4. Sair do Jogo", 20) / 2, 350, 20, BLACK);
                DrawText("Escolha uma opção com o número correspondente", SCREEN_WIDTH / 2 - MeasureText("Escolha uma opção com o número correspondente", 20) / 2, 450, 20, BLACK);
                break;

            case INSTRUCTIONS:
                ClearBackground(RAYWHITE);

                DrawText("Instruções do Jogo", SCREEN_WIDTH / 2 - MeasureText("Instruções do Jogo", 30) / 2, 50, 30, BLACK);
                DrawText("Objetivo:", 100, 120, 20, DARKGRAY);
                DrawText("- Capturar os objetos que caem para ganhar pontos.", 120, 150, 18, GRAY);
                DrawText("- Evite deixar os objetos caírem no chão, você perderá vidas.", 120, 180, 18, GRAY);
                DrawText("Controles:", 100, 230, 20, DARKGRAY);
                DrawText("- Use as setas ESQUERDA e DIREITA para mover o jogador.", 120, 260, 18, GRAY);
                DrawText("- Pressione ESPAÇO para usar o Power-up quando disponível.", 120, 290, 18, GRAY);
                DrawText("Power-ups:", 100, 340, 20, DARKGRAY);
                DrawText("- Capture o objeto especial para ganhar um Power-up.", 120, 370, 18, GRAY);
                DrawText("- O Power-up elimina todos os objetos na tela quando usado.", 120, 400, 18, GRAY);
                DrawText("Pressione 'R' para retornar ao menu principal.", SCREEN_WIDTH / 2 - MeasureText("Pressione 'R' para retornar ao menu principal.", 18) / 2, SCREEN_HEIGHT - 50, 18, DARKGRAY);
                break;

           case PLAYING:
    ClearBackground(RAYWHITE);
    DrawTexture(gameBackground, 0, 0, WHITE);

    if (flashTimer > 0.0f) {
        float flashIntensity = flashTimer / 0.2f;
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(RED, flashIntensity * 0.5f));
    }

    DrawTextureEx(playerTexture, (Vector2){ player.rect.x, player.rect.y }, 0.0f, PLAYER_SCALE, WHITE);

    FallingObject *current = fallingObjects;
    while (current) {
        if (current->active) {
            if (current->type != 2) {
                DrawTextureEx(objectTexture, (Vector2){ current->rect.x, current->rect.y }, 0.0f, OBJECT_SCALE, WHITE);
            } else {
                Color objectColor = ((int)(GetTime() * 10) % 2 == 0) ? YELLOW : ORANGE;
                DrawRectangleRec(current->rect, objectColor);
            }
        }
        current = current->next;
    }

    DrawParticles();

    // Cálculo das larguras dos textos para centralização
    int livesTextWidth = MeasureText(TextFormat("Lives: %d", lives), 20);
    int livesTextX = (SCREEN_WIDTH - livesTextWidth) / 2;

    int powerUpTextWidth = MeasureText(TextFormat("Power-ups: %d", powerUpCount), 20);
    int powerUpTextX = (SCREEN_WIDTH - powerUpTextWidth) / 2;

    int scoreTextWidth = MeasureText(TextFormat("Score: %d", score), 20);
    int scoreTextX = 10; // Mantém a posição X do "Score"

    // Desenhando os textos na tela
    DrawText(TextFormat("Lives: %d", lives), livesTextX, 10, 20, RED);

    int powerUpTextY = 10 + 20 + 5; // 10 (posição Y do "Lives") + 20 (tamanho da fonte) + 5 (espaçamento)
    DrawText(TextFormat("Power-ups: %d", powerUpCount), powerUpTextX, powerUpTextY, 20, BLUE);

    DrawText(TextFormat("Score: %d", score), scoreTextX, 10, 20, GREEN);
    break;


            case RANKING:
                ClearBackground(BLACK);

                DrawText("TOP 10 Scores:", SCREEN_WIDTH / 2 - MeasureText("TOP 10 Scores:", 30) / 2, 50, 30, YELLOW);

                {
                    ScoreEntry *current = topScores;
                    int i = 0;
                    while (current && i < 10) {
                        char rankingText[50];
                        sprintf(rankingText, "%d. %s - %d", i + 1, current->name, current->score);

                        int textWidth = MeasureText(rankingText, 20);
                        int posX = SCREEN_WIDTH / 2 - textWidth / 2;
                        int posY = 100 + (i * 30);

                        DrawText(rankingText, posX, posY, 20, WHITE);

                        current = current->next;
                        i++;
                    }
                }

                DrawText("Pressione R para voltar ao menu", SCREEN_WIDTH / 2 - MeasureText("Pressione R para voltar ao menu", 20) / 2,
                         SCREEN_HEIGHT - 50, 20, GRAY);
                break;

            case ENTER_NAME:
                ClearBackground(BLACK);

                DrawText("Digite seu Nome:", SCREEN_WIDTH / 2 - MeasureText("Digite seu Nome:", 20) / 2, SCREEN_HEIGHT / 2 - 80, 20, YELLOW);
                DrawText(playerName, SCREEN_WIDTH / 2 - MeasureText(playerName, 20) / 2, SCREEN_HEIGHT / 2 - 50, 20, WHITE);
                DrawText("Pressione ENTER para confirmar", SCREEN_WIDTH / 2 - MeasureText("Pressione ENTER para confirmar", 20) / 2, SCREEN_HEIGHT / 2 - 10, 20, GRAY);

                if (((int)(GetTime() * 2) % 2) == 0) {
                    DrawText("_", SCREEN_WIDTH / 2 + MeasureText(playerName, 20) / 2, SCREEN_HEIGHT / 2 - 50, 20, WHITE);
                }
                break;

            case GAME_OVER:
                ClearBackground(BLACK);

                DrawText("Game Over!", SCREEN_WIDTH / 2 - MeasureText("Game Over!", 40) / 2, SCREEN_HEIGHT / 2 - 150, 40, RED);

                char scoreText[50];
                sprintf(scoreText, "Score: %d", score);
                DrawText(scoreText, SCREEN_WIDTH / 2 - MeasureText(scoreText, 20) / 2, SCREEN_HEIGHT / 2 - 100, 20, WHITE);

                char timeText[50];
                sprintf(timeText, "Tempo Jogando: %.2f segundos", totalTimePlayed);
                DrawText(timeText, SCREEN_WIDTH / 2 - MeasureText(timeText, 20) / 2, SCREEN_HEIGHT / 2 - 70, 20, WHITE);

                char objectsText[50];
                sprintf(objectsText, "Objetos Capturados: %d", totalObjectsCaptured);
                DrawText(objectsText, SCREEN_WIDTH / 2 - MeasureText(objectsText, 20) / 2, SCREEN_HEIGHT / 2 - 40, 20, WHITE);

                DrawText("Pressione ENTER para continuar", SCREEN_WIDTH / 2 - MeasureText("Pressione ENTER para continuar", 20) / 2, SCREEN_HEIGHT / 2 + 20, 20, YELLOW);
                break;
        }

        EndDrawing();
    }

    UnloadTexture(menuBackground);
    UnloadTexture(gameBackground);
    UnloadTexture(playerTexture);
    UnloadTexture(objectTexture);
    FreeFallingObjects(&fallingObjects);
    FreeScores(&topScores);
    CloseWindow();
    return 0;
}
