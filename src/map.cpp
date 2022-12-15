#include "map.h"
#include <sstream>
#include <cstring>
#include "utils.h"

using namespace tinyxml2;

void ParseMapNode(Tilemap *map, XMLElement *element, std::function<Object(XMLElement *)> objectInitialiser);
void ParseLayer(Tilemap *map, XMLElement *element);
void ParseLayerData(Layer *layer, XMLElement *element);
void ParseObjectLayer(Tilemap *map, XMLElement *element, std::function<Object(XMLElement *)> objectInitialiser);
void ParseProperties(PropertyCollection *properties, XMLElement *element);
Object ParseObject(XMLElement *element);

Tileset LoadSet(const char* filename) {
    Tileset set;
    XMLDocument doc;

    if (doc.LoadFile(filename) != XML_SUCCESS) {
        std::cout << "Error loading the file" << std::endl;
        return set;
    }

    XMLElement *element = doc.FirstChildElement("tileset");
    set.name = element->Attribute("name");
    set.tileWidth = element->IntAttribute("tilewidth");
    set.tileHeight = element->IntAttribute("tileheight");
    set.tileCount = element->IntAttribute("tilecount");

    set.width = element->IntAttribute("columns");
    set.height = set.tileCount / set.width;

    XMLElement *image = element->FirstChildElement("image");
    if (image != NULL) {
        set.path = image->Attribute("source");
    }

    ParseProperties(&set.properties, element->FirstChildElement("properties"));

    for (XMLElement *tile = element->FirstChildElement("tile"); tile != NULL; tile = tile->NextSiblingElement("tile")) {
        int id = tile->IntAttribute("id") + set.startingGid;
        set.hitboxes[id] = std::vector<Rectangle>();
        XMLElement *objectGroup = tile->FirstChildElement("objectgroup");
        if (objectGroup != NULL) {
            for (XMLElement *object = objectGroup->FirstChildElement("object"); object != NULL; object = object->NextSiblingElement("object")) {
                if (object->Attribute("name") != NULL) continue;
                set.hitboxes[id].push_back(Rectangle {
                    object->FloatAttribute("x"),
                    object->FloatAttribute("y"),
                    object->FloatAttribute("width"),
                    object->FloatAttribute("height")
                });
            }
        }
        XMLElement* el = tile->FirstChildElement("animation");
        if (el != NULL) {
            for (XMLElement *frame = el->FirstChildElement("frame"); frame != NULL; frame = frame->NextSiblingElement("frame")) {
                set.animations[id].push_back(Frame {
                    frame->IntAttribute("tileid") + 1,
                    frame->IntAttribute("duration")
                });
            }
        }
    }

	return set;
}

Tilemap LoadMap(const char* filename) {
	return LoadMap(filename, &ParseObject);
}

Tilemap LoadMap(const char* filename, std::function<Object(XMLElement *)> objectInitialiser) {
    Tilemap map;
    XMLDocument doc;

    if (doc.LoadFile(filename) != XML_SUCCESS) {
        std::cout << "Error loading the file" << std::endl;
        return map;
    }

    ParseMapNode(&map, doc.FirstChildElement("map"), objectInitialiser);
	return map;
}

void ParseMapNode(Tilemap *map, XMLElement *element, std::function<Object(XMLElement *)> objectInitialiser) {
    if (element == NULL) {
        std::cout << "No map node" << std::endl;
        return;
    }

    map->tileWidth = element->IntAttribute("tilewidth");
    map->tileHeight = element->IntAttribute("tileheight");
    map->width = element->IntAttribute("width");
    map->height = element->IntAttribute("height");
    map->rect = Rectangle {0, 0, (float) map->width * map->tileWidth, (float) map->height * map->tileHeight};
	ParseProperties(&map->properties, element->FirstChildElement("properties"));

    const char* orientation = element->Attribute("orientation");
    if (strcmp(orientation, "isometric")) {
        map->orientation = Orientation::Isometric;
    } else if (strcmp(orientation, "orthagonal")) {
        map->orientation = Orientation::Orthogonal;
    } else {
        std::cout << "No orientation attribute" << std::endl;
    }

    const char* renderOrder = element->Attribute("renderorder");
    if (strcmp(renderOrder, "left-down") == 0) {
        map->renderOrder = RenderOrder::LeftDown;
    } else if (strcmp(renderOrder, "left-up") == 0) {
        map->renderOrder = RenderOrder::LeftUp;
    } else if (strcmp(renderOrder, "right-down") == 0) {
        map->renderOrder = RenderOrder::RightDown;
    } else if (strcmp(renderOrder, "right-up") == 0) {
        map->renderOrder = RenderOrder::RightUP;
    } else {
        std::cout << "No render order attribute" << std::endl;
    }
    for (XMLElement *layer = element->FirstChildElement(); layer != NULL; layer = layer->NextSiblingElement()) {
        if (strcmp(layer->Name(), "layer") == 0) {
			ParseLayer(map, layer);
		} else if (strcmp(layer->Name(), "objectgroup") == 0) {
			ParseObjectLayer(map, layer, objectInitialiser);
		}
    }
}

void ParseProperties(PropertyCollection *properties, XMLElement *element) {
	if (element == NULL) {
		return;
	}
	
	for (XMLElement *xmlprop = element->FirstChildElement("property"); xmlprop != NULL; xmlprop = xmlprop->NextSiblingElement("property")) {
        Property property;
		property.name = xmlprop->Attribute("name");
		property.value = xmlprop->Attribute("value");
		property.type = xmlprop->Attribute("type") == NULL ? "string" : xmlprop->Attribute("type");
		properties->properties.push_back(property);
    }
}

void ParseLayer(Tilemap *map, XMLElement *element) {
    Layer &layer = map->layers[element->Attribute("name")];

    layer.name = element->Attribute("name");
    layer.id = element->IntAttribute("id");
    layer.width = element->IntAttribute("width");
    layer.height = element->IntAttribute("height");

    if (element->Attribute("opacity"))
        layer.opacity = element->FloatAttribute("opacity");
    else {
        layer.opacity = 1;
	}
    if (element->Attribute("visible"))
        layer.visible = (element->IntAttribute("visible") == 1 ? true : false);
    else {
        layer.visible = true;
	}
    
	ParseProperties(&map->properties, element->FirstChildElement("properties"));
    ParseLayerData(&layer, element->FirstChildElement("data"));

    map->layerNames.push_back(layer.name);
}

void ParseLayerData(Layer *layer, XMLElement *element) {
    if (element == NULL) {
        std::cout << "No data element" << std::endl;
        return;
    }

	const char* encoding = element->Attribute("encoding");

	if (strcmp(encoding, "csv") == 0)
	{
		std::stringstream csvss (element->FirstChild()->Value());

		int gid = 0;
		while (csvss >> gid)
		{
			if (csvss.peek() == ',' || csvss.peek() == '\n')
			{
				csvss.ignore();
			}

			layer->tiles.push_back(Tile {gid});
		}
	}
}

void ParseObjectLayer(Tilemap *map, XMLElement *element, std::function<Object(XMLElement *)> objectInitialiser) {
	ObjectLayer &layer = map->objectLayers[element->Attribute("name")];

	layer.id = element->IntAttribute("id");
	layer.name = element->Attribute("name");

	ParseProperties(&layer.properties, element->FirstChildElement("properties"));

	for (XMLElement *object = element->FirstChildElement("object"); object != NULL; object = object->NextSiblingElement("object")) {
		layer.objects[object->Attribute("name")].push_back(ParseObject(object));
	}
}

Object ParseObject(XMLElement *element) {
    if (!element->NoChildren()) {
        if (strcmp(element->FirstChildElement()->Name(), "text")) {
            TextObject object;
            XMLElement* textElement = element->FirstChildElement("text");
            object.wrap = textElement->BoolAttribute("wrap", false);
            object.bold = textElement->BoolAttribute("bold", false);
            object.italic = textElement->BoolAttribute("italic", false);
            object.underline = textElement->BoolAttribute("underline", false);
            object.strikeout = textElement->BoolAttribute("strikeout", false);
            object.kerning = textElement->BoolAttribute("kerning", false);
            object.fontSize = textElement->IntAttribute("fontsize", 16);
            object.fontFamily = textElement->Attribute("fontfamily", "Arial");
            object.verticalAlignment = textElement->Attribute("verticalalignment", "top");
            object.horizontalAlignment = textElement->Attribute("horizontalalignment", "left");
            object.text = textElement->GetText();
            return object;
        }
    }

    Object object;
    object.id = element->IntAttribute("id");
    object.name = element->Attribute("name");
    object.rect = {
        element->FloatAttribute("x"), 
        element->FloatAttribute("y"), 
        element->FloatAttribute("width"), 
        element->FloatAttribute("height")
    };

    return object;
}

void TilesetCollection::AddTileset(const char* path, Texture2D image) {
    Tileset tileset = LoadSet(path);
    tileset.image = image;
    int startingGid = 1;
    
    for (Tileset &set : tilesets) {
        startingGid += set.tileCount;
    }

    tileset.startingGid = startingGid;
    tilesets.emplace_back(tileset);
}

void TilesetCollection::AddTileset(Texture2D image, int tileWidth, int tileHeight) {
    Tileset tileset;
    tileset.image = image;
    tileset.tileWidth = tileWidth;
    tileset.tileHeight = tileHeight;
    tileset.width = image.width / tileWidth;
    tileset.height = image.width / tileHeight;
    tileset.tileCount = tileset.width * tileset.height;
    int startingGid = 1;
    
    for (Tileset &set : tilesets) {
        startingGid += set.tileCount;
    }

    tileset.startingGid = startingGid;
    tilesets.emplace_back(tileset);
}

void Tilemap::Clear() {
    for (auto &[num, layer] : layers) {
        layer.tiles.clear();
    }
    for (auto &[num, layer] : objectLayers) {
        layer.objects.clear();
    }
}

Rectangle Tilemap::GetArea(Camera2D camera, int tileWidth, int tileHeight) {
    int sw = (GetScreenWidth() / tileWidth) / camera.zoom;
    int sh = (GetScreenHeight() / tileHeight) / camera.zoom;
    
    int startingx = (int) min(camera.offset.x / tileWidth / camera.zoom, 0);
    int startingy = (int) min(camera.offset.y / tileHeight / camera.zoom, 0);
    int endingx = (int) min(max(camera.offset.x / tileWidth + sw + 2, width), 0);
    int endingy = (int) min(max(camera.offset.y / tileHeight + sh + 2, height), 0);

    return Rectangle {(float) startingx, (float) startingy, (float) endingx, (float) endingy};
}

void Tilemap::Draw(TilesetCollection &tilesets, Camera2D camera, Vector2 offset, Color tint) {
    if (tilesets.tilesets.size() == 0) {
        error("Not enough tilesets");
    }

    float tileWidth = tilesets.tilesets[0].tileWidth;
    float tileHeight = tilesets.tilesets[0].tileHeight;
    int sw = (GetScreenWidth() / tileWidth) / camera.zoom;
    int sh = (GetScreenHeight() / tileHeight) / camera.zoom;
    
    int startingx = min(camera.offset.x / tileWidth / camera.zoom, 0);
    int startingy = min(camera.offset.y / tileHeight / camera.zoom, 0);
    int endingx = min(max(camera.offset.x / tileWidth + sw + 2, width), 0);
    int endingy = min(max(camera.offset.y / tileHeight + sh + 2, height), 0);
    
    if (startingx != endingx || startingy != endingy) {
        for (std::string layerName : layerNames) {
            DrawLayer(layerName, tilesets, camera, Rectangle {(float) startingx, (float) startingy, (float) endingx, (float) endingy}, offset, tint);
        }
    }
}

void Tilemap::DrawLayer(std::string layerName, TilesetCollection &tilesets, Camera2D camera, Vector2 offset, Color tint) {
    DrawLayer(layerName, tilesets, camera, Rectangle {0, 0, (float) width, (float) height}, offset, tint);
}

void Tilemap::DrawLayer(std::string layerName, TilesetCollection &tilesets, Camera2D camera, Rectangle area, Vector2 offset, Color tint) {
    if (layers.find(layerName) == layers.end()) return;
    
    Layer &layer = layers[layerName];
    for (int x = area.x; x < area.width; x++) {
        for (int y = area.y; y < area.height; y++) {
            Tile* tile = layer.Getat({(float) x, (float) y});
            if (tile->gid == 0) continue;

            Tileset* tileset = tilesets.Get(tile->gid);
            if (tileset != nullptr) {
                int gid = tile->gid;

                // Animation
                if (tileset->animations.find(gid) != tileset->animations.end()) {
                    tile->animationIndex += 1;
                    int sum = 0;
                    for (Frame frame : tileset->animations.at(gid)) {
                        sum += ((float) frame.duration / 1000) * GetFPS();
                        if (sum > tile->animationIndex) {
                            gid = frame.gid;
                            break;
                        }
                    }
                    if (sum <= tile->animationIndex) {
                        tile->animationIndex = 0;
                        gid = tileset->animations[gid][0].gid;
                    }
                }

                Rectangle source = tileset->GetSourceRect(gid);
                Rectangle dest = {
                    (x * tileWidth - camera.offset.x) * camera.zoom + offset.x, 
                    (y * tileHeight - camera.offset.y) * camera.zoom + offset.y, 
                    tileWidth * camera.zoom, 
                    tileHeight * camera.zoom
                };
                DrawTexturePro(tileset->image, source, dest, Vector2 {0, 0}, 0, tint);
            }
        }
    }
}

void DrawTile(int gid, Vector2 pos, TilesetCollection &tilesets, Camera2D cam, Color tint, bool flippedX, bool flippedY) {
    if (gid == 0) return;

    Tileset* tileset = tilesets.Get(gid);
    if (tileset != nullptr) {
        Rectangle source = tileset->GetSourceRect(gid);
        Rectangle dest = {
            (pos.x - cam.offset.x) * cam.zoom, 
            (pos.y - cam.offset.y) * cam.zoom, 
            tileset->tileWidth * cam.zoom, 
            tileset->tileHeight * cam.zoom
        };
        if (!CheckCollisionRecs(dest, Rectangle {0, 0, (float) GetScreenWidth(), (float) GetScreenHeight()})) return;

        if (flippedX) {
            source = Rectangle {source.x, source.y, -source.width, source.height};
        }
        if (flippedY) {
            source = Rectangle {source.x, source.y, source.width, -source.height};
        }

        DrawTexturePro(tileset->image, source, dest, Vector2 {0, 0}, 0, tint);
    }
}

/*
void DrawWorld() {
    Rectangle area = game.map.GetArea(game.cam, tileWidth, tileHeight);
    
    for (int y = area.y; y < game.map.height; y++) {
        for (const auto &[id, name] : game.layersIds) {
            int yoffset = 0;
            if (id > 1) {
                yoffset = id - 1;
                if (y - yoffset < 0) {
                    continue;
                }
            }
            
            if (y - yoffset >= area.height) continue;

            game.map.DrawLayer(name, game.tilesets, game.cam, Rectangle {area.x, (float) y - yoffset, area.width, (float) (y - yoffset) + 1}, Vector2 {0, 0}, WHITE);
        
            if (id == 0) {
                if (y * tileHeight <= player.rect.y + player.rect.height && player.rect.y + player.rect.height < (y + 1) * tileHeight) {
                    player.Draw(game.tilesets, game.cam);
                }
            }
        }
    }
}
*/