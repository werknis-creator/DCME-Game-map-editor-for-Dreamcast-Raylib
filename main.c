/***********************************************************************
GTAsaDC - Gra 3D na Dreamcast z autem i ludzikiem
Sterowanie pieszo:
Analog G/D: chodzenie, L/P: obracanie
X/A: skok, Y przy aucie: wsiądź
Sterowanie w aucie:
Analog G/D: jazda, R: gaz, L: hamulec/cofanie, Y: wysiądź
Nowości v2.4:
Obsługa obiektów smallhouse (typ 10) i water (typ 11)
Komendy D(kolor) i N(kolor) – dzień/noc (2 minuty każdy)
Poprawka dla znikających obiektów na Flycast (głębokość)
***********************************************************************/
#include <kos.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

#define PLAYER_SPEED 5.0f
#define TURN_SPEED 3.0f
#define JUMP_FORCE 8.0f
#define GRAVITY 20.0f
#define CAMERA_DISTANCE 6.0f
#define CAMERA_HEIGHT 3.0f
#define CAR_MAX_SPEED 15.0f
#define CAR_ACCEL 5.0f
#define CAR_BRAKE 8.0f
#define CAR_FRICTION 2.0f
#define CAR_ENTER_DIST 3.5f
#define MAX_OBJECTS 512
#define MAX_MAPS 10

typedef struct {
    int type;
    Vector3 position;
    float scale;
    float rotation;
} MapObject;

static MapObject* map_objects = NULL;
static int map_object_count = 0;
static int current_map = 1;

/* Słabe symbole dla map */
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

/* Zmienne globalne dla komend */
static Color bgColor = SKYBLUE;
static Color dayColor = SKYBLUE;
static Color nightColor = (Color){20, 20, 50, 255};
static float timeOfDay = 0.0f;

/* --- Struktury gracza i stanu --- */
typedef struct {
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float width;
    float height;
    bool isJumping;
    Color color;
    bool inCar;
    int currentCar;
    bool isHuman;
} Player;

typedef struct {
    float speed;
    float wheelRotation;
} CarState;

static Player player;
static CarState carStates[MAX_OBJECTS];
static Camera3D camera = { 0 };
static float firePhase = 0.0f;
static float walkPhase = 0.0f;
static float walkAmount = 0.0f;
static float triggerCooldown = 0.0f;

/* Funkcje parsowania kolorów */
static Color ParseColorName(const char* name) {
    if (strcmp(name, "black") == 0) return BLACK;
    if (strcmp(name, "white") == 0) return WHITE;
    if (strcmp(name, "red") == 0) return RED;
    if (strcmp(name, "green") == 0) return GREEN;
    if (strcmp(name, "blue") == 0) return BLUE;
    if (strcmp(name, "yellow") == 0) return YELLOW;
    if (strcmp(name, "gray") == 0) return GRAY;
    if (strcmp(name, "darkgray") == 0) return DARKGRAY;
    if (strcmp(name, "skyblue") == 0) return SKYBLUE;
    if (strcmp(name, "purple") == 0) return PURPLE;
    if (strcmp(name, "orange") == 0) return ORANGE;
    if (strcmp(name, "pink") == 0) return (Color){255, 192, 203, 255};
    if (strcmp(name, "brown") == 0) return (Color){139, 69, 19, 255};
    if (strcmp(name, "cyan") == 0) return (Color){0, 255, 255, 255};
    if (strcmp(name, "magenta") == 0) return (Color){255, 0, 255, 255};
    if (strcmp(name, "lime") == 0) return (Color){0, 255, 0, 255};
    return SKYBLUE;
}

/* Parsuje komendy z settings stringa */
static void ParseSettings(const char* settings) {
    if (!settings || !*settings) return;
    char buf[512];
    strncpy(buf, settings, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    char* token = strtok(buf, "\n");
    while (token) {
        char* p = token;
        while (*p == ' ' || *p == '\t') p++;
        char* end = p + strlen(p) - 1;
        while (end > p && (*end == ' ' || *end == '\t' || *end == '\r')) { *end = '\0'; end--; }
        
        if (strncmp(p, "background", 10) == 0) {
            char* paren = strchr(p, '(');
            if (paren) {
                char* endparen = strchr(paren, ')');
                if (endparen) {
                    *endparen = '\0';
                    char* color = paren + 1;
                    while (*color == ' ' || *color == '\t') color++;
                    bgColor = ParseColorName(color);
                    dayColor = bgColor;
                }
            }
        }
        else if (strncmp(p, "D", 1) == 0 && p[1] == '(') {
            char* paren = strchr(p, '(');
            if (paren) {
                char* endparen = strchr(paren, ')');
                if (endparen) {
                    *endparen = '\0';
                    char* color = paren + 1;
                    while (*color == ' ' || *color == '\t') color++;
                    dayColor = ParseColorName(color);
                }
            }
        }
        else if (strncmp(p, "N", 1) == 0 && p[1] == '(') {
            char* paren = strchr(p, '(');
            if (paren) {
                char* endparen = strchr(paren, ')');
                if (endparen) {
                    *endparen = '\0';
                    char* color = paren + 1;
                    while (*color == ' ' || *color == '\t') color++;
                    nightColor = ParseColorName(color);
                }
            }
        }
        token = strtok(NULL, "\n");
    }
}

static void InitMapTables(void) {
    int i;
    for (i = 0; i <= MAX_MAPS; i++) { map_ptrs[i] = NULL; map_counts[i] = 0; map_settings[i] = NULL; }
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

/* --- Funkcje rysowania --- */
static void DrawRotatedCube(Vector3 pos, float sx, float sy, float sz, float rotationDeg, Color color) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rotationDeg, 0.0f, 1.0f, 0.0f);
    DrawCube((Vector3){0.0f, 0.0f, 0.0f}, sx, sy, sz, color);
    rlPopMatrix();
}

static void DrawSphereLow(Vector3 pos, float radius, Color color) {
    DrawSphereEx(pos, radius, 6, 8, color);
}

static void DrawCarObj(MapObject* obj) {
    float s = obj->scale;
    Vector3 p = obj->position;
    float r = obj->rotation;
    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(r, 0.0f, 1.0f, 0.0f);

    DrawCube((Vector3){0.0f, 0.8f*s, -1.0f*s}, 2.0f*s, 1.1f*s, 2.0f*s, RED);
    DrawCube((Vector3){0.0f, 0.8f*s,  0.2f*s}, 1.7f*s, 1.0f*s, 1.6f*s, RED);
    DrawCube((Vector3){0.0f, 0.7f*s,  1.4f*s}, 1.1f*s, 0.8f*s, 1.4f*s, MAROON);
    DrawCube((Vector3){0.0f, 1.7f*s, -0.4f*s}, 1.6f*s, 0.9f*s, 1.6f*s, DARKGRAY);
    DrawCube((Vector3){-0.45f*s, 0.75f*s, 2.05f*s}, 0.25f*s, 0.25f*s, 0.1f*s, YELLOW);
    DrawCube((Vector3){ 0.45f*s, 0.75f*s, 2.05f*s}, 0.25f*s, 0.25f*s, 0.1f*s, YELLOW);

    rlPopMatrix();
}

static void DrawHuman(Vector3 pos, float yaw, float scale) {
    float s = scale;
    float swing = sinf(walkPhase) * 0.35f * walkAmount;
    float bob   = fabsf(sinf(walkPhase)) * 0.06f * walkAmount;
    Color skin   = (Color){ 235, 190, 150, 255 };
    Color shirt  = (Color){ 200, 40, 40, 255 };
    Color pants  = (Color){ 40, 60, 130, 255 };
    Color shoes  = (Color){ 30, 30, 30, 255 };
    Color hair   = (Color){ 80, 50, 20, 255 };

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y + bob*s, pos.z);
    rlRotatef(yaw * RAD2DEG, 0.0f, 1.0f, 0.0f);

    DrawCube((Vector3){-0.18f*s, 0.4f*s,  swing*s}, 0.26f*s, 0.8f*s, 0.26f*s, pants);
    DrawCube((Vector3){-0.18f*s, 0.05f*s, swing*s}, 0.28f*s, 0.18f*s, 0.34f*s, shoes);
    DrawCube((Vector3){ 0.18f*s, 0.4f*s, -swing*s}, 0.26f*s, 0.8f*s, 0.26f*s, pants);
    DrawCube((Vector3){ 0.18f*s, 0.05f*s,-swing*s}, 0.28f*s, 0.18f*s, 0.34f*s, shoes);

    DrawCube((Vector3){0.0f, 1.15f*s, 0.0f}, 0.62f*s, 0.75f*s, 0.4f*s, shirt);

    DrawCube((Vector3){-0.45f*s, 1.15f*s, -swing*s}, 0.2f*s, 0.65f*s, 0.2f*s, shirt);
    DrawCube((Vector3){-0.45f*s, 0.78f*s, -swing*s}, 0.18f*s, 0.18f*s, 0.18f*s, skin);
    DrawCube((Vector3){ 0.45f*s, 1.15f*s,  swing*s}, 0.2f*s, 0.65f*s, 0.2f*s, shirt);
    DrawCube((Vector3){ 0.45f*s, 0.78f*s,  swing*s}, 0.18f*s, 0.18f*s, 0.18f*s, skin);

    DrawSphereEx((Vector3){0.0f, 1.75f*s, 0.0f}, 0.25f*s, 6, 8, skin);
    DrawCube((Vector3){0.0f, 1.9f*s, -0.05f*s}, 0.5f*s, 0.18f*s, 0.5f*s, hair);
    DrawCube((Vector3){0.0f, 1.72f*s, 0.24f*s}, 0.1f*s, 0.1f*s, 0.1f*s, skin);

    rlPopMatrix();
}

static void DrawFireObj(MapObject* obj) {
    Vector3 p = obj->position;
    float s = obj->scale;
    float t = firePhase;
    float mainSize = 1.5f * s * (1.0f + 0.3f * sinf(t * 3.0f));
    DrawSphereLow(p, mainSize, ORANGE);
    float innerSize = 1.0f * s * (1.0f + 0.4f * sinf(t * 5.0f + 1.0f));
    DrawSphereLow(p, innerSize, RED);

    for (int i = 0; i < 4; i++) {
        float angle = (float)i * 90.0f * DEG2RAD + t * 2.0f;
        float radius = 1.2f * s;
        float sparkY = p.y + 0.5f * s + 0.8f * s * sinf(t * 4.0f + (float)i);
        Vector3 sparkPos = { p.x + cosf(angle) * radius * 0.5f, sparkY, p.z + sinf(angle) * radius * 0.5f };
        float sparkSize = 0.4f * s * (0.5f + 0.5f * sinf(t * 6.0f + (float)i * 2.0f));
        DrawSphereLow(sparkPos, sparkSize, YELLOW);
    }

    for (int i = 0; i < 6; i++) {
        float phase = t * 1.5f + (float)i * 1.2f;
        float dx = sinf(phase * 0.7f) * 1.2f * s;
        float dz = cosf(phase * 0.5f) * 1.2f * s;
        float dy = 1.5f * s + (sinf(phase * 0.9f) * 0.5f + 0.5f) * 2.5f * s;
        float radius = (0.6f + 0.4f * sinf(phase * 0.3f)) * s;
        DrawSphereLow((Vector3){p.x + dx, p.y + dy, p.z + dz}, radius, (Color){120, 120, 120, 200});
    }
}

static void DrawSmallHouse(MapObject* obj) {
    float s = obj->scale;
    Vector3 p = obj->position;
    float r = obj->rotation;
    Color cream = (Color){235, 217, 184, 255};
    Color roofRed = (Color){200, 40, 40, 255};
    DrawRotatedCube((Vector3){p.x, p.y + 0.5f*s, p.z}, 1.6f*s, 1.0f*s, 1.6f*s, r, cream);
    DrawRotatedCube((Vector3){p.x, p.y + 0.5f*s, p.z + 0.82f*s}, 0.4f*s, 0.6f*s, 0.1f*s, r, (Color){100, 50, 30, 255});
    float rad = r * DEG2RAD;
    float c = cosf(rad), sn = sinf(rad);
    float hw = 0.9f*s, hh = 0.7f*s, hl = 0.9f*s;
    Vector3 pts[6] = {
        { -hw, 0.0f,  hl }, { hw, 0.0f,  hl }, { 0.0f, hh,  hl },
        { -hw, 0.0f, -hl }, { hw, 0.0f, -hl }, { 0.0f, hh, -hl }
    };
    for (int i = 0; i < 6; i++) {
        float ox = pts[i].x, oz = pts[i].z;
        pts[i].x = ox * c - oz * sn + p.x;
        pts[i].z = ox * sn + oz * c + p.z;
        pts[i].y += p.y + 1.0f*s;
    }
    rlPushMatrix();
    rlDisableBackfaceCulling();
    rlBegin(RL_TRIANGLES);
    rlColor4ub(roofRed.r, roofRed.g, roofRed.b, roofRed.a);
    rlVertex3f(pts[0].x, pts[0].y, pts[0].z);
    rlVertex3f(pts[1].x, pts[1].y, pts[1].z);
    rlVertex3f(pts[2].x, pts[2].y, pts[2].z);
    rlVertex3f(pts[3].x, pts[3].y, pts[3].z);
    rlVertex3f(pts[5].x, pts[5].y, pts[5].z);
    rlVertex3f(pts[4].x, pts[4].y, pts[4].z);
    rlVertex3f(pts[0].x, pts[0].y, pts[0].z);
    rlVertex3f(pts[2].x, pts[2].y, pts[2].z);
    rlVertex3f(pts[5].x, pts[5].y, pts[5].z);
    rlVertex3f(pts[0].x, pts[0].y, pts[0].z);
    rlVertex3f(pts[5].x, pts[5].y, pts[5].z);
    rlVertex3f(pts[3].x, pts[3].y, pts[3].z);
    rlVertex3f(pts[1].x, pts[1].y, pts[1].z);
    rlVertex3f(pts[4].x, pts[4].y, pts[4].z);
    rlVertex3f(pts[5].x, pts[5].y, pts[5].z);
    rlVertex3f(pts[1].x, pts[1].y, pts[1].z);
    rlVertex3f(pts[5].x, pts[5].y, pts[5].z);
    rlVertex3f(pts[2].x, pts[2].y, pts[2].z);
    rlEnd();
    rlEnableBackfaceCulling();
    rlPopMatrix();
    rlPushMatrix();
    rlDisableBackfaceCulling();
    rlBegin(RL_LINES);
    rlColor4ub(0,0,0,255);
    for (int i = 0; i < 3; i++) {
        int next = (i+1)%3;
        rlVertex3f(pts[i].x, pts[i].y, pts[i].z);
        rlVertex3f(pts[next].x, pts[next].y, pts[next].z);
        rlVertex3f(pts[i+3].x, pts[i+3].y, pts[i+3].z);
        rlVertex3f(pts[(next+3)%6].x, pts[(next+3)%6].y, pts[(next+3)%6].z);
        rlVertex3f(pts[i].x, pts[i].y, pts[i].z);
        rlVertex3f(pts[i+3].x, pts[i+3].y, pts[i+3].z);
    }
    rlEnd();
    rlEnableBackfaceCulling();
    rlPopMatrix();
}

static void DrawWater(MapObject* obj, float time) {
    float s = obj->scale;
    Vector3 p = obj->position;
    float r = obj->rotation;
    int segs = 8;
    float half = 2.0f * s;
    float step = (2.0f*half) / segs;
    Color waterColor = (Color){0, 100, 200, 160};
    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(r, 0.0f, 1.0f, 0.0f);
    rlDisableBackfaceCulling();
    rlBegin(RL_QUADS);
    rlColor4ub(waterColor.r, waterColor.g, waterColor.b, waterColor.a);
    for (int i = 0; i < segs; i++) {
        for (int j = 0; j < segs; j++) {
            float x0 = -half + i * step;
            float x1 = x0 + step;
            float z0 = -half + j * step;
            float z1 = z0 + step;
            float wave = 0.1f * s * sinf(time * 2.0f + x0 * 0.5f + z0 * 0.3f);
            float wave2 = 0.1f * s * sinf(time * 1.7f + x1 * 0.4f + z1 * 0.5f + 1.2f);
            float y0 = wave;
            float y1 = 0.1f * s * sinf(time * 2.0f + x1 * 0.5f + z0 * 0.3f + 0.7f);
            float y2 = 0.1f * s * sinf(time * 1.8f + x0 * 0.4f + z1 * 0.5f + 2.1f);
            float y3 = wave2;
            rlVertex3f(x0, y0, z0);
            rlVertex3f(x1, y1, z0);
            rlVertex3f(x1, y3, z1);
            rlVertex3f(x0, y2, z1);
        }
    }
    rlEnd();
    rlEnableBackfaceCulling();
    rlPopMatrix();
}

static void DrawMapObject(MapObject* obj) {
    switch (obj->type) {
    case 0:
        DrawRotatedCube(obj->position, 4.0f * obj->scale, 0.5f * obj->scale, 4.0f * obj->scale, obj->rotation, GRAY);
        break;
    case 1:
        DrawRotatedCube((Vector3){obj->position.x, obj->position.y + 2.0f * obj->scale, obj->position.z},
        1.0f * obj->scale, 4.0f * obj->scale, 1.0f * obj->scale, obj->rotation, BROWN);
        DrawSphereLow((Vector3){obj->position.x, obj->position.y + 4.5f * obj->scale, obj->position.z},
        2.0f * obj->scale, GREEN);
        break;
    case 2:
        DrawFireObj(obj);
        break;
    case 3:
        DrawCarObj(obj);
        break;
    case 4:
        DrawRotatedCube(obj->position, 40.0f * obj->scale, 0.5f * obj->scale, 40.0f * obj->scale, obj->rotation, GRAY);
        break;
    case 5:
    case 9: {
        float s = obj->scale;
        Vector3 p = obj->position;
        float r = obj->rotation;
        DrawRotatedCube(p, 5.0f * s, 0.2f * s, 60.0f * s, r, DARKGRAY);
        float rad = r * DEG2RAD;
        for (int seg = -28; seg <= 28; seg += 3) {
            float dx = sinf(rad) * seg * s;
            float dz = cosf(rad) * seg * s;
            DrawRotatedCube((Vector3){p.x + dx, p.y + 0.11f, p.z + dz},
            0.3f * s, 0.02f * s, 1.2f * s, r, YELLOW);
        }
        float cw[2] = {-2.7f * s, 2.7f * s};
        for (int i = 0; i < 2; i++) {
            float cx = -sinf(rad) * cw[i];
            float cz = cosf(rad) * cw[i];
            DrawRotatedCube((Vector3){p.x + cx, p.y + 0.15f, p.z + cz},
            0.2f * s, 0.1f * s, 60.0f*s, r, LIGHTGRAY);
        }
        break;
    }
    case 6: break;
    case 7: break;
    case 8: break;
    case 10:
        DrawSmallHouse(obj);
        break;
    case 11:
        DrawWater(obj, firePhase);
        break;
    default: break;
    } 
}

/* --- Ładowanie mapy --- */
static void LoadMap(int num) {
    if (num < 1 || num > MAX_MAPS) return;
    if (map_ptrs[num] == NULL || map_counts[num] == 0) return;
    map_objects = map_ptrs[num];
    map_object_count = map_counts[num];
    current_map = num;
    for (int i = 0; i < map_object_count; i++) {
        carStates[i].speed = 0.0f;
        carStates[i].wheelRotation = 0.0f;
    }

    dayColor = SKYBLUE;
    nightColor = (Color){20, 20, 50, 255};
    bgColor = SKYBLUE;

    if (map_settings[num]) {
        ParseSettings(map_settings[num]);
    }

    player.inCar = false;
    player.currentCar = -1;
    player.isHuman = false;
    for (int i = 0; i < map_object_count; i++) {
        if (map_objects[i].type == 6) {
            player.position = map_objects[i].position;
            player.position.y += 1.0f;
            player.isHuman = true;
            break;
        }
    }
    if (!player.isHuman) {
        player.position = (Vector3){0, 2, 0};
    }
    player.velocity = (Vector3){0,0,0};
    player.yaw = 0;
}

static void InitPlayer(void) {
    player.width = 1.0f;
    player.height = 2.0f;
    player.color = YELLOW;
    player.isJumping = false;
    InitMapTables();
    LoadMap(1);
}

static int FindNearestCar(void) {
    int best = -1;
    float bestDist = CAR_ENTER_DIST;
    for (int i = 0; i < map_object_count; i++) {
        if (map_objects[i].type != 3) continue;
        float d = Vector3Distance(player.position, map_objects[i].position);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

static void CheckTriggerZones(float dt) {
    if (triggerCooldown > 0.0f) {
        triggerCooldown -= dt;
        return;
    }
    if (map_objects == NULL) return;
    for (int i = 0; i < map_object_count; i++) {
        if (map_objects[i].type == 7 || map_objects[i].type == 8) {
            float hs = 2.0f * map_objects[i].scale;
            float dx = player.position.x - map_objects[i].position.x;
            float dz = player.position.z - map_objects[i].position.z;
            if (fabsf(dx) < hs && fabsf(dz) < hs) {
                int targetMap = current_map;
                if (map_objects[i].type == 7) targetMap = current_map + 1;
                else targetMap = current_map - 1;
                if (targetMap >= 1 && targetMap <= MAX_MAPS && map_ptrs[targetMap] != NULL) {
                    LoadMap(targetMap);
                    triggerCooldown = 0.6f;
                }
                break;
            }
        }
    }
}

static void UpdatePlayer(float dt) {
    float moveForward = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
    float turn = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
    if (fabsf(moveForward) < 0.15f) moveForward = 0.0f;
    if (fabsf(turn) < 0.15f) turn = 0.0f;
    bool btnY = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    bool btnR = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    bool btnL = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
    bool btnA = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    bool btnX = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);

    if (player.inCar) {
        int ci = player.currentCar;
        CarState* cs = &carStates[ci];
        MapObject* car = &map_objects[ci];
        
        if (btnR) cs->speed += CAR_ACCEL * dt;
        if (btnL) {
            if (cs->speed > 0) cs->speed -= CAR_BRAKE * dt;
            else cs->speed -= CAR_ACCEL * 0.5f * dt;
        }
        if (moveForward != 0.0f) {
            cs->speed += -moveForward * CAR_ACCEL * dt;
        }
        
        if (cs->speed > 0) {
            cs->speed -= CAR_FRICTION * dt;
            if (cs->speed < 0) cs->speed = 0;
        } else if (cs->speed < 0) {
            cs->speed += CAR_FRICTION * dt;
            if (cs->speed > 0) cs->speed = 0;
        }
        
        if (cs->speed > CAR_MAX_SPEED) cs->speed = CAR_MAX_SPEED;
        if (cs->speed < -CAR_MAX_SPEED * 0.4f) cs->speed = -CAR_MAX_SPEED * 0.4f;
        
        if (fabsf(cs->speed) > 0.1f) {
            car->rotation -= turn * 60.0f * dt * (cs->speed / CAR_MAX_SPEED);
        }
        
        float rad = car->rotation * DEG2RAD;
        car->position.x += sinf(rad) * cs->speed * dt;
        car->position.z += cosf(rad) * cs->speed * dt;
        
        cs->wheelRotation += cs->speed * dt * 3.0f;
        
        player.position = car->position;
        player.position.y = car->position.y + 1.5f * car->scale;
        player.yaw = car->rotation * DEG2RAD;
        
        if (btnY) {
            player.inCar = false;
            player.currentCar = -1;
            player.position.y += 2.0f;
            cs->speed = 0;
        }
    } else {
        player.yaw -= turn * TURN_SPEED * dt;
        
        float moveSpeed = -moveForward * PLAYER_SPEED;
        player.velocity.x = sinf(player.yaw) * moveSpeed;
        player.velocity.z = cosf(player.yaw) * moveSpeed;

        float moving = (fabsf(moveForward) > 0.15f) ? 1.0f : 0.0f;
        walkAmount += (moving - walkAmount) * fminf(1.0f, dt * 10.0f);
        if (moving > 0.5f) walkPhase += dt * 8.0f;
        
        player.velocity.y -= GRAVITY * dt;
        
        player.position.x += player.velocity.x * dt;
        player.position.y += player.velocity.y * dt;
        player.position.z += player.velocity.z * dt;
        
        bool onGround = false;
        for (int i = 0; i < map_object_count; i++) {
            MapObject* obj = &map_objects[i];
            if (obj->type != 0 && obj->type != 4 && obj->type != 5 && obj->type != 9 && obj->type != 11) continue;
            
            float halfX, halfZ;
            if (obj->type == 4) { halfX = 20.0f * obj->scale; halfZ = 20.0f * obj->scale; }
            else if (obj->type == 5 || obj->type == 9) { halfX = 2.5f * obj->scale; halfZ = 30.0f * obj->scale; }
            else if (obj->type == 11) { halfX = 2.0f * obj->scale; halfZ = 2.0f * obj->scale; }
            else { halfX = 2.0f * obj->scale; halfZ = 2.0f * obj->scale; }
            
            float platformTop = obj->position.y + 0.25f * obj->scale;
            if (obj->type == 11) platformTop = obj->position.y + 0.1f * obj->scale;
            
            float rad = obj->rotation * DEG2RAD;
            float dx = player.position.x - obj->position.x;
            float dz = player.position.z - obj->position.z;
            float rdx = dx*cosf(rad) + dz*sinf(rad);
            float rdz = -dx*sinf(rad) + dz*cosf(rad);
            
            if (rdx >= -halfX - player.width/2 && rdx <= halfX + player.width/2 &&
                rdz >= -halfZ - player.width/2 && rdz <= halfZ + player.width/2) {
                if (obj->type == 11) {
                    if (player.position.y < platformTop) {
                        player.position.y = platformTop - 0.5f;
                        player.velocity.y = 0;
                    }
                    continue;
                }
                if (player.position.y - player.height/2 <= platformTop &&
                    player.position.y - player.height/2 >= platformTop - 0.5f &&
                    player.velocity.y <= 0) {
                    player.position.y = platformTop + player.height/2;
                    player.velocity.y = 0;
                    onGround = true;
                    player.isJumping = false;
                }
            }
        }
        
        if (player.position.y < 0.5f && !onGround) {
            player.position.y = 0.5f;
            player.velocity.y = 0;
            onGround = true;
            player.isJumping = false;
        }
        
        if (player.position.y < -20.0f) {
            player.position = (Vector3){ 0.0f, 5.0f, 0.0f };
            player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
            player.yaw = 0.0f;
        }
        
        if ((btnA || btnX) && onGround) {
            player.velocity.y = JUMP_FORCE;
            player.isJumping = true;
        }
        
        if (btnY) {
            int nearest = FindNearestCar();
            if (nearest >= 0) {
                player.inCar = true;
                player.currentCar = nearest;
                player.position = map_objects[nearest].position;
            }
        }
        
        CheckTriggerZones(dt);
    }
}

static void UpdateDayNight(float dt) {
    static float cycleTime = 0.0f;
    cycleTime += dt;
    float period = 240.0f;
    float phase = fmodf(cycleTime, period) / period;
    timeOfDay = phase;
    float t = 0.0f;
    if (phase < 0.5f) {
        t = phase / 0.5f;
        t = t * t;
    } else {
        t = 1.0f - (phase - 0.5f) / 0.5f;
        t = t * t;
    }
    bgColor.r = (unsigned char)(dayColor.r * t + nightColor.r * (1.0f - t));
    bgColor.g = (unsigned char)(dayColor.g * t + nightColor.g * (1.0f - t));
    bgColor.b = (unsigned char)(dayColor.b * t + nightColor.b * (1.0f - t));
    bgColor.a = 255;
}

static void UpdateGameCamera(void) {
    if (player.inCar) {
        MapObject* car = &map_objects[player.currentCar];
        float rad = car->rotation * DEG2RAD;
        camera.position.x = car->position.x - sinf(rad) * 10.0f;
        camera.position.y = car->position.y + 5.0f;
        camera.position.z = car->position.z - cosf(rad) * 10.0f;
        camera.target = car->position;
    } else {
        camera.position.x = player.position.x - sinf(player.yaw) * CAMERA_DISTANCE;
        camera.position.y = player.position.y + CAMERA_HEIGHT;
        camera.position.z = player.position.z - cosf(player.yaw) * CAMERA_DISTANCE;
        camera.target = player.position;
    }
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

static void DrawMap(void) {
    for (int i = 0; i < map_object_count; i++) {
        DrawMapObject(&map_objects[i]);
    }
    if (!player.inCar) {
        if (player.isHuman) {
            DrawHuman(player.position, player.yaw, 1.0f);
        } else {
            DrawCube(player.position, player.width, player.height, player.width, player.color);
            DrawCubeWires(player.position, player.width, player.height, player.width, BLACK);
            Vector3 nosePos = {
                player.position.x + sinf(player.yaw) * 0.8f,
                player.position.y,
                player.position.z + cosf(player.yaw) * 0.8f
            };
            DrawCube(nosePos, 0.3f, 0.3f, 0.3f, RED);
        }
    }
}

static void DrawHUD(void) {
    int sw = 640, sh = 480;
    DrawRectangle(0, 0, sw, 40, Fade(BLACK, 0.6f));
    DrawRectangle(0, sh - 50, sw, 50, Fade(BLACK, 0.6f));

    DrawText(TextFormat("GTAsaDC - Mapa %d", current_map), 10, 8, 20, WHITE);
    DrawText(TextFormat("FPS: %i  |  Obiekty: %d", GetFPS(), map_object_count), sw - 200, 8, 20, YELLOW);

    if (player.isHuman) {
        DrawRectangle(10, 50, 200, 30, Fade(BLACK, 0.7f));
        DrawText("Player1.dcme Active", 15, 55, 20, GREEN);
    }

    if (player.inCar) {
        DrawText("W aucie! R:gaz L:hamulec Y:wysiadz", 10, sh - 35, 16, YELLOW);
        DrawText(TextFormat("Predkosc: %.1f", carStates[player.currentCar].speed), 10, sh - 18, 16, GREEN);
    } else {
        DrawText("Analog G/D:chodzenie L/P:obrot X/A:skok Y:wsiadz", 10, sh - 35, 14, WHITE);
        DrawText(TextFormat("Pozycja: (%.1f, %.1f, %.1f)", 
                player.position.x, player.position.y, player.position.z),
                10, sh - 18, 14, YELLOW);
    }

    char timeStr[32];
    if (timeOfDay < 0.5f) sprintf(timeStr, "Dzien %.0f%%", timeOfDay*200);
    else sprintf(timeStr, "Noc %.0f%%", (timeOfDay-0.5f)*200);
    DrawText(timeStr, sw - 150, 45, 16, WHITE);
}

int main(int argc, char *argv[]) {
    InitWindow(640, 480, "GTAsaDC - Third Person");
    SetTargetFPS(60);
    InitPlayer();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        firePhase += dt * 3.0f;
        
        UpdatePlayer(dt);
        UpdateDayNight(dt);
        UpdateGameCamera();
        
        BeginDrawing();
        {
            ClearBackground(bgColor);
            
            BeginMode3D(camera);
            {
                DrawMap();
            }
            EndMode3D();
            
            DrawHUD();
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}