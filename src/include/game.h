#pragma once
#include "ui.h"
#include "pch.h"
#include "map.h"
#include "debug.h"
#include "utils.h"

#if defined(PLATFORM_WEB)
    #define GLSL_VERSION            100
#else
    #define GLSL_VERSION            330
#endif

const int tileWidth = 16;
const int tileHeight = 16;
const int itemTileSize = 12;

const Vector2 fruitIds = {0, 30};
const Vector2 rottenFruitIds = {31, 40};
const int diamondId = 41;
const int bombId = 42;
const int dynomiteId = 43;
const int heartId = 44;
const int longWagonId = 48;
const int magnetId = 49;
const int lightningId = 50;

const int heartEmptyId = 45;
const int heartFullId = 46;
const int heartHalfFullId = 47;

const Color positiveColor = {103, 209, 50, 255};
const Color negativeColor = {240, 47, 34, 255};

enum ApplicationStates {
    Loading,
    TitleScreen,
    Running,
    GameOver
};

enum Textures {
    cart,
    tiles,
    tractor,
    wheelLarge,
    wheelSmall,
    halo,
    smoke,
    items,
    title,
    coinsheet,
    normalFont,
    titleScreenBg1, 
    titleScreenBg2,
    explosion,
    gui,
    pauseButtons,
    map1,
    map2,
    map3
};

enum Sounds {
    LongWagonVoice,
    ExtraLongWagonVoice,
    BoomVoice,
    MagnetVoice,
    SpeedVoice
};

enum Fonts {
    normal
};

enum EffectType {
    LongWagon,
    Magnet,
    Lightning
};

enum Shaders {
    None,
    FX_GRAYSCALE,
    FX_POSTERIZATION,
    FX_DREAM_VISION,
    FX_PIXELIZER,
    FX_CROSS_HATCHING,
    FX_CROSS_STITCHING,
    FX_PREDATOR_VIEW,
    FX_SCANLINES,
    FX_FISHEYE,
    FX_SOBEL,
    FX_BLOOM,
    FX_BLUR,
};

struct FallingItem {
    FallingItem(Vector2 _pos, int _id, float _yVel) : pos(_pos), id(_id), yVel(_yVel), initalYVel(_yVel) {}

    Vector2 pos;
    int id;
    float xVel = 0;
    float yVel;
    float initalYVel;
    int lifetime = 0;
    bool hasHitGround = false;
    bool insideCart = false;
};

class GameData {
public:
    struct Upgrade {
        int total;
        int unlocked = 0;
        std::vector<int> prices;
        std::vector<float> values;  // Don't forget to include a value when none are unlocked

        Upgrade(int _total, std::vector<int> _prices, std::vector<float> _values)
            : total(_total), prices(_prices), values(_values) {}; 
    };
    
    Upgrade speedUpgrade = Upgrade(6, {100, 200, 350, 600, 900, 1500}, {2, 2.25, 2.50, 2.85, 3.25, 3.6, 4.1});
    Upgrade healthUpgrade = Upgrade(6, {100, 200, 350, 600, 900, 1500}, {4, 6, 8, 10, 12, 14, 16});
    Upgrade luckUpgrade = Upgrade(6, {100, 200, 350, 600, 900, 1500}, {0, 1, 2, 3, 4, 5, 6});
    
    int selectedColor = 0;
    std::vector<Color> colors = {
        Color {122, 156, 253, 255},
        Color {12, 170, 191, 255},
        Color {124, 242, 80, 255},
        Color {50, 184, 75, 255},
        Color {250, 83, 154, 255},
        Color {232, 88, 88, 255},
        Color {136, 51, 172, 255},
        Color {191, 83, 238, 255}
    };
    
    int coins = 0;
    int inGameCoins;
    
    Textures selectedMap = Textures::map1;
    Shaders selectedShader = Shaders::None;

    bool colorsUnlocked = false;
    bool backgroundUnlocked = false;
    bool effectsUnlocked = false;

    int totalCoins() {return coins + inGameCoins;};
    void fromString(std::string);
    std::string toString();
};


class Effect {
public:
    EffectType type;
    int lifetime;

    Effect(EffectType _effectType, int _lifetime) : type(_effectType), lifetime(_lifetime) {};

    bool finished() {
        return timer >= lifetime;
    }

    void increment() {
        if (timer < lifetime)
            timer++;
    }

    int getTime() {
        return timer;
    }

private:
    int timer = 0;
};


class Particle {
public:
    int lifetime;
    int timer;
    bool isDead = false;
    Vector2 pos;
    Vector2 vel;

    virtual void Draw(Camera2D cam) = 0;
};

class ScoreParticle : public Particle {
public:
    std::string text;
    bool posative;
    
    ScoreParticle(int score, Vector2 position, bool isHp = false);
    void Draw(Camera2D cam);
};

class ExplosionParticle : public Particle {
public:
    Vector2 center;

    ExplosionParticle(Vector2 _center) : center(_center) {};
    void Draw(Camera2D cam);

private:
    int timer = 0;
};

class Transition {
public:
    int timer;
    int totalDuration;
    bool isFinished = false;
    bool isReversed = false;
    const char *name;

    Transition() = default;
    Transition(const char *_name, int _totalDuration, bool _isReversed = false);
    virtual void Draw() = 0;
};

class BoxTransition : public Transition {
public:
    int boxSize;
    using Transition::Transition;
    void Draw();
};

class FadeTransition : public Transition {
public:
    Color color;

    FadeTransition(const char *_name, int _totalDuration, Color _color, bool _isReversed = false);
    void Draw();
};

class Animation {
private:
    int timer;

public:
    bool isReapting;
    bool isFinished;
    std::vector<int> frameDurations;
    std::vector<int> frames;

    Animation(){};
    Animation(std::vector<int> frameVector, int frameDur, bool repeating);
    Animation(std::vector<int> frameVector, std::vector<int> frameDurs, bool repeating);

    void Update();
    void Reset();
    int Get();
};

void DrawCoins(Vector2 startPos, int numOfCoins, bool updateAnimation=true);
void SetNextShader(Shaders shader);
bool isTransitionFinished(const char *name);
GameData &GetGameData();
std::string GetGameDataString(GameData &game);

Texture2D &GetTexture(Textures texture);
Sound &GetSound(Sounds sound);
JakeFont &GetFont(Fonts font);
