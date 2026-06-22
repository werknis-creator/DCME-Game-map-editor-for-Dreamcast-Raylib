#include <kos.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dc/pvr.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

#define PLAYER_SPEED 5.0f
#define TURN_SPEED 3.0f
#define CAR_MAX_SPEED 15.0f
#define CAR_ACCEL 5.0f
#define CAR_BRAKE 8.0f
#define CAR_FRICTION 2.0f
#define CAR_ENTER_DIST 3.5f
#define JUMP_FORCE 8.0f
#define GRAVITY 20.0f
#define MAX_OBJECTS 512
#define MAX_MAPS 10
#define MAX_DRAW_DISTANCE 40.0f

typedef struct {
    int type;
    Vector3 position;
    float scale;
    float rotation;
} MapObject;

static MapObject* map_objects = NULL;
static int map_object_count = 0;
static int current_map = 1;
static bool pickedUp[MAX_OBJECTS];

extern MapObject map_objects_1[] __attribute__((weak));
extern int map_object_count_1 __attribute__((weak));
extern const char* map_settings_1 __attribute__((weak));
extern MapObject map_objects_2[] __attribute__((weak));
extern int map_object_count_2 __attribute__((weak));
extern const char* map_settings_2 __attribute__((weak));
extern MapObject map_objects_3[] __attribute__((weak));
extern int map_object_count_3 __attribute__((weak));
extern const char* map_settings_3 __attribute__((weak));
extern MapObject map_objects_4[] __attribute__((weak));
extern int map_object_count_4 __attribute__((weak));
extern const char* map_settings_4 __attribute__((weak));
extern MapObject map_objects_5[] __attribute__((weak));
extern int map_object_count_5 __attribute__((weak));
extern const char* map_settings_5 __attribute__((weak));
extern MapObject map_objects_6[] __attribute__((weak));
extern int map_object_count_6 __attribute__((weak));
extern const char* map_settings_6 __attribute__((weak));
extern MapObject map_objects_7[] __attribute__((weak));
extern int map_object_count_7 __attribute__((weak));
extern const char* map_settings_7 __attribute__((weak));
extern MapObject map_objects_8[] __attribute__((weak));
extern int map_object_count_8 __attribute__((weak));
extern const char* map_settings_8 __attribute__((weak));
extern MapObject map_objects_9[] __attribute__((weak));
extern int map_object_count_9 __attribute__((weak));
extern const char* map_settings_9 __attribute__((weak));
extern MapObject map_objects_10[] __attribute__((weak));
extern int map_object_count_10 __attribute__((weak));
extern const char* map_settings_10 __attribute__((weak));

static MapObject* map_ptrs[MAX_MAPS+1];
static int map_counts[MAX_MAPS+1];
static const char* map_settings[MAX_MAPS+1];

static Color bgColor = SKYBLUE;
static Color dayColor = SKYBLUE;

bool cmd_benchACT = false;
bool cmd_fightACT = false;
bool cmd_hpACT = false;
bool cmd_sprintACT = false;
bool cmd_FPC = false;
bool cmd_FOG = false;
bool cmd_colACT = false;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float width;
    float height;
    Color color;
    bool inCar;
    int currentCar;
    bool isHuman;
    bool isJumping;
    int hp;
} Player;

typedef struct {
    float speed;
    float wheelRotation;
} CarState;

static Player player;
static CarState carStates[MAX_OBJECTS];
static Camera3D camera = {0};
static float walkPhase = 0.0f;
static float walkAmount = 0.0f;
static float globalTime = 0.0f; 

// Dodatkowa zmienna do kontroli pochylenia pionowego kamery przez D-Pad
static float cameraPitchOffset = 0.0f;

typedef struct {
    bool isDead;
    float headAngle;
} NpcState;

static NpcState npcStates[MAX_OBJECTS];

static Color ParseColorName(const char* name) {
    if (!name || !*name) return SKYBLUE;
    if (strcmp(name, "black") == 0) return BLACK;
    if (strcmp(name, "white") == 0) return WHITE;
    if (strcmp(name, "red") == 0) return RED;
    if (strcmp(name, "green") == 0) return GREEN;
    if (strcmp(name, "blue") == 0) return BLUE;
    if (strcmp(name, "yellow") == 0) return YELLOW;
    if (strcmp(name, "gray") == 0) return GRAY;
    if (strcmp(name, "darkgray") == 0) return DARKGRAY;
    return SKYBLUE;
}

static void ParseSettings(const char* settings) {
    if (!settings || !*settings) return;
    char buf[256];
    strncpy(buf, settings, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    char* token = strtok(buf, "\n");
    while (token) {
        if (strncmp(token, "background", 10) == 0) {
            char* paren = strchr(token, '(');
            if (paren) {
                char* endparen = strchr(paren, ')');
                if (endparen) {
                    *endparen = '\0';
                    bgColor = ParseColorName(paren + 1);
                    dayColor = bgColor;
                }
            }
        }
        token = strtok(NULL, "\n");
    }
}

static void InitMapTables(void) {
    for (int i = 0; i <= MAX_MAPS; i++) { map_ptrs[i] = NULL; map_counts[i] = 0; map_settings[i] = NULL; }
    if (&map_object_count_1)  { map_ptrs[1]  = map_objects_1;  map_counts[1]  = map_object_count_1;  map_settings[1]  = map_settings_1;  }
    if (&map_object_count_2)  { map_ptrs[2]  = map_objects_2;  map_counts[2]  = map_object_count_2;  map_settings[2]  = map_settings_2;  }
    if (&map_object_count_3)  { map_ptrs[3]  = map_objects_3;  map_counts[3]  = map_object_count_3;  map_settings[3]  = map_settings_3;  }
    if (&map_object_count_4)  { map_ptrs[4]  = map_objects_4;  map_counts[4]  = map_object_count_4;  map_settings[4]  = map_settings_4;  }
    if (&map_object_count_5)  { map_ptrs[5]  = map_objects_5;  map_counts[5]  = map_object_count_5;  map_settings[5]  = map_settings_5;  }
    if (&map_object_count_6)  { map_ptrs[6]  = map_objects_6;  map_counts[6]  = map_object_count_6;  map_settings[6]  = map_settings_6;  }
    if (&map_object_count_7)  { map_ptrs[7]  = map_objects_7;  map_counts[7]  = map_object_count_7;  map_settings[7]  = map_settings_7;  }
    if (&map_object_count_8)  { map_ptrs[8]  = map_objects_8;  map_counts[8]  = map_object_count_8;  map_settings[8]  = map_settings_8;  }
    if (&map_object_count_9)  { map_ptrs[9]  = map_objects_9;  map_counts[9]  = map_object_count_9;  map_settings[9]  = map_settings_9;  }
    if (&map_object_count_10) { map_ptrs[10] = map_objects_10; map_counts[10] = map_object_count_10; map_settings[10] = map_settings_10; }
}

static void DrawHardwareCube(Vector3 position, float width, float height, float length, Color color) {
    float w = width/2.0f; float h = height/2.0f; float l = length/2.0f;
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-w, -h,  l);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f( w, -h,  l);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f( w,  h,  l);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-w,  h,  l);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(-w, -h, -l);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(-w,  h, -l);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f( w,  h, -l);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f( w, -h, -l);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-w,  h, -l);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-w,  h,  l);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f( w,  h,  l);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f( w,  h, -l);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(-w, -h, -l);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f( w, -h, -l);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f( w, -h,  l);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(-w, -h,  l);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f( w, -h, -l);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f( w,  h, -l);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f( w,  h,  l);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f( w, -h,  l);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-w, -h, -l);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(-w, -h,  l);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(-w,  h,  l);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-w,  h, -l);
    rlEnd();
    rlPopMatrix();
}

static void DrawCubePro(Vector3 position, Vector3 size, Vector3 rotation, Color color) {
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    if (rotation.z != 0.0f) rlRotatef(rotation.z, 0.0f, 0.0f, 1.0f);
    if (rotation.y != 0.0f) rlRotatef(rotation.y, 0.0f, 1.0f, 0.0f);
    if (rotation.x != 0.0f) rlRotatef(rotation.x, 1.0f, 0.0f, 0.0f);
    DrawHardwareCube((Vector3){0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);
    rlPopMatrix();
}

static void DrawRotatedCube(Vector3 pos, float sx, float sy, float sz, float rotationDeg, Color color) {
    DrawCubePro(pos, (Vector3){sx, sy, sz}, (Vector3){0.0f, rotationDeg, 0.0f}, color);
}

static void DrawCarObj(MapObject* obj) {
    float s = obj->scale; Vector3 p = obj->position; float r = obj->rotation;
    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(r, 0.0f, 1.0f, 0.0f);
    
    // Karoseria dolna
    DrawHardwareCube((Vector3){0.0f, 0.5f*s, 0.0f}, 1.8f*s, 0.5f*s, 3.6f*s, RED);
    // Kabina
    DrawHardwareCube((Vector3){0.0f, 1.0f*s, -0.2f*s}, 1.6f*s, 0.6f*s, 1.8f*s, DARKGRAY);
    // Szyby
    DrawHardwareCube((Vector3){0.0f, 1.0f*s, 0.71f*s}, 1.4f*s, 0.4f*s, 0.02f*s, LIGHTGRAY); 
    DrawHardwareCube((Vector3){0.0f, 1.0f*s, -1.11f*s}, 1.4f*s, 0.4f*s, 0.02f*s, LIGHTGRAY); 

    // Koła
    float wheelY = 0.2f * s; float wheelW = 0.3f * s; float wheelH = 0.4f * s; float wheelL = 0.5f * s;
    DrawHardwareCube((Vector3){-0.95f*s, wheelY,  1.1f*s}, wheelW, wheelH, wheelL, BLACK); 
    DrawHardwareCube((Vector3){ 0.95f*s, wheelY,  1.1f*s}, wheelW, wheelH, wheelL, BLACK); 
    DrawHardwareCube((Vector3){-0.95f*s, wheelY, -1.1f*s}, wheelW, wheelH, wheelL, BLACK); 
    DrawHardwareCube((Vector3){ 0.95f*s, wheelY, -1.1f*s}, wheelW, wheelH, wheelL, BLACK); 
    
    // Reflektory
    DrawHardwareCube((Vector3){-0.55f*s, 0.5f*s, 1.81f*s}, 0.25f*s, 0.15f*s, 0.02f*s, YELLOW);
    DrawHardwareCube((Vector3){ 0.55f*s, 0.5f*s, 1.81f*s}, 0.25f*s, 0.15f*s, 0.02f*s, YELLOW);
    DrawHardwareCube((Vector3){-0.55f*s, 0.5f*s, -1.81f*s}, 0.25f*s, 0.15f*s, 0.02f*s, MAROON);
    DrawHardwareCube((Vector3){ 0.55f*s, 0.5f*s, -1.81f*s}, 0.25f*s, 0.15f*s, 0.02f*s, MAROON);
    rlPopMatrix();
}

static void DrawHuman(Vector3 pos, float yaw, float scale, Color shirtColor, float swing, float headYawOffset) {
    float s = scale;
    Color skin  = (Color){ 235, 190, 150, 255 };
    Color pants = (Color){ 40, 60, 130, 255 };
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(yaw * RAD2DEG, 0.0f, 1.0f, 0.0f);
    
    DrawHardwareCube((Vector3){-0.18f*s, 0.4f*s,  swing*s}, 0.22f*s, 0.8f*s,  0.22f*s, pants);
    DrawHardwareCube((Vector3){ 0.18f*s, 0.4f*s, -swing*s}, 0.22f*s, 0.8f*s,  0.22f*s, pants);
    DrawHardwareCube((Vector3){0.0f, 1.15f*s, 0.0f}, 0.62f*s, 0.75f*s, 0.38f*s, shirtColor);
    
    rlPushMatrix();
    rlTranslatef(0.0f, 1.65f*s, 0.0f);
    rlRotatef(headYawOffset * RAD2DEG, 0.0f, 1.0f, 0.0f);
    DrawHardwareCube((Vector3){0.0f, 0.0f, 0.0f}, 0.36f*s,  0.36f*s,  0.36f*s,  skin);
    rlPopMatrix();
    
    DrawHardwareCube((Vector3){-0.42f*s, 1.15f*s, -swing*s}, 0.16f*s, 0.65f*s, 0.16f*s, shirtColor);
    DrawHardwareCube((Vector3){-0.42f*s, 0.75f*s, -swing*s}, 0.13f*s, 0.15f*s, 0.13f*s, skin);
    DrawHardwareCube((Vector3){ 0.42f*s, 1.15f*s,  swing*s}, 0.16f*s, 0.65f*s, 0.16f*s, shirtColor);
    DrawHardwareCube((Vector3){ 0.42f*s, 0.75f*s,  swing*s}, 0.13f*s, 0.15f*s, 0.13f*s, skin);
    rlPopMatrix();
}

/* ODWZOROWANIE 1:1 GEOMETRII I WYMIARÓW Z EDYTORA MAP (DCME) */
static void DrawMapObject(MapObject* obj) {
    int idx = (int)(obj - map_objects);
    float s = obj->scale; Vector3 p = obj->position;
    float t = globalTime;

    switch (obj->type) {
        case 0: // Mały podest szary
            DrawRotatedCube(p, 4.0f * s, 0.5f * s, 4.0f * s, obj->rotation, GRAY); 
            break;
        case 1: // Drzewo (pień + korona)
            DrawRotatedCube((Vector3){p.x, p.y + 1.5f * s, p.z}, 0.6f * s, 3.0f * s, 0.6f * s, obj->rotation, BROWN);
            DrawHardwareCube((Vector3){p.x, p.y + 3.5f * s, p.z}, 2.5f * s, 2.0f * s, 2.5f * s, GREEN);
            break;
        case 2: // Baryłka / Przeszkoda
            DrawHardwareCube(p, 0.8f * s, 1.2f * s, 0.8f * s, BROWN);
            DrawHardwareCube((Vector3){p.x, p.y - 0.35f * s, p.z}, 0.84f * s, 0.08f * s, 0.84f * s, BLACK); 
            DrawHardwareCube((Vector3){p.x, p.y + 0.35f * s, p.z}, 0.84f * s, 0.08f * s, 0.84f * s, BLACK); 
            break;
        case 3: // Pojazd / Auto osobowe
            DrawCarObj(obj); 
            break;
        case 4: // Duży podest szary
            DrawRotatedCube(p, 40.0f * s, 0.5f * s, 40.0f * s, obj->rotation, GRAY); 
            break;
        case 5: // Droga (Asfalt typ 1)
        case 9: // Droga (Asfalt typ 2)
            DrawRotatedCube(p, 5.0f * s, 0.2f * s, 60.0f * s, obj->rotation, DARKGRAY); 
            break;
        case 6: // Punkt startu gracza (Widoczny znacznik)
            DrawHardwareCube(p, 0.6f * s, 1.2f * s, 0.6f * s, PURPLE);
            break;
        case 7: // Latarnia miejska
            DrawHardwareCube((Vector3){p.x, p.y + 2.0f * s, p.z}, 0.2f * s, 4.0f * s, 0.2f * s, GRAY);
            DrawHardwareCube((Vector3){p.x, p.y + 4.1f * s, p.z + 0.3f * s}, 0.2f * s, 0.2f * s, 0.8f * s, GRAY);
            DrawHardwareCube((Vector3){p.x, p.y + 3.9f * s, p.z + 0.6f * s}, 0.4f * s, 0.3f * s, 0.4f * s, YELLOW);
            break;
        case 8: // Mur obronny / Ściana ceglana
            DrawRotatedCube(p, 4.0f * s, 1.8f * s, 0.4f * s, obj->rotation, ORANGE);
            break;
        case 10: // Budynek mieszkalny
            DrawHardwareCube(p, 1.6f*s, 1.0f*s, 1.6f*s, (Color){235, 217, 184, 255}); 
            float rfY = p.y + 0.5f * s;
            DrawHardwareCube((Vector3){p.x, rfY + 0.05f*s, p.z}, 1.8f*s, 0.1f*s, 1.8f*s, RED);
            DrawHardwareCube((Vector3){p.x, rfY + 0.15f*s, p.z}, 1.4f*s, 0.1f*s, 1.4f*s, RED);
            DrawHardwareCube((Vector3){p.x, rfY + 0.25f*s, p.z}, 1.0f*s, 0.1f*s, 1.0f*s, RED);
            DrawHardwareCube((Vector3){p.x, rfY + 0.35f*s, p.z}, 0.6f*s, 0.1f*s, 0.6f*s, RED);
            DrawHardwareCube((Vector3){p.x, rfY + 0.45f*s, p.z}, 0.2f*s, 0.1f*s, 0.2f*s, RED);
            DrawHardwareCube((Vector3){p.x, p.y - 0.15f*s, p.z + 0.81f*s}, 0.3f*s, 0.7f*s, 0.02f*s, BROWN);
            DrawHardwareCube((Vector3){p.x - 0.4f*s, p.y + 0.15f*s, p.z + 0.81f*s}, 0.25f*s, 0.25f*s, 0.02f*s, LIGHTGRAY);
            DrawHardwareCube((Vector3){p.x + 0.4f*s, p.y + 0.15f*s, p.z + 0.81f*s}, 0.25f*s, 0.25f*s, 0.02f*s, LIGHTGRAY);
            DrawHardwareCube((Vector3){p.x - 0.4f*s, p.y + 0.15f*s, p.z - 0.81f*s}, 0.25f*s, 0.25f*s, 0.02f*s, LIGHTGRAY);
            DrawHardwareCube((Vector3){p.x + 0.4f*s, p.y + 0.15f*s, p.z - 0.81f*s}, 0.25f*s, 0.25f*s, 0.02f*s, LIGHTGRAY);
            break;
        case 11: // Platforma wodna (Niebieska)
            DrawRotatedCube(p, 4.0f * s, 0.1f * s, 4.0f * s, obj->rotation, BLUE); 
            break;
        case 12: 
        case 18: // Kontener zbiorczy na odpady
            rlPushMatrix();
            rlTranslatef(p.x, p.y, p.z);
            rlRotatef(obj->rotation, 0.0f, 1.0f, 0.0f);
            Color dumpsterGreen = (Color){ 20, 80, 40, 255 }; 
            DrawHardwareCube((Vector3){0.0f, 0.6f*s, 0.0f}, 1.8f*s, 1.2f*s, 1.2f*s, dumpsterGreen); 
            DrawCubePro((Vector3){0.0f, 1.25f*s, 0.0f}, (Vector3){1.9f*s, 0.1f*s, 1.3f*s}, (Vector3){10.0f, 0.0f, 0.0f}, BLACK); 
            DrawHardwareCube((Vector3){-0.95f*s, 0.7f*s, 0.0f}, 0.1f*s, 0.1f*s, 0.6f*s, DARKGRAY);
            DrawHardwareCube((Vector3){ 0.95f*s, 0.7f*s, 0.0f}, 0.1f*s, 0.1f*s, 0.6f*s, DARKGRAY);
            rlPopMatrix();
            break;
        case 13: // Animowane ogniskowe płomienie + bezpieczny dym PVR
            DrawHardwareCube((Vector3){p.x, p.y + 0.1f*s, p.z}, 0.6f*s, 0.2f*s, 0.6f*s, BROWN); 
            float f1 = 0.6f + 0.35f*sinf(t * 16.0f);
            float f2 = 0.5f + 0.25f*cosf(t * 20.0f + 1.0f);
            DrawHardwareCube((Vector3){p.x, p.y + 0.2f*s + f1*0.4f, p.z}, 0.45f*s, f1*s, 0.45f*s, RED);
            DrawHardwareCube((Vector3){p.x + 0.04f*sinf(t*8.0f), p.y + 0.2f*s + f2*0.4f, p.z + 0.04f*cosf(t*8.0f)}, 0.33f*s, f2*s, 0.33f*s, ORANGE);
            DrawHardwareCube((Vector3){p.x - 0.02f*cosf(t*10.0f), p.y + 0.2f*s + f2*0.2f, p.z - 0.02f*sinf(t*10.0f)}, 0.2f*s, f2*0.8f*s, 0.2f*s, YELLOW);
            
            for (int m = 0; m < 6; m++) {
                float smokeProgress = fmodf(t * 0.5f + (m * 0.16f), 1.0f);
                float smokeY = p.y + 0.5f*s + smokeProgress * 5.5f * s; 
                float smokeSize = (0.2f + smokeProgress * 0.3f) * s; 
                float smokeX = p.x + sinf(t * 3.0f + m*1.2f) * 0.25f * s;
                float smokeZ = p.z + cosf(t * 3.0f + m*1.2f) * 0.25f * s;
                Color smokeColor = (Color){110, 110, 110, 255}; 
                DrawHardwareCube((Vector3){smokeX, smokeY, smokeZ}, smokeSize, smokeSize, smokeSize, smokeColor);
            }
            break;
        case 14: // Postać niezależna (NPC)
            if (!npcStates[idx].isDead) {
                float npcTime = t + (float)idx * 0.65f; 
                Vector3 npcPos = p;
                npcPos.y += sinf(npcTime * 2.2f) * 0.025f * s; 
                float headLook = sinf(npcTime * 1.3f) * 0.45f; 
                DrawHuman(npcPos, obj->rotation, s, (Color){40,180,40,255}, 0.0f, headLook); 
            }
            break;
        case 15: // Bariera betonowa drogowa
            DrawRotatedCube(p, 2.0f * s, 1.0f * s, 1.0f * s, obj->rotation, LIGHTGRAY);
            break;
        case 16: // Skrzynia drewniana magazynowa
            DrawRotatedCube(p, 1.2f * s, 1.2f * s, 1.2f * s, obj->rotation, ORANGE);
            break;
        case 17: // Ławka parkowa odpoczynkowa
            rlPushMatrix(); rlTranslatef(p.x, p.y, p.z); rlRotatef(obj->rotation, 0.0f, 1.0f, 0.0f);
            DrawHardwareCube((Vector3){-0.8f*s, 0.25f*s, 0.0f}, 0.12f*s, 0.5f*s, 0.45f*s, DARKGRAY); 
            DrawHardwareCube((Vector3){ 0.8f*s, 0.25f*s, 0.0f}, 0.12f*s, 0.5f*s, 0.45f*s, DARKGRAY); 
            DrawHardwareCube((Vector3){0.0f, 0.55f*s, 0.0f}, 2.0f*s, 0.1f*s, 0.5f*s, BROWN); 
            rlPopMatrix();
            break;
        default: break;
    }
}

static void ExecuteMapLoad(int num) {
    if (num < 1 || num > MAX_MAPS) return;
    if (map_ptrs[num] == NULL || map_counts[num] == 0) return;
    
    map_objects = map_ptrs[num]; 
    map_object_count = map_counts[num]; 
    current_map = num;

    for (int i = 0; i < map_object_count; i++) { carStates[i].speed = 0.0f; carStates[i].wheelRotation = 0.0f; pickedUp[i] = false; }
    if (map_settings[num]) ParseSettings(map_settings[num]);

    player.inCar = false; player.currentCar = -1; player.isHuman = true;
    player.position = (Vector3){0, 0.5f, 0};
    for (int i = 0; i < map_object_count; i++) {
        if (map_objects[i].type == 6) { player.position = map_objects[i].position; break; }
    }
    
    float groundY = -1000.0f;
    for (int i = 0; i < map_object_count; i++) {
        if (map_objects[i].type == 0 || map_objects[i].type == 4 || map_objects[i].type == 5 || map_objects[i].type == 9) {
            float top = map_objects[i].position.y + 0.25f * map_objects[i].scale;
            if (top > groundY) groundY = top;
        }
    }
    if (groundY > -1000.0f && player.position.y < groundY) player.position.y = groundY;
    if (player.position.y < 0.25f) player.position.y = 0.25f;

    player.velocity = (Vector3){0,0,0}; player.yaw = 0;
}

static void UpdatePlayer(float dt) {
    float moveForward = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
    float turn = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
    if (fabsf(moveForward) < 0.15f) moveForward = 0.0f;
    if (fabsf(turn) < 0.15f) turn = 0.0f;
    
    bool btnA_down = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    bool btnA_pressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    bool btnY = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    bool btnL = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1);

    // SYSTEM OBRACANIA KAMERY W PIONIE (DPAD UP / DPAD DOWN)
    if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
        cameraPitchOffset += 4.0f * dt; // Podnoszenie oczu kamery w górę
        if (cameraPitchOffset > 6.0f) cameraPitchOffset = 6.0f;
    }
    if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
        cameraPitchOffset -= 4.0f * dt; // Opuszczanie oczu kamery w dół
        if (cameraPitchOffset < -2.0f) cameraPitchOffset = -2.0f;
    }

    if (player.inCar) {
        int ci = player.currentCar; CarState* cs = &carStates[ci]; MapObject* car = &map_objects[ci];
        if (btnA_down) cs->speed += CAR_ACCEL * dt; 
        if (btnL) cs->speed -= CAR_BRAKE * dt;
        
        if (cs->speed > 0) { cs->speed -= CAR_FRICTION * dt; if (cs->speed < 0) cs->speed = 0; }
        else if (cs->speed < 0) { cs->speed += CAR_FRICTION * dt; if (cs->speed > 0) cs->speed = 0; }
        if (cs->speed > CAR_MAX_SPEED) cs->speed = CAR_MAX_SPEED;
        if (cs->speed < -CAR_MAX_SPEED * 0.4f) cs->speed = -CAR_MAX_SPEED * 0.4f;

        car->rotation -= turn * 45.0f * dt * (fabsf(cs->speed) / CAR_MAX_SPEED + 0.1f);
        float rad = car->rotation * DEG2RAD;
        car->position.x += sinf(rad) * cs->speed * dt; car->position.z += cosf(rad) * cs->speed * dt;
        player.position = car->position;
        if (btnY) { player.inCar = false; player.currentCar = -1; player.position.y += 1.5f; cs->speed = 0; }
        walkAmount = 0.0f; 
    } else {
        if (btnY) {
            int nearest = -1; float bestDist = CAR_ENTER_DIST;
            for (int i = 0; i < map_object_count; i++) {
                if (map_objects[i].type != 3) continue;
                float d = Vector3Distance(player.position, map_objects[i].position);
                if (d < bestDist) { bestDist = d; nearest = i; }
            }
            if (nearest >= 0) { player.inCar = true; player.currentCar = nearest; }
        }
        
        player.yaw -= turn * TURN_SPEED * dt;
        
        float speed = 0.0f;
        if (moveForward < -0.15f) speed = PLAYER_SPEED; 
        else if (moveForward > 0.15f) speed = -PLAYER_SPEED; 
        
        player.velocity.x = sinf(player.yaw) * speed;
        player.velocity.z = cosf(player.yaw) * speed;

        player.velocity.y -= GRAVITY * dt;
        player.position.x += player.velocity.x * dt; 
        player.position.y += player.velocity.y * dt; 
        player.position.z += player.velocity.z * dt;

        bool onGround = false; float groundY = -1000.0f;
        for (int i = 0; i < map_object_count; i++) {
            MapObject* obj = &map_objects[i]; 
            if (obj->type != 0 && obj->type != 4 && obj->type != 5 && obj->type != 9) continue;
            float halfX, halfZ;
            if (obj->type == 4) { halfX = 20.0f * obj->scale; halfZ = 20.0f * obj->scale; }
            else if (obj->type == 5 || obj->type == 9) { halfX = 2.5f * obj->scale; halfZ = 30.0f * obj->scale; }
            else { halfX = 2.0f * obj->scale; halfZ = 2.0f * obj->scale; }
            
            float platformTop = obj->position.y + 0.25f * obj->scale;
            float rad = obj->rotation * DEG2RAD;
            float dx = player.position.x - obj->position.x; 
            float dz = player.position.z - obj->position.z;
            float rdx = dx*cosf(rad) + dz*sinf(rad); 
            float rdz = -dx*sinf(rad) + dz*cosf(rad);
            if (rdx >= -halfX - 1.0f && rdx <= halfX + 1.0f && rdz >= -halfZ - 1.0f && rdz <= halfZ + 1.0f) {
                if (platformTop > groundY) groundY = platformTop;
                if (player.position.y <= platformTop && player.position.y >= platformTop - 0.5f && player.velocity.y <= 0) { 
                    player.position.y = platformTop; player.velocity.y = 0; onGround = true; player.isJumping = false; 
                }
            }
        }

        if (!onGround && groundY > -1000.0f && player.position.y < groundY) { player.position.y = groundY; player.velocity.y = 0; onGround = true; player.isJumping = false; }
        if (player.position.y < 0.0f && !onGround) { player.position.y = 0.0f; player.velocity.y = 0; onGround = true; player.isJumping = false; }
        
        if (btnA_pressed && onGround) { player.velocity.y = JUMP_FORCE; player.isJumping = true; }

        if (fabsf(speed) > 0.1f && onGround) {
            walkAmount = 1.0f; walkPhase += 12.0f * dt;
        } else {
            walkAmount = 0.0f;
        }
    }
}

int main(int argc, char *argv[]) {
    InitWindow(640, 480, "GTAsaDC");
    SetTargetFPS(60);
    InitMapTables(); 
    ExecuteMapLoad(1);

    Image img = GenImageColor(2, 2, WHITE);
    Texture2D whiteTex = LoadTextureFromImage(img);
    UnloadImage(img);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt <= 0.0001f) dt = 1.0f / 60.0f; 
        if (dt > 0.1f) dt = 0.1f; 

        globalTime = (float)GetTime(); 

        pvr_wait_ready();
        UpdatePlayer(dt);

        // Wyliczanie wysokości docelowej kamery (baza 3.0f lub 4.0f + offset z D-Pada)
        if (player.inCar) {
            MapObject* car = &map_objects[player.currentCar]; float rad = car->rotation * DEG2RAD;
            camera.position = (Vector3){ car->position.x - sinf(rad)*8.0f, car->position.y + 4.0f + cameraPitchOffset, car->position.z - cosf(rad)*8.0f };
            camera.target = car->position;
        } else {
            camera.position = (Vector3){ player.position.x - sinf(player.yaw)*6.0f, player.position.y + 3.0f + cameraPitchOffset, player.position.z - cosf(player.yaw)*6.0f };
            camera.target = player.position;
        }
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; camera.fovy = 60.0f; camera.projection = CAMERA_PERSPECTIVE;

        BeginDrawing();
        ClearBackground(bgColor);
        rlDrawRenderBatchActive();      
        
        BeginMode3D(camera);
            rlSetTexture(whiteTex.id); 

            for (int i = 0; i < map_object_count; i++) {
                if (Vector3Distance(map_objects[i].position, camera.position) < MAX_DRAW_DISTANCE) {
                    DrawMapObject(&map_objects[i]);
                }
            }
            
            if (!player.inCar) {
                float pSwing = sinf(walkPhase) * 0.4f * walkAmount;
                DrawHuman(player.position, player.yaw, 1.0f, RED, pSwing, 0.0f);
            }
        EndMode3D();

        rlDrawRenderBatchActive();      

        DrawRectangle(0, 0, 640, 35, Fade(BLACK, 0.5f));
        DrawText(TextFormat("MAPA: %d | FPS: %d", current_map, GetFPS()), 10, 8, 20, WHITE);
        if (player.inCar) DrawText("D-Pad G/D: Widok pionowy | A: Gaz | Y: Wysiadz", 10, 440, 18, YELLOW);
        else DrawText("D-Pad G/D: Widok pionowy | Analog: Ruch | Y: Auto", 10, 440, 18, WHITE);
        
        EndDrawing();
    }

    UnloadTexture(whiteTex);
    CloseWindow();
    return 0;
}