#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "../log.h"

#include "draw_font.h"
#include "font_cache.h"

#include "../collections/robin_hood_hashing/hash_algos.h"
#include "../collections/robin_hood_hashing/robin_hood.h"

#include "../components/tiles.h"

int CHARACTER_LEN = 254;
wchar_t CHARACTER_ARRAY[254] = {
    0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25D8, 0x25CB,
    0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C, 0x25BA, 0x25C4, 0x2195,
    0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8, 0x2191, 0x2193, 0x2192, 0x2190,
    0x221F, 0x2194, 0x25B2, 0x25BC, 0x0020, 0x0021, 0x0022, 0x0023, 0x0024,
    0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D,
    0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036,
    0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048,
    0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051,
    0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A,
    0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x0060, 0x0061, 0x0062, 0x0063,
    0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C,
    0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075,
    0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E,
    0x2302, 0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5, 0x00C9,
    0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6,
    0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, 0x00E1, 0x00ED, 0x00F3,
    0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD,
    0x00BC, 0x00A1, 0x00AB, 0x00BB, 0x2591, 0x2592, 0x2593, 0x2502, 0x2524,
    0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C,
    0x255B, 0x2510, 0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E,
    0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A,
    0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, 0x03B1, 0x00DF,
    0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9,
    0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229, 0x2261, 0x00B1, 0x2265, 0x2264,
    0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F,
    0x00B2, 0x25A0,
};

bool make_bitmap(FontCache *cache, wchar_t c) {
    FT_UInt glyph_index = FT_Get_Char_Index(cache->face, c);

    if (FT_Load_Glyph(cache->face, glyph_index, FT_LOAD_DEFAULT) != 0) {
        log_error("could not load glyph for %C", c);
        return false;
    }

    if (FT_Render_Glyph(cache->face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
        log_error("could not render glyph for %C", c);
        return false;
    }

    // {
    //     unsigned int rows;
    //     unsigned int width;
    //     int pitch;
    //     unsigned char *buffer;
    //     unsigned short num_grays;
    //     unsigned char pixel_mode;
    //     unsigned char palette_mode;
    //     void *palette;
    // }
    FT_Bitmap *bitmap = malloc(sizeof(*bitmap));
    bitmap->buffer = NULL;
    bitmap->palette = NULL;
    bitmap->palette_mode = 0;
    bitmap->rows = 0;
    bitmap->width = 0;
    bitmap->num_grays = 0;
    bitmap->pixel_mode = 0;
    bitmap->pitch = 0;

    if (FT_Bitmap_Copy(cache->library, &cache->face->glyph->bitmap, bitmap) !=
        FT_Err_Ok) {
        log_error("could not copy bitmap\n");
        free(bitmap);

        return false;
    }

    wchar_t *key = malloc(sizeof(wchar_t));

    if (key == NULL) {
        log_error("could not allocate key\n");
        free(bitmap);
        return false;
    }

    *key = c;

    if (insert_value(cache->bitmap_cache, key, bitmap) == false) {
        log_error("could not insert in to hashmap");
        free(bitmap);
        free(key);
        return false;
    }

    if (bitmap->width > cache->max_char_width) {
        cache->max_char_width = bitmap->width;
    }

    if (bitmap->rows > cache->max_char_height) {
        cache->max_char_height = bitmap->rows;
    }

    return true;
}

bool make_bitmaps(FontCache *font_cache, int point_size) {
    if (FT_Set_Char_Size(font_cache->face, 0, point_size * 64, 0, 72) != 0) {
        log_error("cant set font size");
        return false;
    }

    bool success = true;

    for (int i = 0; i < CHARACTER_LEN && success; ++i) {
        success = make_bitmap(font_cache, CHARACTER_ARRAY[i]);
    }

    return success;
}

uint64_t wchar_hash(void *key) {
    return integer_hash64(*(wchar_t *)key);
}

FontCache *allocate_cache() {
    FontCache *cache = malloc(sizeof(*cache));

    if (cache == NULL) {
        return NULL;
    }

    cache->library = NULL;
    cache->face = NULL;

    cache->bitmap_cache = create_map(wchar_hash);

    if (cache->bitmap_cache == NULL) {
        log_error("could not make bitmap_cache");
        return NULL;
    }

    cache->sprite_rects = create_map(wchar_hash);

    if (cache->sprite_rects == NULL) {
        log_error("could not make sprite_rects");
        return NULL;
    }

    return cache;
}

FontCache *_init_font_cache(char font_path[], int point_size) {
    FontCache *cache = allocate_cache();

    if (cache == NULL) {
        drop_font_cache(cache);
        return NULL;
    }

    if (FT_Init_FreeType(&cache->library) != 0) {
        drop_font_cache(cache);
        return NULL;
    }

    if (FT_New_Face(cache->library, font_path, 0, &cache->face) != 0) {
        drop_font_cache(cache);
        return NULL;
    }

    return cache;
}

FontCache *init_font_cache(char font_path[], int point_size) {
    log_info("initing font cache");

    if (font_path == NULL) {
        log_error("font path is null");
        return NULL;
    }

    FontCache *cache = _init_font_cache(font_path, point_size);

    if (cache == NULL) {
        log_error("could not initalize font cache");
    }

    if (make_bitmaps(cache, point_size) == false) {
        log_error("could not make bitmaps");

        drop_font_cache(cache);

        return NULL;
    }

    return cache;
}

void drop_font_cache(FontCache *cache) {
    if (cache->face) {
        FT_Done_Face(cache->face);
    }

    if (cache->library) {
        FT_Done_FreeType(cache->library);
    }

    if (cache->bitmap_cache) {
        drop_map(cache->bitmap_cache);
    }

    if (cache->sprite_rects) {
        drop_map(cache->sprite_rects);
    }

    free(cache);
}

FontSpriteCache *init_sprite_cache(FontCache *cache) {
    log_info("making character sprites");

    FontSpriteCache *sprites = malloc(sizeof(FontSpriteCache));

    if (sprites == NULL) {
        return NULL;
    }

    size_t char_width = cache->max_char_width;
    size_t char_height = cache->max_char_height;

    sprites->width = 12 * char_width;
    sprites->height = ((CHARACTER_LEN / 2) + 5) * char_height;

    size_t total = sprites->width * sprites->height;

    sprites->data = malloc(sizeof(uint32_t) * total);

    if (sprites->data == NULL) {
        log_error("could not make sprite cache");
        free(sprites);
        return NULL;
    }

    for (int i = 0; i < total; ++i) {
        // sprites->data[i] = 0xffffffff;
        sprites->data[i] = 0xffff00ff;
    }

    Color fg = {255, 255, 255};

    if (draw_characters(cache, sprites, &fg, CHARACTER_ARRAY, CHARACTER_LEN) ==
        false) {
        log_error("could not make sprites");
        free(sprites->data);
        free(sprites);
        return NULL;
    }

    return sprites;
}

typedef struct {
    size_t r;
    size_t b;
    size_t g;
    size_t a;
} ColorMask;

ColorMask *make_color_mask() {
    ColorMask *cm = malloc(sizeof(ColorMask));

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    cm->r = 0xff000000;
    cm->g = 0x00ff0000;
    cm->b = 0x0000ff00;
    cm->a = 0x000000ff;
#else
    cm->r = 0x000000ff;
    cm->g = 0x0000ff00;
    cm->b = 0x00ff0000;
    cm->a = 0xff000000;
#endif

    return cm;
}

SDL_Surface *make_font_surface(FontSpriteCache *sprites) {
    ColorMask *cm = make_color_mask();
    SDL_Surface *sp;

    sp = SDL_CreateRGBSurfaceFrom(sprites->data, sprites->width,
                                  sprites->height, 32, sprites->width * 4,
                                  cm->r, cm->g, cm->b, cm->a);

    free(cm);
    return sp;
}

SDL_Rect *get_sprite_rect(FontCache *cache, wchar_t *key) {
    Item *item = lookup_key(cache->sprite_rects, key);

    if (item == NULL) {
        log_error("did not get tile from sprite_rects");

        return NULL;
    }

    return item->value;
}
