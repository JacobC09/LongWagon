#pragma once
#include "game.h"
#include "pch.h"

struct SmokeParticle {
    int lifetime;
    int timer;
    int type;
    Color color;
    Vector2 pos;
    Vector2 vel;
};

class Tractor {
public:
    int idleAnimationTimer;
    int runningAnimationTimer;
    int cartDesiredDis;
    int flipTimer;
    int rainbowTimer;

    float cartX;
    float ySquish;
    
    bool facingRight;
    bool isMoving;
    bool isLongWagon;
    bool isRainbow;
    Color color;

    int smokeParticleTimer;
    std::vector<SmokeParticle> smokeParticlces;

    Vector2 momentum;
    Rectangle rect;
    Rectangle hitbox;
    
    void Init(int sw, Camera2D cam, int startY);
    void Update(Camera2D cam, GameData &game, bool canMove=true, float customSpeed=-1);
    void DrawParticles(Camera2D cam, bool updateParticles=true);
    void Draw(Camera2D cam, bool updateAnimations=true);

    Rectangle GetTractorRect();
    Rectangle GetCartRect();
};