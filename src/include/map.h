#pragma once
#include <functional>
#include "pch.h"
#include "tinyxml2.h"
#include "debug.h"

class TilesetCollection;

enum Orientation {
    Orthogonal,
    Isometric
};

enum RenderOrder {
    LeftDown,
    LeftUp,
    RightDown,
    RightUP,
};

struct Tile {
    int gid;
    int animationIndex = 0;
};

struct Frame {
    int gid;
    int duration;
};

class Property {
public:
    std::string name;
	std::string type;
	std::string value;

    inline int GetInt() const
	{
		if (type != "int" || value.empty()) exit(1);
		return atoi(value.c_str());
	}

	inline float GetFloat() const
	{
		if (type != "float" || value.empty()) exit(1);

		return float(atof(value.c_str()));
	}

	inline const char* GetString() const
	{
        if (type != "string") exit(1);

		return value.c_str();
	}
};

class PropertyCollection {
public:
    std::vector<Property> properties;

    inline const Property* GetProperty(std::string name) {
        for (const Property &property : properties) {
            if (property.name == name) return &property;
        }
        return nullptr;
    }

    inline std::vector<Property> GetPropertyAll(std::string name) {
        std::vector<Property> propertiesOfType;
        for (const Property &property : properties) {
            if (property.name == name) {
                propertiesOfType.emplace_back(property);
            }
        }
        return properties;
    }

    inline void AddProperty(std::string name, std::string type, std::string value) {
        Property property;
        property.name = name;
        property.type = type;
        property.value = value;
        properties.push_back(property);
    }

};

class Object {
public:
    int id = -1;
    std::string name;
    Rectangle rect;
    PropertyCollection properties; 
};

class Layer {
public:
    int id = -1;
    int width, height;
    float opacity = 1;
    std::string name;
    bool visible = true;
    std::vector<Tile> tiles;
    PropertyCollection properties;

    inline Tile* Getat(Vector2 pos) {
        if (pos.x < width && pos.x >= 0 && pos.y < height && pos.y >= 0) {
            int index = (pos.y * width) + pos.x;
            if (index >= 0 && index < (int) tiles.size()) {
                return &tiles[index];
            }
        }
        return nullptr;
    }

    inline void SetAt(Vector2 pos, Tile tile) {
        if (pos.x < width && pos.x >= 0 && pos.y < height && pos.y >= 0) {
            int index = (pos.y * width) + pos.x;
            if (index >= 0 && index < (int) tiles.size()) {
                tiles.at(index) = tile;
            }
        }
    }
};

class ObjectLayer {
public:
    int id = -1;
    std::string name;
    PropertyCollection properties;
    std::map<std::string, std::vector<Object>> objects;

    inline Object* GetObj(std::string type) {
        if (objects.find(type) == objects.end()) return nullptr;
        for (Object &obj : objects[type]) {
            return &obj;
        }
        return nullptr;
    }
    
    inline std::vector<Object>* GetObjs(std::string type) {
        if (objects.find(type) == objects.end()) return nullptr;
        return &objects[type];
    }
};

class Tilemap {
public:
    int width, height, tileWidth, tileHeight;
    Rectangle rect;
    Orientation orientation;
    RenderOrder renderOrder;
    PropertyCollection properties;
    std::vector<std::string> layerNames;
    std::map<std::string, Layer> layers;
    std::map<std::string, ObjectLayer> objectLayers;

    inline Layer* GetLayerID(int id) {
        for (auto &[name, layer] : layers) {
            if (layer.id == id) {
                return &layer;
            }
        }
        return nullptr;
    }
    
    inline ObjectLayer* GetObjLayerID(int id) {
        for (auto &[num, layer] : objectLayers) {
            if (layer.id == id) {
                return &layer;
            }
        } 
        return nullptr;
    }

    void Draw(TilesetCollection &tilesets, Camera2D camera, Vector2 offset = {0, 0}, Color tint = WHITE);
    void DrawLayer(std::string layerName, TilesetCollection &tilesets, Camera2D camera, Vector2 offset = {0, 0}, Color tint = WHITE);
    void DrawLayer(std::string layerName, TilesetCollection &tilesets, Camera2D camera, Rectangle area, Vector2 offset = {0, 0}, Color tint = WHITE);

    Rectangle GetArea(Camera2D camera, int tileWidth, int tileHeight);
    void Clear();
};

class TextObject : public Object {
public:
	Color yextColor = WHITE;
	int fontSize = 20;
	bool wrap = false;
	bool bold = false;
	bool italic = false;
	bool underline = false;
	bool strikeout = false;
	bool kerning = true;
	std::string text;
	std::string fontFamily;
	std::string horizontalAlignment = "left";
	std::string verticalAlignment = "top";
};

class Tileset {
public:
    std::string name;
    std::string path;
    std::map<int, std::vector<Frame>> animations;
    std::map<int, std::vector<Rectangle>> hitboxes;
    int width, height, tileWidth, tileHeight, tileCount;
    int startingGid = 1;
    Texture2D image;
    PropertyCollection properties;
    
    inline std::vector<Rectangle> GetHitbox(int gid) {
        gid -= startingGid - 1;
        if (hitboxes.find(gid) == hitboxes.end()) {
            return std::vector<Rectangle>();
        }
        return hitboxes[gid];
    }

    inline std::vector<Frame> GetAnimation(int gid) {
        gid -= startingGid - 1;
        if (animations.find(gid) == animations.end()) {
            return std::vector<Frame>();
        }
        return animations[gid];
    }
    
    inline Rectangle GetSourceRect(int gid) {
        gid -= startingGid; // Don't subtract 1 here because top left in this context is id 0
        int x = gid % width;
        int y = gid / width;
        return Rectangle {(float) x * tileWidth, (float) y * tileHeight, (float) tileWidth, (float) tileHeight};
    }

};

class TilesetCollection {
public:
    std::vector<Tileset> tilesets;
    int tileCount;

    void AddTileset(const char* path, Texture2D image);
    void AddTileset(Texture2D image, int tileWidth, int tileHeight);

    inline Tileset* Get(int gid) {
        for (Tileset &tileset : tilesets) {
            if (tileset.startingGid <= gid && tileset.startingGid + tileset.tileCount > gid) {
                return &tileset;
            }
        }
        return nullptr;
    }
    
    inline Tileset* Get(const char* name) {
        for (Tileset &tileset : tilesets) {
            if (tileset.name == name) {
                return &tileset;
            }
        }
        return nullptr;
    }
};

void DrawTile(int gid, Vector2 pos, TilesetCollection &tilesets, Camera2D cam, Color tint = WHITE, bool flippedX = false, bool flippedY = false);

inline Rectangle GetSourceRect(int gid, Vector2 textureSize, int tileWidth, int tileHeight) {
    int x = gid % ((int) textureSize.x / tileWidth);
    int y = gid / ((int) textureSize.x / tileWidth);
    return Rectangle {(float) x * tileWidth, (float) y * tileHeight, (float) tileWidth, (float) tileHeight};
}

Tileset LoadSet(const char* filename);
Tilemap LoadMap(const char* filename);
Tilemap LoadMap(const char* filename, std::function<Object(tinyxml2::XMLElement*)> objectInitialiser);
